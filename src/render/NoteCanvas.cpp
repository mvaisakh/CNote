#include "NoteCanvas.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QMouseEvent>

NoteCanvas::NoteCanvas(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
}

NoteCanvas::~NoteCanvas() {}

void NoteCanvas::setPdfPath(const QString& path)
{
    if (m_pdfPath == path) return;
    m_pdfPath = path;
    m_pdfEngine.loadDocument(path.toStdString());
    m_pdfDirty = true;
    emit pdfPathChanged();
    update();
}

void NoteCanvas::mousePressEvent(QMouseEvent *event)
{
    m_activePoints.clear();
    InkPoint p = { (float)event->position().x(), (float)event->position().y(), 0.5f, 0.0f, (long long)event->timestamp() };
    m_activePoints.push_back(p);
    m_activeStrokeDirty = true;
    update();
}

void NoteCanvas::mouseMoveEvent(QMouseEvent *event)
{
    InkPoint p = { (float)event->position().x(), (float)event->position().y(), 0.5f, 0.0f, (long long)event->timestamp() };
    m_activePoints.push_back(p);
    m_activeStrokeDirty = true;
    update();
}

void NoteCanvas::mouseReleaseEvent(QMouseEvent *event)
{
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
}

void NoteCanvas::touchEvent(QTouchEvent *event)
{
    // Placeholder for touch handling
    QQuickItem::touchEvent(event);
}

QSGNode *NoteCanvas::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode;
    if (!root) {
        root = new QSGNode();
    }

    // Layer 1: Static Ink
    QSGGeometryNode *staticNode = nullptr;
    if (root->childCount() > 0) {
        staticNode = static_cast<QSGGeometryNode*>(root->childAtIndex(0));
    } else {
        staticNode = new QSGGeometryNode();
        QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
        mat->setColor(Qt::white);
        staticNode->setMaterial(mat);
        staticNode->material()->setFlag(QSGMaterial::Blending, true);
        root->appendChildNode(staticNode);
    }

    // Layer 2: Active Ink
    QSGGeometryNode *activeNode = nullptr;
    if (root->childCount() > 1) {
        activeNode = static_cast<QSGGeometryNode*>(root->childAtIndex(1));
    } else {
        activeNode = new QSGGeometryNode();
        QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
        mat->setColor(Qt::red);
        activeNode->setMaterial(mat);
        activeNode->material()->setFlag(QSGMaterial::Blending, true);
        root->appendChildNode(activeNode);
    }

    if (m_activeStrokeDirty) {
        if (m_activePoints.size() > 1) {
            QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), m_activePoints.size());
            geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
            geometry->setLineWidth(2.0f);
            
            QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
            for (size_t i = 0; i < m_activePoints.size(); ++i) {
                vertices[i].set(m_activePoints[i].x, m_activePoints[i].y);
            }
            
            activeNode->setGeometry(geometry);
            activeNode->setFlag(QSGNode::OwnsGeometry);
        } else {
            activeNode->setGeometry(nullptr);
        }
        m_activeStrokeDirty = false;
    }

    if (m_strokesDirty) {
        // Simple implementation: combine all finished strokes into one geometry
        size_t totalPoints = 0;
        for (const auto& s : m_finishedStrokes) {
            if (s.points.size() > 1) totalPoints += s.points.size();
        }

        if (totalPoints > 0) {
            // Using DrawLines would require 2 points per segment, but let's use a trick or just multiple nodes.
            // For now, let's just render the last finished stroke to keep it simple.
            // In a real app, we'd use a persistent texture (Layer 2 requirement).
            const auto& lastStroke = m_finishedStrokes.back();
            QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), lastStroke.points.size());
            geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
            geometry->setLineWidth(2.0f);
            
            QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
            for (size_t i = 0; i < lastStroke.points.size(); ++i) {
                vertices[i].set(lastStroke.points[i].x, lastStroke.points[i].y);
            }
            staticNode->setGeometry(geometry);
            staticNode->setFlag(QSGNode::OwnsGeometry);
        }
        m_strokesDirty = false;
    }

    return root;
}
