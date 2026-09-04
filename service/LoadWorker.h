#pragma once
#include <QObject>
#include <QString>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

class LoaderWorker : public QObject {
    Q_OBJECT
public:
    explicit LoaderWorker(const QString& path, QObject* parent = nullptr);

public slots:
    void run();

signals:
    void finished(vtkSmartPointer<vtkImageData> imageData);
    void errorOccurred(const QString& message);

private:
    QString m_path;
};