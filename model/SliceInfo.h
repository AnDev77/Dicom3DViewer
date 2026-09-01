#pragma once
#include <string>
#include <QByteArray>

struct SliceInfo {
    std::string filePath;
    QByteArray rawPixelData;
    int width;
    int height;
    double slope;
    double intercept;
    int bitsAllocated;
    int pixelRepresentation;
    int samplesPerPixel;
};
