#pragma once
#include <QString>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

class VolumeModel {
public:
    VolumeModel() = default;
    ~VolumeModel() = default;

    vtkSmartPointer<vtkImageData> LoadDicomSeriesPureGDCM(const QString& folderPath);
};