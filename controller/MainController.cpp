#include "MainController.h"
#include <QFileDialog>
#include <QMessageBox>

#include "../service/DicomLoadService.h"
#include "../viewer/DicomVolumeViewer.h"


MainController::MainController( DicomVolumeViewer* viewer, QObject* parent)
    : QObject(parent), viewer(viewer) {

    connect(viewer->getOpenButton(), &QPushButton::clicked, this, &MainController::onOpenFolderClicked);

    // ★ Show 버튼 연결 추가
    connect(viewer->getShowButton(), &QPushButton::clicked, this, &MainController::onShowButtonClicked);

	connect(viewer->getComboBox(), &QComboBox::currentTextChanged, this, &MainController::onViewComboClicked);

}

void MainController::onOpenFolderClicked() {
    QString dirPath = QFileDialog::getExistingDirectory(
        viewer, "Select DICOM series folder", "", QFileDialog::ShowDirsOnly);

    if (dirPath.isEmpty()) return;

    // ★ 비동기 로딩 서비스 생성 (이후 서비스 내부에서 QThread + Worker 구동)
    DicomLoadService* service = new DicomLoadService(this);

    connect(service, &DicomLoadService::finished, this, [=](vtkSmartPointer<vtkImageData> volumeData) {
        if (volumeData) {
            m_sharedVolumeData = volumeData; // 원본 데이터 보관
            //viewer->RenderVolume(m_sharedVolumeData); // 우선 기존 3D 뷰어로 렌더링 테스트
            viewer->getOpenButton()->setEnabled(true); // 버튼 복구
            QMessageBox::information(viewer, "Success", "Asynchronous DICOM volume loading completed!");
        }
        else {
            QMessageBox::critical(viewer, "Error", "Failed to read valid DICOM data .");
        }
        service->deleteLater();
        });

    connect(service, &DicomLoadService::error, this, [=](QString msg) {
        QMessageBox::critical(viewer, "Error", msg);
        viewer->getOpenButton()->setEnabled(true); // 버튼 복구 
        service->deleteLater();
        });

    // 비동기 로딩 시작 (이 시점에 UI가 멈추지 않고 백그라운드 스레드가 돎)
    service->loadAsync(dirPath);
}



void MainController::onViewComboClicked() {
    if (!m_sharedVolumeData) {
        QMessageBox::warning(viewer, "Warning", "FOLDER FIRST!");
        return;
    }

    QString viewMode = viewer->getSelectedViewMode();

    if (viewMode.contains("Coronal")) {
		viewer->RenderSlice(m_sharedVolumeData, "Coronal");
    }
    else if (viewMode.contains("Axial")) {
        viewer->RenderSlice(m_sharedVolumeData, "Axial");
    }
    else if (viewMode.contains("Sagittal")) {
        viewer->RenderSlice(m_sharedVolumeData, "Sagittal");
    }
}



void MainController::onShowButtonClicked() {
    if (!m_sharedVolumeData) {
        QMessageBox::warning(viewer, "Warning", "Warning", "FOLDER FIRST!");
        return;

    }

    viewer->RenderVolume(m_sharedVolumeData);
}