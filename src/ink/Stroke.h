#pragma once

#include <vector>
#include <QColor>

struct InkPoint {
    float x;
    float y;
    float pressure;
    float tilt; // Optional, can be 0.0f
    long long timestamp; // ms
};

struct Stroke {
    std::vector<InkPoint> points;
    QColor color;
    float width;
    bool isFinished = false;
};
