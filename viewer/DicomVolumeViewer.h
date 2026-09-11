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

class DicomVolumeViewer : public QMainWindow {
    Q_OBJECT

public:
    explicit DicomVolumeViewer(QWidget* parent = nullptr);
    ~DicomVolumeViewer() override = default;

    QPushButton* getOpenButton() const;
    QPushButton* getShowButton() const;
	QComboBox* getComboBox() const;
    QString getSelectedViewMode() const;
    QPushButton* getBrushToggleBtn() { return m_brushToggleBtn; }


    void RenderVolume(vtkSmartPointer<vtkImageData> imageData);
    void RenderSlice(vtkSmartPointer<vtkImageData> imageData, QString viewMode); // ★ 2D 단면 렌더링 함수 신규 추가
    void InitInteractor(); // 초기화 단계에서 호출
    void ToggleBrushMode(); // UI 토글 버튼 시그널과 연결

private:
    QPushButton* btnOpenDicom;
    QPushButton* m_showButton;       // 렌더링 실행 버튼
    QPushButton* m_brushToggleBtn;
    QComboBox* m_viewComboBox;
    QVTKOpenGLNativeWidget* vtkWidget;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkMatrix4x4> m_currentResliceAxes; // 현재 뷰 모드의 Reslice 행렬
    vtkSmartPointer<vtkImageData> m_currentImageData;
    vtkSmartPointer < vtkVolume> m_volume;

    vtkSmartPointer<vtkImageData> m_sharedMaskData;
    vtkSmartPointer<VolumeBrushInteractorStyle> m_brushStyle;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_normalStyle;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    bool isBrushMode = false;

};