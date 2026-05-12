#include "ink/InkProcessor.h"
#include <iostream>
#include <cassert>

void testSmoothing() {
    std::vector<InkPoint> raw;
    raw.push_back({0, 0, 0.5f, 0, 0});
    raw.push_back({10, 10, 0.6f, 0, 10});
    raw.push_back({20, 0, 0.5f, 0, 20});

    auto smoothed = InkProcessor::smoothPoints(raw, 4);

    std::cout << "Raw points: " << raw.size() << std::endl;
    std::cout << "Smoothed points: " << smoothed.size() << std::endl;

    assert(smoothed.size() > raw.size());
    assert(smoothed.front().x == raw.front().x);
    assert(smoothed.back().x == raw.back().x);

    std::cout << "Test passed!" << std::endl;
}

int main() {
    testSmoothing();
    return 0;
}
