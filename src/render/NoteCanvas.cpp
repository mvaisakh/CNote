#include "NoteCanvas.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QMatrix4x4>
#include <cmath>
#include <QSGTransformNode>
#include <QElapsedTimer>
#include "core/Trace.h"
#include "core/PersistenceManager.h"

NoteCanvas::NoteCanvas(QQuickItem *parent) : QQuickItem(parent), m_renderSize(0, 0), m_penColor(Qt::white), m_currentTool(Pen), m_zoomLevel(1.0)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    setAcceptTouchEvents(true);
    setAntialiasing(true);
}

NoteCanvas::~NoteCanvas() {}

#include <QSGVertexColorMaterial>

static InkPoint interpolatePoints(const InkPoint& p0, const InkPoint& p1, const InkPoint& p2, const InkPoint& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    auto catmull = [&](float v0, float v1, float v2, float v3) {
        return 0.5f * ((2.0f * v1) +
                       (-v0 + v2) * t +
                       (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
                       (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3);
    };

    InkPoint p;
    p.x = catmull(p0.x, p1.x, p2.x, p3.x);
    p.y = catmull(p0.y, p1.y, p2.y, p3.y);
    p.pressure = p1.pressure + (p2.pressure - p1.pressure) * t;
    p.timestamp = p1.timestamp + (p2.timestamp - p1.timestamp) * t;
    return p;
}

QSGGeometryNode* NoteCanvas::createStrokeNode(const Stroke& s)
{
    if (s.points.size() < 2) return nullptr;

    // 1. High-Density Spline Interpolation
    std::vector<InkPoint> smoothed;
    if (s.points.size() >= 4) {
        for (size_t i = 0; i < s.points.size() - 1; ++i) {
            const auto& p1 = s.points[i];
            const auto& p2 = s.points[i+1];
            const auto& p0 = (i == 0) ? p1 : s.points[i-1];
            const auto& p3 = (i == s.points.size() - 2) ? p2 : s.points[i+2];

            smoothed.push_back(p1);
            for (int step = 1; step < 5; ++step) {
                smoothed.push_back(interpolatePoints(p0, p1, p2, p3, step / 5.0f));
            }
        }
        smoothed.push_back(s.points.back());
    } else {
        smoothed = s.points;
    }

    QSGGeometryNode *node = new QSGGeometryNode();
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);

    // 2. Solid 2-Vertex Strip
    int vertexCount = static_cast<int>(smoothed.size() * 2);
    QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
    geo->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    
    QSGGeometry::Point2D *v = geo->vertexDataAsPoint2D();
    
    for (size_t i = 0; i < smoothed.size(); ++i) {
        QPointF p(smoothed[i].x, smoothed[i].y);
        QPointF dir;
        
        if (i == 0) {
            dir = QPointF(smoothed[i+1].x, smoothed[i+1].y) - p;
        } else if (i == smoothed.size() - 1) {
            dir = p - QPointF(smoothed[i-1].x, smoothed[i-1].y);
        } else {
            QPointF d1 = p - QPointF(smoothed[i-1].x, smoothed[i-1].y);
            QPointF d2 = QPointF(smoothed[i+1].x, smoothed[i+1].y) - p;
            dir = (d1 + d2) / 2.0;
        }

        float len = std::sqrt(dir.x()*dir.x() + dir.y()*dir.y());
        QPointF normal(0, 0);
        if (len > 0.0001f) {
            normal = QPointF(-dir.y() / len, dir.x() / len);
        }

        float pressure = smoothed[i].pressure;
        float halfWidth = (s.width * (0.15f + pressure * 0.85f)) / 2.0f;
        
        v[i*2].set(p.x() + normal.x() * halfWidth, p.y() + normal.y() * halfWidth);
        v[i*2+1].set(p.x() - normal.x() * halfWidth, p.y() - normal.y() * halfWidth);
    }
    
    node->setGeometry(geo);

    QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
    mat->setColor(s.color);
    mat->setFlag(QSGMaterial::Blending, true);
    node->setMaterial(mat);

    return node;
}

void NoteCanvas::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_renderSize = newGeometry.size().toSize();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void NoteCanvas::updateGeometry(QSGGeometry *geometry, const std::vector<InkPoint>& points, float baseWidth)
{
    int vertexCount = static_cast<int>(points.size() * 2);
    if (geometry->vertexCount() != vertexCount)
        geometry->allocate(vertexCount);

    QSGGeometry::Point2D *v = geometry->vertexDataAsPoint2D();

    for (size_t i = 0; i < points.size(); ++i) {
        QPointF p(points[i].x, points[i].y);
        QPointF dir;
        
        if (i == 0 && points.size() > 1) {
            dir = QPointF(points[i+1].x, points[i+1].y) - p;
        } else if (i > 0 && i == points.size() - 1) {
            dir = p - QPointF(points[i-1].x, points[i-1].y);
        } else if (i > 0 && i < points.size() - 1) {
            QPointF d1 = p - QPointF(points[i-1].x, points[i-1].y);
            QPointF d2 = QPointF(points[i+1].x, points[i+1].y) - p;
            dir = (d1 + d2) / 2.0;
        } else {
            dir = QPointF(1, 0);
        }

        float len = std::sqrt(dir.x()*dir.x() + dir.y()*dir.y());
        QPointF normal(0, 0);
        if (len > 0.0001f) {
            normal = QPointF(-dir.y() / len, dir.x() / len);
        }

        float pressure = points[i].pressure;
        float halfWidth = (baseWidth * (0.15f + pressure * 0.85f)) / 2.0f;

        v[i*2].set(p.x() + normal.x() * halfWidth, p.y() + normal.y() * halfWidth);
        v[i*2+1].set(p.x() - normal.x() * halfWidth, p.y() - normal.y() * halfWidth);
    }
}

#include <QFileInfo>
#include <QDebug>
#include "Trace.h"

void NoteCanvas::setPenColor(const QColor &color) 
{ 
    if (m_penColor != color) { 
        m_penColor = color; 
        emit penColorChanged(); 
    } 
}

QPointF NoteCanvas::mapToCanvas(const QPointF &screenPos) const
{
    return (screenPos - m_panOffset) / m_zoomLevel;
}

QPointF NoteCanvas::mapFromCanvas(const QPointF &canvasPos) const
{
    return canvasPos * m_zoomLevel + m_panOffset;
}

void NoteCanvas::setPdfPath(const QString& path)
{
    if (m_pdfPath == path) return;
    m_pdfPath = path;
    
    QFileInfo checkFile(path);
    if (checkFile.exists() && checkFile.isFile() && path.endsWith(".pdf")) {
        CN_TRACE("Loading PDF: %s", path.toLocal8Bit().constData());
        m_pdfEngine.loadDocument(path.toStdString());
    } else if (path.endsWith(".note")) {
        CN_TRACE("Loading Blank Note: %s", path.toLocal8Bit().constData());
        m_pdfEngine.closeDocument(); // Ensure no previous PDF stays loaded
    } else {
        CN_TRACE("File is not a supported document: %s", path.toLocal8Bit().constData());
    }
    
    m_pdfDirty = true;
    emit pdfPathChanged();
    loadNotes();
    update();
}

void NoteCanvas::mousePressEvent(QMouseEvent *) {}
void NoteCanvas::mouseMoveEvent(QMouseEvent *) {}
void NoteCanvas::mouseReleaseEvent(QMouseEvent *) {}

#include <QTabletEvent>

bool NoteCanvas::event(QEvent *event)
{
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::MouseButtonPress) {
        QSinglePointEvent *spe = static_cast<QSinglePointEvent*>(event);
        QPointingDevice::PointerType ptype = spe->pointingDevice()->pointerType();
        
        // Let Finger events flow to touchEvent()
        if (ptype == QPointingDevice::PointerType::Finger) {
            return QQuickItem::event(event);
        }

        float pressure = spe->point(0).pressure();
        if (pressure <= 0.0f || std::isnan(pressure)) pressure = 0.5f;
        QPointF pos = spe->position();

        if (event->type() == QEvent::MouseButtonPress) {
            m_lastMousePos = pos;
        }


        static QElapsedTimer timer;
        if (!timer.isValid()) timer.start();

        {
            QMutexLocker locker(&m_mutex);
            m_activePoints.clear();
            InkPoint p = { (float)mapToCanvas(pos).x(), (float)mapToCanvas(pos).y(), pressure, 0.0f, timer.elapsed() };
            m_activePoints.push_back(p);
            m_activeStrokeDirty = true;
        }
        update();
        return true;
    } else if (event->type() == QEvent::TabletMove || event->type() == QEvent::MouseMove) {
        QSinglePointEvent *spe = static_cast<QSinglePointEvent*>(event);
        QPointingDevice::PointerType ptype = spe->pointingDevice()->pointerType();
        
        if (ptype == QPointingDevice::PointerType::Finger) {
            return QQuickItem::event(event);
        }

        float pressure = spe->point(0).pressure();
        if (pressure <= 0.0f || std::isnan(pressure)) pressure = 0.5f;
        QPointF pos = spe->position();

        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (!(me->buttons() & (Qt::LeftButton | Qt::MiddleButton))) return QQuickItem::event(event);
            
            // Middle Mouse Panning (Desktop)
            if (me->buttons() & Qt::MiddleButton) {
                QPointF delta = me->position() - m_lastMousePos;
                m_panOffset += delta;
                m_lastMousePos = me->position();
                m_transformDirty = true;
                update();
                return true;
            }
            m_lastMousePos = me->position();
        }
        
        QPointF canvasPos = mapToCanvas(pos);
        
        if (m_currentTool == Eraser) {
            bool changed = false;
            auto it = m_finishedStrokes.begin();
            while (it != m_finishedStrokes.end()) {
                bool hit = false;
                for (const auto& p : it->points) {
                    float dx = p.x - canvasPos.x();
                    float dy = p.y - canvasPos.y();
                    if (dx*dx + dy*dy < (20/m_zoomLevel)*(20/m_zoomLevel)) { 
                        hit = true;
                        break;
                    }
                }
                if (hit) {
                    it = m_finishedStrokes.erase(it);
                    changed = true;
                } else {
                    ++it;
                }
            }
            if (changed) {
                {
                    QMutexLocker locker(&m_mutex);
                    m_strokesDirty = true;
                    m_fullReload = true;
                }
                update();
            }
        } else {
            // Drawing
            float adjustedPressure = std::pow(pressure, 1.2f);
            static QElapsedTimer timer;
            if (!timer.isValid()) timer.start();
            InkPoint p = { (float)canvasPos.x(), (float)canvasPos.y(), adjustedPressure, 0.0f, timer.elapsed() };
            {
                QMutexLocker locker(&m_mutex);
                m_activePoints.push_back(p);
                m_activeStrokeDirty = true;
            }
            update();
        }
        return true;
    } else if (event->type() == QEvent::Wheel) {
        QWheelEvent *we = static_cast<QWheelEvent*>(event);
        float delta = we->angleDelta().y() / 120.0f;
        
        if (we->modifiers() & Qt::ControlModifier) {
            // Zoom relative to mouse position
            float factor = (delta > 0) ? 1.1f : (1.0f / 1.1f);
            QPointF mousePos = we->position();
            QPointF before = mapToCanvas(mousePos);
            m_zoomLevel *= factor;
            QPointF after = mapToCanvas(mousePos);
            m_panOffset += (after - before) * m_zoomLevel;
        } else {
            // Standard Vertical Scroll
            m_panOffset.setY(m_panOffset.y() + delta * 60);
        }
        
        m_transformDirty = true;
        update();
        return true;
    }
 else if (event->type() == QEvent::TabletRelease || event->type() == QEvent::MouseButtonRelease) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_currentTool != Eraser && !m_activePoints.empty()) {
                Stroke s;
                s.points = m_activePoints;
                s.color = m_penColor;
                if (m_currentTool == Highlighter) {
                    s.color.setAlphaF(0.4);
                    s.width = 15.0f;
                } else {
                    s.width = 3.5f;
                }
                m_finishedStrokes.push_back(s);
                m_strokesDirty = true;
            }
            m_activePoints.clear();
            m_activeStrokeDirty = true;
        }
        saveNotes();
        update();
        return true;
    }
    return QQuickItem::event(event);
}

void NoteCanvas::saveNotes()
{
    if (m_pdfPath.isEmpty()) return;
    CanvasState state;
    {
        QMutexLocker locker(&m_mutex);
        state = { m_zoomLevel, m_panOffset, m_finishedStrokes };
    }
    PersistenceManager::save(m_pdfPath + ".cerium", state);
}

void NoteCanvas::loadNotes()
{
    if (m_pdfPath.isEmpty()) return;
    CanvasState state;
    if (PersistenceManager::load(m_pdfPath + ".cerium", state)) {
        QMutexLocker locker(&m_mutex);
        m_zoomLevel = state.zoomLevel;
        m_panOffset = state.panOffset;
        m_finishedStrokes = state.strokes;
        m_strokesDirty = true;
        m_fullReload = true;
        m_transformDirty = true;
        emit notesLoaded();
    } else {
        // Reset state for new files
        QMutexLocker locker(&m_mutex);
        m_zoomLevel = 1.0f;
        m_panOffset = QPointF(0, 0);
        m_finishedStrokes.clear();
        m_strokesDirty = true;
        m_fullReload = true;
        m_transformDirty = true;
    }
}

#include "pdf/ExportEngine.h"

void NoteCanvas::exportCurrentPdf(const QString &outputPath)
{
    if (m_pdfPath.isEmpty() || !m_pdfPath.endsWith(".pdf")) return;
    
    // Convert URL-style path to local path if needed
    QString out = outputPath;
    if (out.startsWith("file://")) out = QUrl(out).toLocalFile();
    
    CN_TRACE("Exporting flattened PDF to: %s", out.toLocal8Bit().constData());
    ExportEngine::exportPdf(m_pdfPath, out, m_finishedStrokes, 20.0f, m_renderSize);
}

void NoteCanvas::touchEvent(QTouchEvent *event)
{
    CN_TRACE("Touch Event: %d points", static_cast<int>(event->points().count()));
    
    if (event->points().count() == 1) {
        const QEventPoint &p = event->points().first();
        if (p.state() == QEventPoint::State::Updated) {
            QPointF delta = p.position() - p.lastPosition();
            m_panOffset += delta;
            m_transformDirty = true;
            update();
        }
        event->accept();
        return;
    } else if (event->points().count() == 2) {
        const QEventPoint &p1 = event->points().first();
        const QEventPoint &p2 = event->points().last();
        
        QPointF pos1 = p1.position();
        QPointF pos2 = p2.position();
        float dist = QLineF(pos1, pos2).length();
        
        if (event->touchPointStates() & Qt::TouchPointPressed) {
            m_lastTouchDist = dist;
        } else if (event->touchPointStates() & Qt::TouchPointMoved && m_lastTouchDist > 0) {
            float factor = dist / m_lastTouchDist;
            QPointF center = (pos1 + pos2) / 2.0;
            
            QPointF before = mapToCanvas(center);
            m_zoomLevel *= factor;
            QPointF after = mapToCanvas(center);
            m_panOffset += (after - before) * m_zoomLevel;
            
            m_lastTouchDist = dist;
            m_transformDirty = true;
            update();
        }
        event->accept();
        return; 
    }
    QQuickItem::touchEvent(event);
}

#include <QSGSimpleTextureNode>

QSGNode *NoteCanvas::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGTransformNode *root = static_cast<QSGTransformNode *>(oldNode);
    if (!root) {
        root = new QSGTransformNode();
        root->appendChildNode(new QSGNode()); // Index 0: PDF Layer
        root->appendChildNode(new QSGNode()); // Index 1: Static Ink Layer
        root->appendChildNode(new QSGNode()); // Index 2: Active Ink Layer
    }

    if (m_transformDirty) {
        QMatrix4x4 m;
        m.translate(m_panOffset.x(), m_panOffset.y());
        m.scale(m_zoomLevel);
        root->setMatrix(m);
        m_transformDirty = false;
    }

    if (!window()) return root;

    QSGNode *pdfLayer = root->childAtIndex(0);
    QSGNode *staticLayer = root->childAtIndex(1);
    QSGNode *activeLayer = root->childAtIndex(2);

    // 1. Update PDF Background (Multi-page)
    if (m_pdfDirty && !m_pdfPath.isEmpty() && !m_renderSize.isEmpty()) {
        CN_TRACE("Rendering multi-page document flow...");
        
        while (pdfLayer->childCount() > 0) {
            QSGNode *n = pdfLayer->childAtIndex(0);
            pdfLayer->removeChildNode(n);
            delete n;
        }

        int pageCount = m_pdfEngine.pageCount();
        float currentY = 0;
        float spacing = 20.0f; // Gap between pages

        for (int i = 0; i < pageCount; ++i) {
            QSizeF pageSize = m_pdfEngine.pageSize(i);
            if (pageSize.isEmpty()) continue;

            qreal dpr = window()->devicePixelRatio();
            float baseScale = std::min(m_renderSize.width() / pageSize.width(), 
                                       m_renderSize.height() / pageSize.height());
            
            float renderScale = baseScale * dpr;
            int targetW = pageSize.width() * baseScale;
            int targetH = pageSize.height() * baseScale;
            int offsetX = (m_renderSize.width() - targetW) / 2;

            QImage img = m_pdfEngine.renderTile(i, 0, 0, targetW * dpr, targetH * dpr, renderScale);
            if (!img.isNull()) {
                QSGTexture *texture = window()->createTextureFromImage(img);
                if (texture) {
                    texture->setFiltering(QSGTexture::Linear);
                    QSGSimpleTextureNode *node = new QSGSimpleTextureNode();
                    node->setOwnsTexture(true);
                    node->setTexture(texture);
                    node->setRect(offsetX, currentY, targetW, targetH);
                    pdfLayer->appendChildNode(node);
                }
            }
            currentY += targetH + spacing;
        }
        m_pdfDirty = false;
    }

    // 2. Update Active Stroke (Real-time feedback)
    if (m_activeStrokeDirty) {
        while (activeLayer->childCount() > 0) {
            QSGNode *n = activeLayer->childAtIndex(0);
            activeLayer->removeChildNode(n);
            delete n;
        }

        {
            QMutexLocker locker(&m_mutex);
            if (!m_activePoints.empty()) {
                QSGGeometryNode *node = new QSGGeometryNode();
                node->setFlag(QSGNode::OwnsGeometry);
                node->setFlag(QSGNode::OwnsMaterial);

                float activeWidth = m_currentTool == Highlighter ? 15.0f : 3.5f;
                int vertexCount = static_cast<int>(m_activePoints.size() * 2);
                QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
                geo->setDrawingMode(QSGGeometry::DrawTriangleStrip);
            
                updateGeometry(geo, m_activePoints, activeWidth);
                node->setGeometry(geo);

                QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
                QColor activeColor = m_penColor;
                if (m_currentTool == Highlighter) activeColor.setAlphaF(0.4);
                mat->setColor(activeColor);
                mat->setFlag(QSGMaterial::Blending, true);
                node->setMaterial(mat);

                activeLayer->appendChildNode(node);
            }
            m_activeStrokeDirty = false;
        }
    }

    // 3. Update Static Ink (Persistent strokes)
    if (m_strokesDirty) {
        QMutexLocker locker(&m_mutex);
        if (m_fullReload) {
            while (staticLayer->childCount() > 0) {
                QSGNode *n = staticLayer->childAtIndex(0);
                staticLayer->removeChildNode(n);
                delete n;
            }
            for (const auto &s : m_finishedStrokes) {
                QSGGeometryNode *node = createStrokeNode(s);
                staticLayer->appendChildNode(node);
            }
            m_fullReload = false;
        } else if (!m_finishedStrokes.empty()) {
            const auto& s = m_finishedStrokes.back();
            staticLayer->appendChildNode(createStrokeNode(s));
        }
        m_strokesDirty = false;
    }

    return root;
}
