#include "NoteCanvas.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QSGTransformNode>
#include "core/Trace.h"
#include "core/PersistenceManager.h"

NoteCanvas::NoteCanvas(QQuickItem *parent) : QQuickItem(parent), m_renderSize(0, 0), m_penColor(Qt::white), m_currentTool(Pen), m_zoomLevel(1.0)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    setAcceptTouchEvents(true);
}

NoteCanvas::~NoteCanvas() {}

QSGGeometryNode* NoteCanvas::createStrokeNode(const Stroke& s)
{
    QSGGeometryNode *node = new QSGGeometryNode();
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);

    QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), s.points.size());
    geo->setDrawingMode(QSGGeometry::DrawLineStrip);
    geo->setLineWidth(s.width);
    
    QSGGeometry::Point2D *v = geo->vertexDataAsPoint2D();
    for (size_t i = 0; i < s.points.size(); ++i) {
        v[i].set(s.points[i].x, s.points[i].y);
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

void NoteCanvas::updateGeometry(QSGGeometry *geometry, const std::vector<InkPoint>& points)
{
    if (geometry->vertexCount() != (int)points.size())
        geometry->allocate(points.size());

    QSGGeometry::Point2D *v = geometry->vertexDataAsPoint2D();
    for (size_t i = 0; i < points.size(); ++i) {
        v[i].set(points[i].x, points[i].y);
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
    if (checkFile.exists() && checkFile.isFile()) {
        CN_TRACE("Loading PDF: %s", path.toLocal8Bit().constData());
        m_pdfEngine.loadDocument(path.toStdString());
    } else {
        CN_TRACE("PDF file does not exist or is not a file: %s", path.toLocal8Bit().constData());
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
        if (event->type() == QEvent::MouseButtonPress) {
            m_lastMousePos = static_cast<QMouseEvent*>(event)->position();
        }
        m_activePoints.clear();
        m_activeStrokeDirty = true;
        update();
        return true;
    } else if (event->type() == QEvent::TabletMove || event->type() == QEvent::MouseMove) {
        QPointF pos;
        float pressure = 0.5f;
        if (event->type() == QEvent::TabletMove) {
            QTabletEvent *te = static_cast<QTabletEvent*>(event);
            pos = te->position();
            pressure = te->pressure();
        } else {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (!(me->buttons() & (Qt::LeftButton | Qt::MiddleButton))) return QQuickItem::event(event);
            
            // Middle Mouse Panning
            if (me->buttons() & Qt::MiddleButton) {
                QPointF delta = me->position() - m_lastMousePos;
                m_panOffset += delta;
                m_lastMousePos = me->position();
                m_transformDirty = true;
                update();
                return true;
            }
            m_lastMousePos = me->position();
            pos = me->position();
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
                m_strokesDirty = true;
                update();
            }
        } else {
            InkPoint p = { (float)canvasPos.x(), (float)canvasPos.y(), pressure, 0.0f, 0 };
            m_activePoints.push_back(p);
            m_activeStrokeDirty = true;
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
        if (m_currentTool != Eraser && !m_activePoints.empty()) {
            Stroke s;
            s.points = m_activePoints;
            s.color = m_penColor;
            if (m_currentTool == Highlighter) {
                s.color.setAlphaF(0.4);
                s.width = 15.0f;
            } else {
                s.width = 2.0f;
            }
            m_finishedStrokes.push_back(s);
            m_strokesDirty = true;
            saveNotes();
        }
        m_activePoints.clear();
        m_activeStrokeDirty = true;
        update();
        return true;
    }
    return QQuickItem::event(event);
}

void NoteCanvas::saveNotes()
{
    if (m_pdfPath.isEmpty()) return;
    CanvasState state = { m_zoomLevel, m_panOffset, m_finishedStrokes };
    PersistenceManager::save(m_pdfPath + ".cerium", state);
}

void NoteCanvas::loadNotes()
{
    if (m_pdfPath.isEmpty()) return;
    CanvasState state;
    if (PersistenceManager::load(m_pdfPath + ".cerium", state)) {
        m_zoomLevel = state.zoomLevel;
        m_panOffset = state.panOffset;
        m_finishedStrokes = state.strokes;
        m_strokesDirty = true;
        m_fullReload = true;
        m_transformDirty = true;
        emit notesLoaded();
    } else {
        // Reset state for new files
        m_zoomLevel = 1.0f;
        m_panOffset = QPointF(0, 0);
        m_finishedStrokes.clear();
        m_strokesDirty = true;
        m_fullReload = true;
        m_transformDirty = true;
    }
}

void NoteCanvas::touchEvent(QTouchEvent *event)
{
    if (event->points().count() == 2) {
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

        if (!m_activePoints.empty()) {
            QSGGeometryNode *node = new QSGGeometryNode();
            node->setFlag(QSGNode::OwnsGeometry);
            node->setFlag(QSGNode::OwnsMaterial);

            QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), m_activePoints.size());
            geo->setDrawingMode(QSGGeometry::DrawLineStrip);
            float activeWidth = m_currentTool == Highlighter ? 15.0f : 3.0f;
            geo->setLineWidth(activeWidth);
            
            QSGGeometry::Point2D *v = geo->vertexDataAsPoint2D();
            for (size_t i = 0; i < m_activePoints.size(); ++i) {
                v[i].set(m_activePoints[i].x, m_activePoints[i].y);
            }
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

    // 3. Update Static Ink (Persistent strokes)
    if (m_strokesDirty) {
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
