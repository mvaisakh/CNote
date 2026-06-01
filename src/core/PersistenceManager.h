#pragma once

#include <QString>
#include <vector>
#include <QPointF>
#include "ink/Stroke.h"

struct CanvasState {
    float zoomLevel;
    QPointF panOffset;
    std::vector<Stroke> strokes;
    int pageCount = 1;
};

class PersistenceManager
{
public:
    static bool save(const QString &path, const CanvasState &state);
    static bool load(const QString &path, CanvasState &state);
};
