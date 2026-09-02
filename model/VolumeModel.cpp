#include "VolumeModel.h"
#include <QDir>
#include <QFileInfoList>
#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <vtkImageAppend.h>
#include <vtkImageShiftScale.h>
#include <vtkImageImport.h>
#include<SliceInfo.h>

#include <vector>

vtkSmartPointer<vtkImageData> VolumeModel::LoadDicomSeriesPureGDCM(const QString& folderPath) {
    QDir dir(folderPath);
    dir.setSorting(QDir::Name);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    if (fileList.isEmpty()) return nullptr;

    std::vector<SliceInfo> validSlices;
    double sliceSpacingZ = 2.0;
    double rescaleSlope = 1.0;
    double rescaleIntercept = 0.0;
    bool slopeInterceptFound = false;

    // 1단계: 각 DICOM 파일을 순회하며 메타데이터(간격, Slope, Intercept) 및 픽셀 버퍼 추출
    for (const QFileInfo& fileInfo : fileList) {
        std::string utf8Path = fileInfo.absoluteFilePath().toUtf8().constData();
        gdcm::ImageReader reader;
        reader.SetFileName(utf8Path.c_str());
        if (!reader.Read()) continue;

        const gdcm::Image& image = reader.GetImage();
        const double* spacing = image.GetSpacing();
        if (spacing && spacing[2] > 0.0) {
            sliceSpacingZ = spacing[2]; // Z축 방향 슬라이스 두께(Spacing) 확보
        }

        // 미디엄 아티클 핵심: 시리즈 내 공통된 Rescale Slope와 Intercept 추출 (첫 슬라이스 기준)
        if (!slopeInterceptFound) {
            rescaleSlope = image.GetSlope();
            rescaleIntercept = image.GetIntercept();
            slopeInterceptFound = true;
        }

        unsigned long bufferLength = image.GetBufferLength();
        if (bufferLength == 0) continue;

        QByteArray pixelData;
        pixelData.resize(bufferLength);
        // 원본 픽셀 배열(Raw Pixel Data) 복사
        if (!image.GetBuffer(pixelData.data())) continue;

        SliceInfo info;
        info.filePath = utf8Path;
        info.rawPixelData = pixelData;
        info.width = image.GetDimension(0);
        info.height = image.GetDimension(1);
        validSlices.push_back(info);
    }

    if (validSlices.empty()) return nullptr;

    // 2단계: 개별 2D 슬라이스들을 Z축 방향으로 결합하기 위한 vtkImageAppend 설정
    // - vtkImageAppend: 여러 개의 독립된 2D/3D 이미지 데이터 포인트를 지정한 축(SetAppendAxis) 기준으로 병합하여 하나의 3D 볼륨으로 만듦
    auto appendFilter = vtkSmartPointer<vtkImageAppend>::New();
    appendFilter->SetAppendAxis(2); // 2번 축(Z축)을 기준으로 슬라이스들을 위로 쌓아 올림

    for (const auto& slice : validSlices) {
        // - vtkImageImport: 메모리 상에 존재하는 순수 바이트 포인터(Raw Buffer)를 VTK가 이해할 수 있는 vtkImageData 형식으로 감싸주는 역할
        auto importer = vtkSmartPointer<vtkImageImport>::New();
        importer->SetDataSpacing(1.0, 1.0, sliceSpacingZ); // 픽셀 간격 및 Z축 슬라이스 두께 지정
        importer->SetDataOrigin(0.0, 0.0, 0.0);             // 3D 공간 상의 시작 좌표 (Origin)
        importer->SetWholeExtent(0, slice.width - 1, 0, slice.height - 1, 0, 0); // 이미지 크기 범위(Extent) 설정
        importer->SetDataExtent(0, slice.width - 1, 0, slice.height - 1, 0, 0); // ★ 이 부분을 추가/변경


        importer->SetDataScalarTypeToShort();              // CT 데이터 표준인 16-bit Signed Short 타입 지정
        importer->SetNumberOfScalarComponents(1);          // 흑백(Grayscale) 단일 채널 설정
        importer->SetImportVoidPointer(const_cast<char*>(slice.rawPixelData.constData())); // 메모리 주소 연동
        importer->Update();

        // 각 슬라이스를 큐에 추가하듯 Append 필터에 입력
        appendFilter->AddInputData(importer->GetOutput());
    }
    appendFilter->Update();

    // 3단계: 미디엄 아티클의 HU 공식 적용을 위한 vtkImageShiftScale 처리
    // - vtkImageShiftScale: 볼륨 전체의 스칼라 값에 일괄적으로 곱셈(Scale)과 덧셈(Shift) 연산을 수행하는 필터
    //   공식 적용 결과: Output = (Input * Scale) + Shift  ==> 즉, 원본 픽셀 값이 정확한 Hounsfield Unit(HU)으로 변환됨
    auto shiftScaleFilter = vtkSmartPointer<vtkImageShiftScale>::New();
    shiftScaleFilter->SetInputData(appendFilter->GetOutput());
    shiftScaleFilter->SetScale(rescaleSlope);     // Rescale Slope 적용 (곱하기)
    shiftScaleFilter->SetShift(rescaleIntercept); // Rescale Intercept 적용 (더하기)
    shiftScaleFilter->SetOutputScalarTypeToShort(); // 연산 후 데이터 손실을 막기 위해 16-bit Short 유지
    shiftScaleFilter->Update();

    // 최종적으로 HU 밀도 단위로 변환 완료된 vtkImageData 렌더링 파이프라인으로 반환
    return shiftScaleFilter->GetOutput();
}