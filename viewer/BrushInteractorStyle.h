#ifndef BRUSHINTERACTORSTYLE_H
#define BRUSHINTERACTORSTYLE_H

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPropPicker.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <QDebug>
#include <vtkMatrix4x4.h>

class BrushInteractorStyle : public vtkInteractorStyleImage {
public:
    static BrushInteractorStyle* New();
    vtkTypeMacro(BrushInteractorStyle, vtkInteractorStyleImage);

    void SetImageData(vtkSmartPointer<vtkImageData> imageData) { m_imageData = imageData; }

	void PaintVoxels(int* voxelIndex);
	void SetMaskData(vtkSmartPointer<vtkImageData> maskData) { m_maskData = maskData; }
    void SetResliceAxes(vtkSmartPointer<vtkMatrix4x4> axes) { m_resliceAxes = axes; }

    void SetImageDate(vtkSmartPointer<vtkImageData> data) { m_imageData = data; }

    virtual void OnLeftButtonDown() override;
    virtual void OnMouseMove() override;
    virtual void OnLeftButtonUp() override;

private:
    bool m_isDrawing = false;	
    
    vtkSmartPointer<vtkImageData> m_maskData; // 마스크 데이터 저장용

    vtkSmartPointer<vtkImageData> m_imageData;
    vtkSmartPointer<vtkMatrix4x4> m_resliceAxes; // ★ 절단면 행렬 저장용
    // 화면 좌표(Display) -> DICOM Voxel Index(I, J, K) 변환 함수
    bool GetVoxelIndexUnderMouse(int displayX, int displayY, int voxelIndex[3]);
};

#endif // BRUSHINTERACTORSTYLE_H