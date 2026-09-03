#pragma once
#include <QObject>
#include <QString>
#include <vtkSmartPointer>
#include <vtkImageData>

class DicomLoadService : public QObject {
    Q_OBJECT
public:
    explicit DicomLoadService(QObject* parent = nullptr);
    ~DicomLoadService();

    void loadAsync(const QString& folderPath);

signals:
    void finished(vtkSmartPointer<vtkImageData> imageData);
    void error(const QString& message);
};