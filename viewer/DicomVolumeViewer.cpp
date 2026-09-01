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

    // Opacity
    auto opacityFunc = vtkSmartPointer<vtkPiecewiseFunction>::New();
    opacityFunc->AddPoint(-1000.0, 0.0);
    opacityFunc->AddPoint(-400.0, 0.0);
    opacityFunc->AddPoint(-100.0, 0.05);
    opacityFunc->AddPoint(100.0, 0.15);
    opacityFunc->AddPoint(200.0, 0.4);
    opacityFunc->AddPoint(800.0, 0.85);
    opacityFunc->AddPoint(1500.0, 0.95);

    // Color
    auto colorFunc = vtkSmartPointer<vtkColorTransferFunction>::New();
    colorFunc->AddRGBPoint(-1000.0, 0.0, 0.0, 0.0);
    colorFunc->AddRGBPoint(-100.0, 0.6, 0.3, 0.1);
    colorFunc->AddRGBPoint(100.0, 0.8, 0.6, 0.4);
    colorFunc->AddRGBPoint(300.0, 0.9, 0.85, 0.8);
    colorFunc->AddRGBPoint(1000.0, 1.0, 1.0, 1.0);

    // Property
    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetColor(colorFunc);
    volumeProperty->SetScalarOpacity(opacityFunc);
    volumeProperty->ShadeOn();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetAmbient(0.3);
    volumeProperty->SetDiffuse(0.7);
    volumeProperty->SetSpecular(0.2);

    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    renderer->RemoveAllViewProps();
    renderer->AddVolume(volume);
    renderer->ResetCamera();

    renderWindow->Render();
}