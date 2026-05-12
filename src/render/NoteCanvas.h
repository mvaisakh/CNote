#pragma once

#include <QQuickItem>
#include <QSGNode>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include "ink/Stroke.h"
#include "pdf/PdfEngine.h"

class NoteCanvas : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(NoteCanvas)
    Q_PROPERTY(QString pdfPath READ pdfPath WRITE setPdfPath NOTIFY pdfPathChanged)
    Q_PROPERTY(QColor penColor READ penColor WRITE setPenColor NOTIFY penColorChanged)
    Q_PROPERTY(int currentTool READ currentTool WRITE setCurrentTool NOTIFY currentToolChanged)

public:
    enum Tool { Pen, Highlighter, Eraser };
    Q_ENUM(Tool)

    NoteCanvas(QQuickItem *parent = nullptr);
    ~NoteCanvas();

    QString pdfPath() const { return m_pdfPath; }
    void setPdfPath(const QString& path);

    QColor penColor() const { return m_penColor; }
    void setPenColor(const QColor& color) { if (m_penColor != color) { m_penColor = color; emit penColorChanged(); } }

    int currentTool() const { return m_currentTool; }
    void setCurrentTool(int tool) { if (m_currentTool != tool) { m_currentTool = tool; emit currentToolChanged(); } }

signals:
    void pdfPathChanged();
    void penColorChanged();
    void currentToolChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    bool event(QEvent *event) override;
    void touchEvent(QTouchEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_pdfPath;
    PdfEngine m_pdfEngine;
    
    std::vector<Stroke> m_finishedStrokes;
    std::vector<InkPoint> m_activePoints;
    
    bool m_pdfDirty = false;
    bool m_strokesDirty = false;
    bool m_activeStrokeDirty = false;
    QSize m_renderSize;
    QColor m_penColor = Qt::white;
    int m_currentTool = Pen;

    void updateGeometry(QSGGeometry *geometry, const std::vector<InkPoint>& points);
};
