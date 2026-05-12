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

public:
    NoteCanvas(QQuickItem *parent = nullptr);
    ~NoteCanvas();

    QString pdfPath() const { return m_pdfPath; }
    void setPdfPath(const QString& path);

signals:
    void pdfPathChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
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

    void updateGeometry(QSGGeometry *geometry, const std::vector<InkPoint>& points);
};
