#pragma once
#include <QObject>
#include "../model/VolumeModel.h"
#include "../viewer/DicomVolumeViewer.h"

class MainController : public QObject {
    Q_OBJECT

public:
    MainController(VolumeModel* model, DicomVolumeViewer* viewer, QObject* parent = nullptr);

private slots:
    void onOpenFolderClicked();

private:
    VolumeModel* model;
    DicomVolumeViewer* viewer;
};