#include "LoadWorker.h"
#include "../model/SliceInfo.h"
#include <QDir>
#include <QFileInfoList>
#include <vector>
#include <algorithm>
#include <cmath>

// GDCM Includes
#include <gdcmImageReader.h>
#include <gdcmImage.h>

// VTK Includes
#include <vtkImageAppend.h>
#include <vtkImageShiftScale.h>
#include <vtkImageImport.h>

LoaderWorker::LoaderWorker(const QString& path, QObject* parent)
    : QObject(parent), m_path(path) {
}

void LoaderWorker::run() {
    QDir dir(m_path);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    if (fileList.isEmpty()) {
        emit errorOccurred("선택한 폴더에 파일이 없습니다.");
        return;
    }

    std::vector<SliceInfo> validSlices;
    double rescaleSlope = 1.0;
    double rescaleIntercept = 0.0;
    bool slopeInterceptFound = false;

    for (const QFileInfo& fileInfo : fileList) {
        std::string utf8Path = fileInfo.absoluteFilePath().toUtf8().constData();
        gdcm::ImageReader reader;
        reader.SetFileName(utf8Path.c_str());
        if (!reader.Read()) continue;

        const gdcm::Image& image = reader.GetImage();
        const double* origin = image.GetOrigin();
        double zPos = (origin != nullptr) ? origin[2] : 0.0;

        SliceInfo info;
        info.filePath = utf8Path;
        info.imagePositionPatientZ = zPos;
        info.width = image.GetColumns();
        info.height = image.GetRows();

        const double* spacing = image.GetSpacing();
        info.pixelSpacingX = (spacing && spacing[0] > 0.0) ? spacing[0] : 1.0;
        info.pixelSpacingY = (spacing && spacing[1] > 0.0) ? spacing[1] : 1.0;

        if (!slopeInterceptFound) {
            rescaleSlope = image.GetSlope();
            rescaleIntercept = image.GetIntercept();
            slopeInterceptFound = true;
        }

        unsigned long bufferLength = image.GetBufferLength();
        if (bufferLength == 0) continue;

        info.rawPixelData.resize(bufferLength);
        if (!image.GetBuffer(info.rawPixelData.data())) continue;

        validSlices.push_back(info);
    }

    if (validSlices.empty()) {
        emit errorOccurred("유효한 DICOM 슬라이스를 찾지 못했습니다.");
        return;
    }

    // Z축 정렬
    std::sort(validSlices.begin(), validSlices.end(), [](const SliceInfo& a, const SliceInfo& b) {
        return a.imagePositionPatientZ < b.imagePositionPatientZ;
        });

    double sliceSpacingZ = 1.0;
    if (validSlices.size() > 1) {
        sliceSpacingZ = std::abs(validSlices[1].imagePositionPatientZ - validSlices[0].imagePositionPatientZ);
        if (sliceSpacingZ <= 0.0) sliceSpacingZ = 1.0;
    }

    auto appendFilter = vtkSmartPointer<vtkImageAppend>::New();
    appendFilter->SetAppendAxis(2);

    for (const auto& slice : validSlices) {
        auto importer = vtkSmartPointer<vtkImageImport>::New();
        importer->SetDataSpacing(slice.pixelSpacingX, slice.pixelSpacingY, sliceSpacingZ);
        importer->SetDataOrigin(0.0, 0.0, 0.0);
        importer->SetWholeExtent(0, slice.width - 1, 0, slice.height - 1, 0, 0);
        importer->SetDataExtent(0, slice.width - 1, 0, slice.height - 1, 0, 0);
        importer->SetDataScalarTypeToShort();
        importer->SetNumberOfScalarComponents(1);
        importer->SetImportVoidPointer(const_cast<char*>(slice.rawPixelData.constData()));
        importer->Update();
        appendFilter->AddInputData(importer->GetOutput());
    }
    appendFilter->Update();

    auto shiftScaleFilter = vtkSmartPointer<vtkImageShiftScale>::New();
    shiftScaleFilter->SetInputData(appendFilter->GetOutput());
    shiftScaleFilter->SetScale(rescaleSlope);
    shiftScaleFilter->SetShift(rescaleIntercept);
    shiftScaleFilter->SetOutputScalarTypeToShort();
    shiftScaleFilter->Update();

    emit finished(shiftScaleFilter->GetOutput());
}