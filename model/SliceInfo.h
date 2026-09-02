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
	double imagePositionPatientZ; // 실제 Z좌표 (환자 기준 물리적 위치)
	double pixelSpacingX;
	double pixelSpacingY;
   

};
