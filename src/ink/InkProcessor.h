#pragma once

#include "Stroke.h"
#include <vector>

class InkProcessor {
public:
    // Smoothens a sequence of points using Catmull-Rom spline interpolation
    static std::vector<InkPoint> smoothPoints(const std::vector<InkPoint>& rawPoints, int segmentsPerPair = 4);

private:
    static InkPoint catmullRom(const InkPoint& p0, const InkPoint& p1,
                               const InkPoint& p2, const InkPoint& p3, float t);
};
