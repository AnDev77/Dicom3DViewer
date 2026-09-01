#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <QVTKOpenGLNativeWidget.h>

class DicomVolumeViewer : public QMainWindow {
    Q_OBJECT

public:
    explicit DicomVolumeViewer(QWidget* parent = nullptr);
    ~DicomVolumeViewer() override = default;

    QPushButton* getOpenButton() const { return btnOpenDicom; }
    void RenderVolume(vtkSmartPointer<vtkImageData> imageData);

private:
    QPushButton* btnOpenDicom;
    QVTKOpenGLNativeWidget* vtkWidget;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderer> renderer;
};