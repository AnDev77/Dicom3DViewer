#pragma once
#include <QObject>
#include <QString>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <QThread>
#include "LoadWorker.h"

class DicomLoadService : public QObject {
    Q_OBJECT
public:
    explicit DicomLoadService(QObject* parent = nullptr);
    ~DicomLoadService();

    void loadAsync(const QString& folderPath);


signals:
    void finished(vtkSmartPointer<vtkImageData> imageData);
    void error(const QString& message);

private:
    void cleanupThread();
    QThread* m_thread = nullptr;
    LoaderWorker* m_worker = nullptr;
};