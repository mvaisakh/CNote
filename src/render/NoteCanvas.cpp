#include "NoteCanvas.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QMouseEvent>

NoteCanvas::NoteCanvas(QQuickItem *parent) : QQuickItem(parent), m_renderSize(0, 0)
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
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletMove || event->type() == QEvent::TabletRelease) {
        QTabletEvent *tablet = static_cast<QTabletEvent *>(event);
        switch (event->type()) {
        case QEvent::TabletPress:
            m_activePoints.clear();
            [[fallthrough]];
        case QEvent::TabletMove: {
            InkPoint p = { 
                (float)tablet->position().x(), 
                (float)tablet->position().y(), 
                (float)tablet->pressure(), 
                (float)tablet->xTilt(), 
                (long long)tablet->timestamp() 
            };
            m_activePoints.push_back(p);
            m_activeStrokeDirty = true;
            update();
            break;
        }
        case QEvent::TabletRelease:
            if (!m_activePoints.empty()) {
                Stroke s;
                s.points = m_activePoints;
                s.color = Qt::white;
                s.width = 2.0f;
                m_finishedStrokes.push_back(s);
            }
            m_activePoints.clear();
            m_activeStrokeDirty = true;
            m_strokesDirty = true;
            update();
            break;
        default:
            break;
        }
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
    CN_TRACE("updatePaintNode start");
    QSGNode *root = oldNode;
    if (!root) {
        CN_TRACE("Creating new root node");
        root = new QSGNode();
    }

    if (!window()) {
        CN_TRACE("No window, aborting");
        return root;
    }

    // Layer 0: PDF Background
    QSGSimpleTextureNode *pdfNode = nullptr;
    if (root->childCount() > 0) {
        pdfNode = static_cast<QSGSimpleTextureNode*>(root->childAtIndex(0));
    }

    // Layer 1: Static Ink
    QSGGeometryNode *staticNode = nullptr;
    QSGGeometryNode *activeNode = nullptr;
    for (int i = 0; i < root->childCount(); ++i) {
        if (root->childAtIndex(i)->type() == QSGNode::GeometryNodeType) {
            staticNode = static_cast<QSGGeometryNode*>(root->childAtIndex(i));
            // Assuming the first GeometryNode is static, second is active
            if (i + 1 < root->childCount() && root->childAtIndex(i+1)->type() == QSGNode::GeometryNodeType) {
                activeNode = static_cast<QSGGeometryNode*>(root->childAtIndex(i+1));
            }
            break;
        }
    }

    if (!staticNode) {
        CN_TRACE("Creating Static Ink node");
        staticNode = new QSGGeometryNode();
        staticNode->setFlag(QSGNode::OwnsGeometry);
        staticNode->setFlag(QSGNode::OwnsMaterial);
        QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geo->setDrawingMode(QSGGeometry::DrawLineStrip);
        geo->setLineWidth(3.0f);
        staticNode->setGeometry(geo);
        QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
        mat->setColor(Qt::white);
        mat->setFlag(QSGMaterial::Blending, true);
        staticNode->setMaterial(mat);
        root->appendChildNode(staticNode);
    }

    if (!activeNode) {
        CN_TRACE("Creating Active Ink node");
        activeNode = new QSGGeometryNode();
        activeNode->setFlag(QSGNode::OwnsGeometry);
        activeNode->setFlag(QSGNode::OwnsMaterial);
        QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geo->setDrawingMode(QSGGeometry::DrawLineStrip);
        geo->setLineWidth(3.0f);
        activeNode->setGeometry(geo);
        QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
        mat->setColor(Qt::red); // Red for active stroke
        mat->setFlag(QSGMaterial::Blending, true);
        activeNode->setMaterial(mat);
        root->appendChildNode(activeNode);
    }

    if (m_pdfDirty && !m_pdfPath.isEmpty() && !m_renderSize.isEmpty()) {
        CN_TRACE("Rendering PDF tile...");
        QImage img = m_pdfEngine.renderTile(0, 0, 0, m_renderSize.width(), m_renderSize.height(), 1.0f);
        if (!img.isNull()) {
            QSGTexture *texture = window()->createTextureFromImage(img);
            if (texture) {
                if (!pdfNode) {
                    pdfNode = new QSGSimpleTextureNode();
                    root->prependChildNode(pdfNode);
                }
                pdfNode->setOwnsTexture(true);
                pdfNode->setTexture(texture);
                pdfNode->setRect(0, 0, m_renderSize.width(), m_renderSize.height());
            }
        }
        m_pdfDirty = false;
    }

    if (m_activeStrokeDirty) {
        if (!m_activePoints.empty()) {
            QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), m_activePoints.size());
            geo->setDrawingMode(QSGGeometry::DrawLineStrip);
            geo->setLineWidth(3.0f);
            QSGGeometry::Point2D *v = geo->vertexDataAsPoint2D();
            for (size_t i = 0; i < m_activePoints.size(); ++i) {
                v[i].set(m_activePoints[i].x, m_activePoints[i].y);
            }
            activeNode->setGeometry(geo);
            CN_TRACE("Active stroke: %zu points", m_activePoints.size());
        } else {
            activeNode->setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0));
        }
        m_activeStrokeDirty = false;
    }

    if (m_strokesDirty) {
        // Simple implementation: Combine all strokes into one geometry for now
        size_t totalPoints = 0;
        for (const auto& s : m_finishedStrokes) totalPoints += s.points.size();
        
        if (totalPoints > 0) {
            // Note: DrawLineStrip won't work well for disconnected strokes in one geometry.
            // For this preview, we'll just show the last stroke to keep it simple and stable.
            const auto& s = m_finishedStrokes.back();
            QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), s.points.size());
            geo->setDrawingMode(QSGGeometry::DrawLineStrip);
            geo->setLineWidth(3.0f);
            QSGGeometry::Point2D *v = geo->vertexDataAsPoint2D();
            for (size_t i = 0; i < s.points.size(); ++i) {
                v[i].set(s.points[i].x, s.points[i].y);
            }
            staticNode->setGeometry(geo);
            CN_TRACE("Static ink updated: %zu strokes", m_finishedStrokes.size());
        }
        m_strokesDirty = false;
    }

    return root;
}
