#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <QVTKOpenGLNativeWidget.h>
#include <QComboBox>
#include <QMessageBox>
#include <QLabel>
#include <VolumeBrushInteractorStyle.h>
#include<BrushInteractorStyle.h>

class DicomVolumeViewer : public QMainWindow {
    Q_OBJECT

public:
    enum class InteractionMode {
        Normal,   // 3D 카메라 기본 탐색/회전
        Brush3D,  // 3D 볼륨 브러시
        Brush2D   // 2D 슬라이스 브러시
    };
    explicit DicomVolumeViewer(QWidget* parent = nullptr);
    ~DicomVolumeViewer() override = default;



    QPushButton* getOpenButton() const;
    QPushButton* getShowButton() const;
	QComboBox* getComboBox() const;
    QString getSelectedViewMode() const;
    QPushButton* getBrushToggleBtn() { return m_brushToggleBtn; }
    void SetInteractionMode(InteractionMode mode); // 인터렉티브 모드 전환
    void SetImageData(vtkSmartPointer<vtkImageData> data) { m_currentImageData = data; } // 인터렉티브 모드 전환

    void RenderVolume(vtkSmartPointer<vtkImageData> imageData);
    void RenderSlice(vtkSmartPointer<vtkImageData> imageData, QString viewMode); // ★ 2D 단면 렌더링 함수 신규 추가
    void ToggleBrushMode(); // UI 토글 버튼 시그널과 연결


private:

    void InitInteractor();
    void SyncStyleData();
    QPushButton* btnOpenDicom;
    QPushButton* m_showButton;       // 렌더링 실행 버튼
    QPushButton* m_brushToggleBtn;
    QComboBox* m_viewComboBox;
    QVTKOpenGLNativeWidget* vtkWidget;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkMatrix4x4> m_currentResliceAxes; // 현재 뷰 모드의 Reslice 행렬
    vtkSmartPointer<vtkImageData> m_currentImageData;
    vtkSmartPointer <vtkVolume> m_volume;

    vtkSmartPointer<vtkImageData> m_sharedMaskData;
    vtkSmartPointer<VolumeBrushInteractorStyle> m_brush3DStyle;
    vtkSmartPointer<BrushInteractorStyle> m_brush2DStyle;

    vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_normalStyle;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    bool isBrushMode = false;
    InteractionMode m_currentMode = InteractionMode::Normal;


};