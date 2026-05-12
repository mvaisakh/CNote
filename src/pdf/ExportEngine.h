#pragma once

#include <QString>
#include <vector>
#include "ink/Stroke.h"
#include <mupdf/fitz.h>

class ExportEngine {
public:
    static bool exportPdf(const QString &sourcePdf, 
                         const QString &targetPdf, 
                         const std::vector<Stroke> &strokes,
                         float verticalSpacing);
};
