#include "InkProcessor.h"
#include <cmath>

InkPoint InkProcessor::catmullRom(const InkPoint& p0, const InkPoint& p1,
                                 const InkPoint& p2, const InkPoint& p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    auto interpolate = [&](float v0, float v1, float v2, float v3) {
        return 0.5f * ((2.0f * v1) +
                       (-v0 + v2) * t +
                       (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
                       (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3);
    };

    InkPoint p;
    p.x = interpolate(p0.x, p1.x, p2.x, p3.x);
    p.y = interpolate(p0.y, p1.y, p2.y, p3.y);
    p.pressure = interpolate(p0.pressure, p1.pressure, p2.pressure, p3.pressure);
    p.tilt = interpolate(p0.tilt, p1.tilt, p2.tilt, p3.tilt);
    p.timestamp = p1.timestamp + static_cast<long long>(t * (p2.timestamp - p1.timestamp));

    return p;
}

std::vector<InkPoint> InkProcessor::smoothPoints(const std::vector<InkPoint>& rawPoints, int segmentsPerPair)
{
    if (rawPoints.size() < 2) return rawPoints;

    std::vector<InkPoint> smoothed;
    smoothed.reserve(rawPoints.size() * segmentsPerPair);

    for (size_t i = 0; i < rawPoints.size() - 1; ++i) {
        const InkPoint& p1 = rawPoints[i];
        const InkPoint& p2 = rawPoints[i + 1];
        
        // Virtual p0 and p3 for boundaries
        const InkPoint& p0 = (i == 0) ? p1 : rawPoints[i - 1];
        const InkPoint& p3 = (i + 2 >= rawPoints.size()) ? p2 : rawPoints[i + 2];

        for (int s = 0; s < segmentsPerPair; ++s) {
            float t = static_cast<float>(s) / segmentsPerPair;
            smoothed.push_back(catmullRom(p0, p1, p2, p3, t));
        }
    }

    // Add the last point
    smoothed.push_back(rawPoints.back());

    return smoothed;
}
