#include "MainController.h"
#include <QFileDialog>
#include <QMessageBox>
#include "../model/VolumeModel.h"

MainController::MainController(VolumeModel* model, DicomVolumeViewer* viewer, QObject* parent)
    : QObject(parent), model(model), viewer(viewer) {

    connect(viewer->getOpenButton(), &QPushButton::clicked, this, &MainController::onOpenFolderClicked);
}

void MainController::onOpenFolderClicked() {
    QString dirPath = QFileDialog::getExistingDirectory(
        viewer, "DICOM 시리즈 폴더 선택", "", QFileDialog::ShowDirsOnly);

    if (!dirPath.isEmpty()) {
        auto volumeData = model->LoadDicomSeriesPureGDCM(dirPath);
        if (volumeData) {
            viewer->RenderVolume(volumeData);
        }
        else {
            QMessageBox::critical(viewer, "오류", "유효한 DICOM 데이터를 읽지 못했습니다.");
        }
    }
}