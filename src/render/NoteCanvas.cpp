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

#include <QFileInfo>
#include <QDebug>

void NoteCanvas::setPdfPath(const QString& path)
{
    if (m_pdfPath == path) return;
    m_pdfPath = path;
    
    QFileInfo checkFile(path);
    if (checkFile.exists() && checkFile.isFile()) {
        qDebug() << "NoteCanvas: Loading PDF:" << path;
        m_pdfEngine.loadDocument(path.toStdString());
    } else {
        qWarning() << "NoteCanvas: PDF file does not exist or is not a file:" << path;
    }
    
    m_pdfDirty = true;
    emit pdfPathChanged();
    update();
}

void NoteCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->source() == Qt::MouseEventNotSynthesized) {
        m_activePoints.clear();
        InkPoint p = { (float)event->position().x(), (float)event->position().y(), 0.5f, 0.0f, (long long)event->timestamp() };
        m_activePoints.push_back(p);
        m_activeStrokeDirty = true;
        update();
    }
}

void NoteCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (event->source() == Qt::MouseEventNotSynthesized) {
        InkPoint p = { (float)event->position().x(), (float)event->position().y(), 0.5f, 0.0f, (long long)event->timestamp() };
        m_activePoints.push_back(p);
        m_activeStrokeDirty = true;
        update();
    }
}

void NoteCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->source() == Qt::MouseEventNotSynthesized) {
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
}

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
    printf("NoteCanvas: updatePaintNode start\n");
    QSGNode *root = oldNode;
    if (!root) {
        printf("NoteCanvas: Creating new root node\n");
        root = new QSGNode();
    }

    if (!window()) {
        printf("NoteCanvas: No window, aborting\n");
        return root;
    }

    // Layer 0: PDF Background
    QSGSimpleTextureNode *pdfNode = nullptr;
    if (root->childCount() > 0) {
        pdfNode = static_cast<QSGSimpleTextureNode*>(root->childAtIndex(0));
    } else {
        printf("NoteCanvas: Creating PDF node\n");
        pdfNode = new QSGSimpleTextureNode();
        root->appendChildNode(pdfNode);
    }

    // Layer 1: Static Ink
    QSGGeometryNode *staticNode = nullptr;
    if (root->childCount() > 1) {
        staticNode = static_cast<QSGGeometryNode*>(root->childAtIndex(1));
    } else {
        printf("NoteCanvas: Creating Static Ink node\n");
        staticNode = new QSGGeometryNode();
        QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
        mat->setColor(Qt::white);
        staticNode->setMaterial(mat);
        staticNode->material()->setFlag(QSGMaterial::Blending, true);
        root->appendChildNode(staticNode);
    }

    // Layer 2: Active Ink
    QSGGeometryNode *activeNode = nullptr;
    if (root->childCount() > 2) {
        activeNode = static_cast<QSGGeometryNode*>(root->childAtIndex(2));
    } else {
        printf("NoteCanvas: Creating Active Ink node\n");
        activeNode = new QSGGeometryNode();
        QSGFlatColorMaterial *mat = new QSGFlatColorMaterial();
        mat->setColor(Qt::red);
        activeNode->setMaterial(mat);
        activeNode->material()->setFlag(QSGMaterial::Blending, true);
        root->appendChildNode(activeNode);
    }

    if (m_pdfDirty && !m_pdfPath.isEmpty()) {
        printf("NoteCanvas: Rendering PDF tile...\n");
        QImage img = m_pdfEngine.renderTile(0, 0, 0, (int)width(), (int)height(), 1.0f);
        if (!img.isNull()) {
            printf("NoteCanvas: Uploading texture %dx%d...\n", img.width(), img.height());
            QSGTexture *texture = window()->createTextureFromImage(img);
            if (texture) {
                pdfNode->setOwnsTexture(true);
                pdfNode->setTexture(texture);
                pdfNode->setRect(0, 0, width(), height());
                printf("NoteCanvas: Texture uploaded\n");
            } else {
                printf("NoteCanvas: FAILED to create texture\n");
            }
        }
        m_pdfDirty = false;
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
        if (!m_finishedStrokes.empty()) {
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
