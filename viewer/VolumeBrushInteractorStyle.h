#pragma once
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkVolumePicker.h>
#include <vtkImageData.h>

class VolumeBrushInteractorStyle : public vtkInteractorStyleTrackballCamera {
public:
    static VolumeBrushInteractorStyle* New();
    vtkTypeMacro(VolumeBrushInteractorStyle, vtkInteractorStyleTrackballCamera);

    // 좌표 변환을 위한 원본 볼륨 데이터 주입
    void SetVolumeData(vtkSmartPointer<vtkImageData> imageData) { m_imageData = imageData; }
    void PaintVoxels(int* voxelIndex);
    void SetMaskData(vtkSmartPointer<vtkImageData> maskData) { m_maskData = maskData; }
    void OnMouseMove();
    virtual void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    bool GetVoxels(int* voxels);
private:
    VolumeBrushInteractorStyle();
    ~VolumeBrushInteractorStyle() = default;

    vtkSmartPointer<vtkImageData> m_imageData;
    vtkSmartPointer<vtkImageData>m_maskData;
    vtkSmartPointer<vtkVolumePicker> m_picker;
    bool isDrawing = false;
    bool firstDraw = false;
};