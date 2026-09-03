#include "DicomLoadService.h"


#include <QDir>
#include <QFileInfoList>
#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <vtkImageAppend.h>
#include <vtkImageShiftScale.h>
#include <vtkImageImport.h>
#include<../model/SliceInfo.h>
#include <gdcmDataSet.h>
#include <gdcmAttribute.h>


#include <vector>

// 내부 Worker (QThread에서 실행될 작업 단위)
class InternalLoaderWorker : public QObject {
    Q_OBJECT
public:
    InternalLoaderWorker(QString path) : m_path(path) {}

public slots:
    void run() {
    QDir dir(m_path);
    //dir.setSorting(QDir::Name);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    if (fileList.isEmpty()){
        emit error("선택한 폴더에 DICOM 파일이 없습니다.");
        return;
	}

    std::vector<SliceInfo> validSlices;
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
        

        if (!slopeInterceptFound) {
            rescaleSlope = image.GetSlope();
            rescaleIntercept = image.GetIntercept();
            slopeInterceptFound = true;
        }



        unsigned long bufferLength = image.GetBufferLength();
        if (bufferLength == 0) continue;

        // GDCM으로 Z좌표(Image Position Patient) 추출 로직 필요
        // (기존 SliceInfo 구조체에 imagePositionPatientZ 변수가 있다고 가정)
        const double* origin = image.GetOrigin();
        double zPos = (origin != nullptr) ? origin[2] : 0.0;

        QByteArray pixelData;
        pixelData.resize(bufferLength);
        // 원본 픽셀 배열(Raw Pixel Data) 복사
        if (!image.GetBuffer(pixelData.data())) continue;


        SliceInfo info;
        info.filePath = utf8Path;
        info.rawPixelData = pixelData;
        info.imagePositionPatientZ = zPos;
        info.width = image.GetColumns();
        info.height = image.GetRows();
        
        const double* spacing = image.GetSpacing();
        info.pixelSpacingX = spacing ? spacing[0] : 1.0;
        info.pixelSpacingY = spacing ? spacing[1] : 1.0;

        // 버퍼 추출 및 기타 로직 (기존 코드 유지)
        validSlices.push_back(info);
    }

    if (validSlices.empty()) {
        emit errorOccurred("유효한 DICOM 슬라이스를 찾지 못했습니다.");
        return;
    }

    // ✅ 2단계: 실제 Z좌표(환자 기준 물리적 위치) 기반으로 오름차순 정렬
    std::sort(validSlices.begin(), validSlices.end(), [](const SliceInfo& a, const SliceInfo& b) {
        return a.imagePositionPatientZ < b.imagePositionPatientZ;
        });

    // ✅ 3단계: 정렬된 슬라이스들 간의 실제 물리적 Z축 간격(Spacing) 계산
    double sliceSpacingZ;
    if (validSlices.size() > 1) {
        sliceSpacingZ = std::abs(validSlices[1].imagePositionPatientZ - validSlices[0].imagePositionPatientZ);
        if (sliceSpacingZ <= 0.0) sliceSpacingZ = 1.0; // 방어 코드
    }

    auto appendFilter = vtkSmartPointer<vtkImageAppend>::New();
    appendFilter->SetAppendAxis(2);


    for (const auto& slice : validSlices) {
        // - vtkImageImport: 메모리 상에 존재하는 순수 바이트 포인터(Raw Buffer)를 VTK가 이해할 수 있는 vtkImageData 형식으로 감싸주는 역할
        auto importer = vtkSmartPointer<vtkImageImport>::New();
        importer->SetDataSpacing(slice.pixelSpacingX, slice.pixelSpacingY, sliceSpacingZ);
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

    // - vtkImageShiftScale: 볼륨 전체의 스칼라 값에 일괄적으로 곱셈(Scale)과 덧셈(Shift) 연산을 수행하는 필터
    //   공식 적용 결과: Output = (Input * Scale) + Shift  ==> 즉, 원본 픽셀 값이 정확한 Hounsfield Unit(HU)으로 변환됨
    auto shiftScaleFilter = vtkSmartPointer<vtkImageShiftScale>::New();
    shiftScaleFilter->SetInputData(appendFilter->GetOutput());
    shiftScaleFilter->SetScale(rescaleSlope);     // Rescale Slope 적용 (곱하기)
    shiftScaleFilter->SetShift(rescaleIntercept); // Rescale Intercept 적용 (더하기)
    shiftScaleFilter->SetOutputScalarTypeToShort(); // 연산 후 데이터 손실을 막기 위해 16-bit Short 유지
    shiftScaleFilter->Update();

    // 최종적으로 HU 밀도 단위로 변환 완료된 vtkImageData 렌더링 파이프라인으로 반환
    emit finished(shiftScaleFilter->GetOutput());
    }

signals:
    void finished(vtkSmartPointer<vtkImageData> imageData);
    void errorOccurred(const QString& message);

private:
    QString m_path;
};

// --- DicomLoadService 구현부 ---
DicomLoadService::DicomLoadService(QObject* parent) : QObject(parent) {}
DicomLoadService::~DicomLoadService() {}

void DicomLoadService::loadAsync(const QString& folderPath) {
    QThread* thread = new QThread(this);
    InternalLoaderWorker* worker = new InternalLoaderWorker(folderPath);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &InternalLoaderWorker::run);
    connect(worker, &InternalLoaderWorker::finished, this, [=](vtkSmartPointer<vtkImageData> data) {
        emit finished(data);
        thread->quit();
        });
    connect(worker, &InternalLoaderWorker::errorOccurred, this, [=](QString msg) {
        emit error(msg);
        thread->quit();
        });

    // 스레드 종료 시 메모리 자동 정리
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    thread->start();
}

// moc 빌드를 위해 포함 (Qt 빌드 시스템 설정에 따라 생략 가능)
#include "DicomLoadService.moc"