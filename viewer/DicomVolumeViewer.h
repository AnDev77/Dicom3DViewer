#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <QVTKOpenGLNativeWidget.h>
#include <QComboBox>

class DicomVolumeViewer : public QMainWindow {
    Q_OBJECT

public:
    explicit DicomVolumeViewer(QWidget* parent = nullptr);
    ~DicomVolumeViewer() override = default;

    QPushButton* getOpenButton() const;
    QPushButton* getShowButton() const;
	QComboBox* getComboBox() const;
    QString getSelectedViewMode() const;

    void RenderVolume(vtkSmartPointer<vtkImageData> imageData);
    void RenderSlice(vtkSmartPointer<vtkImageData> imageData, QString viewMode); // ★ 2D 단면 렌더링 함수 신규 추가

private:
    QPushButton* btnOpenDicom;
    QPushButton* m_showButton;       // 렌더링 실행 버튼
    QComboBox* m_viewComboBox;

    QVTKOpenGLNativeWidget* vtkWidget;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderer> renderer;
    
};