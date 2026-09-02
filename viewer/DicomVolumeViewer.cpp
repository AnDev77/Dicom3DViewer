#include "DicomVolumeViewer.h"
#include <QVBoxLayout>
#include <vtkSmartVolumeMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>

DicomVolumeViewer::DicomVolumeViewer(QWidget* parent) : QMainWindow(parent) {
    this->setWindowTitle("DICOM Series to 3D Volume Viewer");
    this->resize(1024, 768);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    btnOpenDicom = new QPushButton("DICOM open folder", this);
    btnOpenDicom->setFixedHeight(40);

    vtkWidget = new QVTKOpenGLNativeWidget(this);

    layout->addWidget(btnOpenDicom);
    layout->addWidget(vtkWidget);
    this->setCentralWidget(centralWidget);

    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWidget->setRenderWindow(renderWindow);

    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.1, 0.1, 0.1);
    renderWindow->AddRenderer(renderer);
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

    // ==========================================
    // 2. 색상 전달 함수 (Color Transfer Function)
    // ==========================================
    // vtkColorTransferFunction: 밀도 값에 따라 3D 렌더링 색상(RGB)을 매핑하는 함수
    // - 첫 번째 인자 (double): HU 밀도 값
    // - 두 번째~네 번째 인자 (double, double, double): Red, Green, Blue 색상 비율 (0.0 ~ 1.0 범위)
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
    volumeProperty->SetInterpolationTypeToLinear(); // 선형 보간을 적용해 계단 현상 완화
    volumeProperty->SetAmbient(0.3);                // 주변광 세기 설정
    volumeProperty->SetDiffuse(0.7);                // 확산광 세기 설정
    volumeProperty->SetSpecular(0.2);               // 반사광(하이라이트) 세기 설정

    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    renderer->RemoveAllViewProps();
    renderer->AddVolume(volume);
    renderer->ResetCamera();
    renderWindow->Render();
}