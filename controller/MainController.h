#pragma once
#include <QObject>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
class DicomVolumeViewer;

class MainController : public QObject {
    Q_OBJECT
public:
    explicit MainController(DicomVolumeViewer* viewer, QObject* parent = nullptr);
    ~MainController() = default;

private slots:
    void onOpenFolderClicked();
    void onViewComboClicked();
    void onShowButtonClicked(); // ★ 새로 추가될 Show 버튼 처리 슬롯
    // ★ 새로 추가될 View 버튼 처리 슬롯

private:
    DicomVolumeViewer* viewer;

    // 백그라운드 스레드에서 로딩된 원본 3D 볼륨 데이터를 보관하는 단일 소스(Single Source of Truth)
    vtkSmartPointer<vtkImageData> m_sharedVolumeData = nullptr;
};