#include "VolumeModel.h"
#include "SliceInfo.h"

#include <QDir>
#include <QFileInfoList>
#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <vtkImageAppend.h>
#include <vector>

vtkSmartPointer<vtkImageData> VolumeModel::LoadDicomSeriesPureGDCM(const QString& folderPath) {
    QDir dir(folderPath);
    dir.setSorting(QDir::Name);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    if (fileList.isEmpty()) return nullptr;

    std::vector<SliceInfo> validSlices;
    double sliceSpacingZ = 2.0;

    for (const QFileInfo& fileInfo : fileList) {
        std::string utf8Path = fileInfo.absoluteFilePath().toUtf8().constData();

        gdcm::ImageReader reader;
        reader.SetFileName(utf8Path.c_str());
        if (!reader.Read()) continue;

        const gdcm::Image& image = reader.GetImage();
        const double* spacing = image.GetSpacing();
        if (spacing && spacing[2] > 0.0) {
            sliceSpacingZ = spacing[2];
        }

        unsigned long bufferLength = image.GetBufferLength();
        if (bufferLength == 0) continue;

        QByteArray pixelData;
        pixelData.resize(bufferLength);
        if (!image.GetBuffer(pixelData.data())) continue;

        SliceInfo info;
        info.filePath = utf8Path;
        info.rawPixelData = pixelData;
        info.width = image.GetDimension(0);
        info.height = image.GetDimension(1);

        validSlices.push_back(info);
    }

    if (validSlices.empty()) return nullptr;

    auto appender = vtkSmartPointer<vtkImageAppend>::New();
    appender->SetAppendAxis(2);

    for (const auto& slice : validSlices) {
        auto singleSlice = vtkSmartPointer<vtkImageData>::New();
        singleSlice->SetDimensions(slice.width, slice.height, 1);
        singleSlice->AllocateScalars(VTK_SHORT, 1);

        std::memcpy(singleSlice->GetScalarPointer(),
            slice.rawPixelData.constData(),
            slice.rawPixelData.size());

        appender->AddInputData(singleSlice);
    }

    appender->Update();
    vtkSmartPointer<vtkImageData> volumeData = appender->GetOutput();

    volumeData->SetSpacing(1.0, 1.0, sliceSpacingZ);
    short* ptr = static_cast<short*>(volumeData->GetScalarPointer());
    vtkIdType numPts = volumeData->GetNumberOfPoints();
    for (vtkIdType i = 0; i < numPts; ++i) {
        ptr[i] = ptr[i] - 1024; // DICOM Intercept º¸Á¤
    }

    volumeData->Modified();
    return volumeData;
}