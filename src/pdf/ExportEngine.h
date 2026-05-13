#pragma once
#include <QString>
#include <vector>
#include <QSizeF>
#include "ink/Stroke.h"
#ifdef ENABLE_PDF
#include <mupdf/fitz.h>
#endif

class ExportEngine {
public:
    static bool exportPdf(const QString &sourcePdf, 
                         const QString &targetPdf, 
                         const std::vector<Stroke> &strokes,
                         float verticalSpacing,
                         const QSizeF &canvasSize);
};
