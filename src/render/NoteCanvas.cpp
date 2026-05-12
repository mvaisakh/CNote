#include "NoteCanvas.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QMouseEvent>

NoteCanvas::NoteCanvas(QQuickItem *parent) : QQuickItem(parent), m_renderSize(0, 0), m_penColor(Qt::white), m_currentTool(Pen)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
}

NoteCanvas::~NoteCanvas() {}

void NoteCanvas::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_renderSize = newGeometry.size().toSize();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

#include <QFileInfo>
#include <QDebug>
#include "Trace.h"

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
    update();
}

void NoteCanvas::mousePressEvent(QMouseEvent *) {}
void NoteCanvas::mouseMoveEvent(QMouseEvent *) {}
void NoteCanvas::mouseReleaseEvent(QMouseEvent *) {}

#include <QTabletEvent>

bool NoteCanvas::event(QEvent *event)
{
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::MouseButtonPress) {
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
            if (!(me->buttons() & Qt::LeftButton)) return QQuickItem::event(event);
            pos = me->position();
        }
        
        if (m_currentTool == Eraser) {
            bool changed = false;
            auto it = m_finishedStrokes.begin();
            while (it != m_finishedStrokes.end()) {
                bool hit = false;
                for (const auto& p : it->points) {
                    float dx = p.x - pos.x();
                    float dy = p.y - pos.y();
                    if (dx*dx + dy*dy < 20*20) { // 20px eraser radius
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
            InkPoint p = { (float)pos.x(), (float)pos.y(), pressure, 0.0f, 0 };
            m_activePoints.push_back(p);
            m_activeStrokeDirty = true;
            update();
        }
        return true;
    } else if (event->type() == QEvent::TabletRelease || event->type() == QEvent::MouseButtonRelease) {
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
        }
        m_activePoints.clear();
        m_activeStrokeDirty = true;
        update();
        return true;
    }
    return QQuickItem::event(event);
}

void NoteCanvas::touchEvent(QTouchEvent *event)
{
    QQuickItem::touchEvent(event);
}

#include <QSGSimpleTextureNode>

QSGNode *NoteCanvas::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode;
    
    // Safety: If the root exists but doesn't have our 3-layer structure, flush it.
    if (root && root->childCount() != 3) {
        CN_TRACE("Invalid layer structure detected, flushing...");
        while (root->childCount() > 0) {
            QSGNode *n = root->childAtIndex(0);
            root->removeChildNode(n);
            delete n;
        }
        delete root;
        root = nullptr;
    }

    if (!root) {
        CN_TRACE("Initializing Layer Stack (PDF, Static, Active)");
        root = new QSGNode();
        root->appendChildNode(new QSGNode()); // Index 0: PDF Layer
        root->appendChildNode(new QSGNode()); // Index 1: Static Ink Layer
        root->appendChildNode(new QSGNode()); // Index 2: Active Ink Layer
    }

    if (!window()) return root;

    QSGNode *pdfLayer = root->childAtIndex(0);
    QSGNode *staticLayer = root->childAtIndex(1);
    QSGNode *activeLayer = root->childAtIndex(2);

    // 1. Update PDF Background
    if (m_pdfDirty && !m_pdfPath.isEmpty() && !m_renderSize.isEmpty()) {
        CN_TRACE("Rendering PDF at Actual Size...");
        
        QSizeF pageSize = m_pdfEngine.pageSize(0);
        if (!pageSize.isEmpty()) {
            qreal dpr = window()->devicePixelRatio();
            // Actual Size: 1 point (1/72") -> 1.33 pixels (1/96")
            float actualScale = 1.333f; 
            
            float renderScale = actualScale * dpr;
            int targetW = pageSize.width() * actualScale;
            int targetH = pageSize.height() * actualScale;
            
            // Center in the current view
            int offsetX = (m_renderSize.width() - targetW) / 2;
            int offsetY = (m_renderSize.height() - targetH) / 2;

            QImage img = m_pdfEngine.renderTile(0, 0, 0, targetW * dpr, targetH * dpr, renderScale);
            if (!img.isNull()) {
                QSGTexture *texture = window()->createTextureFromImage(img);
                if (texture) {
                    texture->setFiltering(QSGTexture::Linear);
                    while (pdfLayer->childCount() > 0) {
                        QSGNode *n = pdfLayer->childAtIndex(0);
                        pdfLayer->removeChildNode(n);
                        delete n;
                    }
                    
                    QSGSimpleTextureNode *node = new QSGSimpleTextureNode();
                    node->setOwnsTexture(true);
                    node->setTexture(texture);
                    node->setRect(offsetX, offsetY, targetW, targetH);
                    pdfLayer->appendChildNode(node);
                }
            }
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
        if (!m_finishedStrokes.empty()) {
            const auto& s = m_finishedStrokes.back();
            CN_TRACE("Committing stroke: color=%s, width=%.1f", s.color.name().toLocal8Bit().constData(), s.width);
            
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

            staticLayer->appendChildNode(node);
        }
        m_strokesDirty = false;
    }

    return root;
}
