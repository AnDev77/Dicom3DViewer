#include "DicomVolumeViewer.h"
#include <QVBoxLayout>
#include <vtkSmartVolumeMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>

#include <vtkImageReslice.h> // ★ 2D 단면 추출을 위한 필수 헤더
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkImageActor.h>
#include<vtkMatrix4x4.h>


//브러시 도입을 위한 헤더 추가   
#include <vtkImageBlend.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>

#include <vtkWindowLevelLookupTable.h>


DicomVolumeViewer::DicomVolumeViewer(QWidget* parent) : QMainWindow(parent) {
    this->setWindowTitle("DICOM Series to 3D Volume Viewer");
    this->resize(1024, 768);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    btnOpenDicom = new QPushButton("DICOM open folder", this);
    btnOpenDicom -> setFixedHeight(40);
    
    m_showButton = new QPushButton("Show Volume", this);
    
    m_viewComboBox = new QComboBox(this);

    m_viewComboBox->addItem("Axial");
    m_viewComboBox->addItem("Coronal");
    m_viewComboBox->addItem("Sagittal");
    vtkWidget = new QVTKOpenGLNativeWidget(this);


    layout->addWidget(btnOpenDicom);
    
    layout->addWidget(m_viewComboBox);
	layout->addWidget(m_showButton);
    layout->addWidget(vtkWidget);
    
this->setCentralWidget(centralWidget);

    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWidget->setRenderWindow(renderWindow);

    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.2, 0.2, 0.2);
    renderWindow->AddRenderer(renderer);

}

QPushButton* DicomVolumeViewer::getOpenButton() const { return btnOpenDicom; }
QPushButton* DicomVolumeViewer::getShowButton() const { return m_showButton; }
QComboBox* DicomVolumeViewer::getComboBox() const {
    return m_viewComboBox;
}
QString DicomVolumeViewer::getSelectedViewMode() const {
    return m_viewComboBox->currentText();
}

void DicomVolumeViewer::RenderVolume(vtkSmartPointer<vtkImageData> imageData) {
    if (!imageData) return;

    auto volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    volumeMapper->SetInputData(imageData);
    // ==========================================
    // 1. 투명도 전달 함수 (Opacity Transfer Function)
    // ==========================================
    // vtkPiecewiseFunction: 특정 Hounsfield Unit(HU) 밀도 값에 투명도(Opacity)를 매핑하는 함수
    // - 첫 번째 인자 (double): HU 밀도 값 (예: -1000은 공기, 0은 물, +300 이상은 뼈)
    // - 두 번째 인자 (double): 투명도 (0.0 = 완전 투명, 1.0 = 완전 불투명)
    auto opacityFunc = vtkSmartPointer<vtkPiecewiseFunction>::New();

    opacityFunc->AddPoint(-1000.0, 0.0);  // 공기 영역 (-1000 HU): 완전히 투명하게 처리하여 배경 제거
    opacityFunc->AddPoint(-400.0, 0.0);  // 저밀도 지방 조직 (-400 HU): 완전 투명
    opacityFunc->AddPoint(-100.0, 0.0);  // 일반 연부 조직 및 근육 영역: 완전 투명하게 지움
    opacityFunc->AddPoint(100.0, 0.0);  // 연부 조직 경계선 부근: 투명하게 유지
    opacityFunc->AddPoint(200.0, 0.0);  // 뼈가 시작되는 지점 직전까지는 모두 투명 처리
    opacityFunc->AddPoint(300.0, 0.3);  // 해면골(Spongy bone) 밀도 시작점: 서서히 형태가 나타나도록 설정 (투명도 0.3)
    opacityFunc->AddPoint(800.0, 0.85); // 단단한 피질골(Compact bone): 선명하게 드러나도록 불투명도 높임 (0.85)
    opacityFunc->AddPoint(1500.0, 1.0);  // 초고밀도 뼈 및 임플란트 영역: 완전 불투명 처리 (1.0)

    //// ==========================================
    //// 2. 색상 전달 함수 (Color Transfer Function)
    //// ==========================================
    //// vtkColorTransferFunction: 밀도 값에 따라 3D 렌더링 색상(RGB)을 매핑하는 함수
    //// - 첫 번째 인자 (double): HU 밀도 값
    //// - 두 번째~네 번째 인자 (double, double, double): Red, Green, Blue 색상 비율 (0.0 ~ 1.0 범위)
    auto colorFunc = vtkSmartPointer<vtkColorTransferFunction>::New();

    colorFunc->AddRGBPoint(-1000.0, 0.0, 0.0, 0.0);   // 공기 영역: 검은색 (투명도에 의해 보이지 않음)
    colorFunc->AddRGBPoint(-100.0, 0.0, 0.0, 0.0);   // 연부 조직 영역: 검은색 처리
    colorFunc->AddRGBPoint(200.0, 0.9, 0.85, 0.8);  // 뼈가 시작되는 시점: 은은한 상아색(Ivory) 부여
    colorFunc->AddRGBPoint(800.0, 0.95, 0.95, 0.95);// 일반 뼈 영역: 밝은 회색톤
    colorFunc->AddRGBPoint(1500.0, 1.0, 1.0, 1.0);   // 고밀도 뼈 영역: 깨끗한 순백색(White)으로 강조




    // ==========================================
    // 3. 볼륨 속성 및 렌더러 설정
    // ==========================================
    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetColor(colorFunc);
    volumeProperty->SetScalarOpacity(opacityFunc);
    volumeProperty->SetShade(true);                 // 조명 효과(Shading) 활성화로 뼈의 입체감과 굴곡을 극대화
    volumeProperty->SetInterpolationTypeToLinear(); // 선형 보간을 적용
    volumeProperty->SetAmbient(0.3);                // 주변광 세기 설정
    volumeProperty->SetDiffuse(0.6);                // 확산광 세기 설정
    volumeProperty->SetSpecular(0.4);               // 반사광(하이라이트) 세기 설정

    auto volume = vtkSmartPointer<vtkVolume>::New();

    volumeMapper->SetSampleDistance(volumeMapper->GetSampleDistance() * 0.5);
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    renderer->RemoveAllViewProps();
    renderer->AddVolume(volume);
    renderer->ResetCamera();
    renderWindow->Render();
};

void DicomVolumeViewer::RenderSlice(vtkSmartPointer<vtkImageData> imageData, QString viewMode) {
    if (!imageData) return;
    auto reslice = vtkSmartPointer<vtkImageReslice>::New();
    

    // -------------------------------------------------------------
    // [Step 1 테스트용]: 가짜 마스크 데이터 생성 (화면 중앙에 붉은 점 찍기)
    // 실제 구현 시에는 이 부분을 MaskVolumeModel에서 받아오게 됩니다.
    // -------------------------------------------------------------
    auto maskData = vtkSmartPointer<vtkImageData>::New();
    maskData->SetDimensions(imageData->GetDimensions());
    maskData->SetSpacing(imageData->GetSpacing());
    maskData->SetOrigin(imageData->GetOrigin());
    maskData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

    // 전체를 0(배경)으로 초기화
    memset(maskData->GetScalarPointer(), 0, maskData->GetNumberOfPoints() * sizeof(unsigned char));

    // 정중앙 좌표 계산하여 반경 10픽셀 크기의 구(Sphere) 라벨(1) 칠하기
    int dims[3];
    maskData->GetDimensions(dims);
    int cx = dims[0] / 2, cy = dims[1] / 2, cz = dims[2] / 2;
    int radius = 10;
    for (int z = cz - radius; z <= cz + radius; ++z) {
        for (int y = cy - radius; y <= cy + radius; ++y) {
            for (int x = cx - radius; x <= cx + radius; ++x) {
                if ((x - cx) * (x - cx) + (y - cy) * (y - cy) + (z - cz) * (z - cz) <= radius * radius) {
                    unsigned char* pixel = static_cast<unsigned char*>(maskData->GetScalarPointer(x, y, z));
                    if (pixel) *pixel = 1; // 1번 라벨 부여
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////



    double center[3];
    imageData->GetCenter(center);

    reslice->SetInputData(imageData);
    reslice->SetOutputDimensionality(2); // 출력을 2D로 고정

    auto resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
    resliceAxes->Identity();

    // 뷰 모드에 따른 단면 지정 (행렬을 직접 안 건드리고 간단하게 조절 가능)
    if (viewMode.contains("Axial")) {
        // Z축 고정 단면
        resliceAxes->SetElement(0, 0, 1); resliceAxes->SetElement(0, 1, 0); resliceAxes->SetElement(0, 2, 0);
        resliceAxes->SetElement(1, 0, 0); resliceAxes->SetElement(1, 1, 1); resliceAxes->SetElement(1, 2, 0);
        resliceAxes->SetElement(2, 0, 0); resliceAxes->SetElement(2, 1, 0); resliceAxes->SetElement(2, 2, 1);
    }
    else if (viewMode.contains("Coronal")) {
        // Y축 고정 단면
        resliceAxes->SetElement(0, 0, 1); resliceAxes->SetElement(0, 1, 0); resliceAxes->SetElement(0, 2, 0);
        resliceAxes->SetElement(1, 0, 0); resliceAxes->SetElement(1, 1, 0); resliceAxes->SetElement(1, 2, 1);
        resliceAxes->SetElement(2, 0, 0); resliceAxes->SetElement(2, 1, 1); resliceAxes->SetElement(2, 2, 0);
    }
    else if (viewMode.contains("Sagittal")) {
        // X축 고정 단면
        resliceAxes->SetElement(0, 0, 0); resliceAxes->SetElement(0, 1, 0); resliceAxes->SetElement(0, 2, 1);
        resliceAxes->SetElement(1, 0, 0); resliceAxes->SetElement(1, 1, 1); resliceAxes->SetElement(1, 2, 0);
        resliceAxes->SetElement(2, 0, 1); resliceAxes->SetElement(2, 1, 0); resliceAxes->SetElement(2, 2, 0);
    }

    // ★ 핵심 포인트: 회전 행렬의 이동(Translation) 위치에 볼륨의 정확한 중심점 지정
    // 이 처리를 해야 Coronal/Sagittal 변환 시 절단면이 볼륨 바깥(허공)으로 튕겨 나가지 않음
    resliceAxes->SetElement(0, 3, center[0]);
    resliceAxes->SetElement(1, 3, center[1]);
    resliceAxes->SetElement(2, 3, center[2]);
    
    reslice->SetResliceAxes(resliceAxes);
	reslice->SetInterpolationModeToLinear(); // 선형 보간 적용
    reslice->Update();



    // ★ Window/Level Lookup Table
    auto dicomLut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
    dicomLut->SetWindow(1000); // Window Width
    dicomLut->SetLevel(300);   // Window Level
    dicomLut->Build();

    // ★ [추가] 원본 DICOM을 8-bit RGBA로 변환하는 Window/Level 필터
    auto dicomColorMap = vtkSmartPointer<vtkImageMapToColors>::New();
    dicomColorMap->SetInputConnection(reslice->GetOutputPort());
    dicomColorMap->SetLookupTable(dicomLut);         // CT Window Width (밝기 범위)
    dicomColorMap->SetOutputFormatToRGBA(); // 출력 포맷을 4채널(RGBA)로 고정


    auto maskReslice = vtkSmartPointer<vtkImageReslice>::New();
    maskReslice->SetInputData(maskData);
    maskReslice->SetOutputDimensionality(2);
    maskReslice->SetResliceAxes(resliceAxes);     // 원본과 완벽히 같은 행렬 사용
    maskReslice->SetInterpolationModeToNearestNeighbor(); // 마스크는 보간하면 경계가 흐려지므로 Nearest 사용

    // -------------------------------------------------------------
    // 3. 마스크에 색상 및 투명도(Alpha) 맵핑
    // -------------------------------------------------------------
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(2);
    lut->SetTableRange(0, 1);
    lut->Build();
    lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0); // 0: 완전히 투명 (Alpha 0)
    lut->SetTableValue(1, 1.0, 0.0, 0.0, 0.6); // 1: 빨간색 (Alpha 0.6 = 60% 불투명)

    auto colorMap = vtkSmartPointer<vtkImageMapToColors>::New();
    colorMap->SetLookupTable(lut);
    colorMap->SetInputConnection(maskReslice->GetOutputPort());
    colorMap->SetOutputFormatToRGBA();

    // -------------------------------------------------------------
    // 4. 원본과 마스크 블렌딩 (겹치기)
    // -------------------------------------------------------------
    auto blend = vtkSmartPointer<vtkImageBlend>::New();
    //blend->AddInputConnection(reslice->GetOutputPort()); // Background (Layer 0)
    blend->AddInputConnection(colorMap->GetOutputPort());     // Foreground (Layer 1)
	blend->AddInputConnection(dicomColorMap->GetOutputPort()); // Background (Layer 0)


    auto maskActor = vtkSmartPointer<vtkImageActor>::New();
    maskActor->GetMapper()->SetInputConnection(colorMap->GetOutputPort());

    // ★ Z-fighting(두 영상이 같은 위치에서 깜빡이는 현상) 방지를 위해 마스크를 DICOM 바로 앞(Z+0.1)에 배치
    maskActor->SetPosition(0, 0, 0.1);



    // 렌더러에 2D 액터로 올리기
    auto imageActor = vtkSmartPointer<vtkImageActor>::New();
    imageActor->GetMapper()->SetInputConnection(dicomColorMap->GetOutputPort());

    renderer->RemoveAllViewProps();
    renderer->AddActor(imageActor);
    renderer->AddActor(maskActor);  // 2. 그 위에 반투명 빨간 마스크 얹기
    renderer->ResetCamera();
    renderWindow->Render();
}