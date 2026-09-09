#include "BrushInteractorStyle.h"
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>               // ★ GetDefaultRenderer() 반환 타입 정의
#include <vtkRenderWindow.h>           // ★ GetRenderWindow() 반환 타입 정의
#include <vtkRenderWindowInteractor.h> // ★ GetInteractor() 반환 타입 정의
#include <vtkPropPicker.h>             // ★ vtkPropPicker 정의
#include <vtkActor.h>                  // ★ GetActor() 반환 타입 정의
#include <cmath>
#include <vtkMatrix4x4.h>


vtkStandardNewMacro(BrushInteractorStyle);

void BrushInteractorStyle::OnLeftButtonDown() {
    
    m_isDrawing = true;

    int displayX = this->GetInteractor()->GetEventPosition()[0];
    int displayY = this->GetInteractor()->GetEventPosition()[1];
    //qDebug() << "[Step 2 Clicked] Voxel Index (I, J, K):" << displayX << displayY << 0;

    int voxelIdx[3];
    if (GetVoxelIndexUnderMouse(displayX, displayY, voxelIdx)) {
		PaintVoxels(voxelIdx);
        qDebug() << "[Step 2 Clicked] Voxel Index (I, J, K):" << voxelIdx[0] << voxelIdx[1] << voxelIdx[2];
    }
    else {
        qDebug() << "[Step 2 Clicked] Volume out of range ";
    }

    // 슬라이드 이동 등 기본 기능도 유지하고 싶다면 주석 해제 (단, 드래그 시 카메라 이동과 그리기 충돌 주의)
    // vtkInteractorStyleImage::OnLeftButtonDown(); 
}

void BrushInteractorStyle::OnMouseMove() {
    if (!m_isDrawing) {
        vtkInteractorStyleImage::OnMouseMove();
        return;
    }

    int displayX = this->GetInteractor()->GetEventPosition()[0];
    int displayY = this->GetInteractor()->GetEventPosition()[1];

    int voxelIdx[3];
    if (GetVoxelIndexUnderMouse(displayX, displayY, voxelIdx)) {
        // ★ 마우스가 움직일 때마다 해당 복셀 위치에 브러시 칠하기 수행
        PaintVoxels(voxelIdx);
    }

}

void BrushInteractorStyle::OnLeftButtonUp() {

    m_isDrawing = false;
    vtkInteractorStyleImage::OnLeftButtonUp();


     //★ 브러시 그리기가 끝났을 때 3D 볼륨 뷰어도 함께 갱신
     //(DicomVolumeViewer 포인터를 스타일 클래스에 주입해 두었다면 아래와 같이 호출 가능)
    // if (m_viewer) {
    //     m_viewer->RenderVolume(m_imageData, m_maskData);
    // }

    //return;

}

bool BrushInteractorStyle::GetVoxelIndexUnderMouse(int displayX, int displayY, int voxelIndex[3]) {
    if (!m_imageData) return false;

    // 1. 현재 렌더러 가져오기
    vtkRenderer* renderer = this->GetDefaultRenderer();
    if (!renderer) renderer = this->GetInteractor()->FindPokedRenderer(displayX, displayY);
    if (!renderer) return false;

    // 2. 디스플레이(마우스) 좌표를 렌더러 내부의 월드 좌표로 변환
    renderer->SetDisplayPoint(displayX, displayY, 0.0);
    renderer->DisplayToWorld();
    double worldPos[4];
    renderer->GetWorldPoint(worldPos);

    if (worldPos[3] != 0.0) {
        worldPos[0] /= worldPos[3];
        worldPos[1] /= worldPos[3];
        worldPos[2] /= worldPos[3];
    }

    // 3. 2D 단면 좌표계(worldPos)를 3D 원본 볼륨 좌표계로 변환 (ResliceAxes 행렬 곱셈)
    double outPos4[4] = { worldPos[0], worldPos[1], 0.0, 1.0 };
    double inPos4[4];
    m_resliceAxes->MultiplyPoint(outPos4, inPos4);

    // 2. World 좌표(mm) -> Voxel Index (I, J, K) 변환 공식
    // Index = (WorldPosition - Origin) / Spacing
    double origin[3], spacing[3];
    int dims[3];
    m_imageData->GetOrigin(origin);
    m_imageData->GetSpacing(spacing);
    m_imageData->GetDimensions(dims);

    voxelIndex[0] = std::round((inPos4[0] - origin[0]) / spacing[0]);
    voxelIndex[1] = std::round((inPos4[1] - origin[1]) / spacing[1]);
    voxelIndex[2] = std::round((inPos4[2] - origin[2]) / spacing[2]);

    qDebug() << "voxel x:" << voxelIndex[0] << "y:" << voxelIndex[1] << "z:" << voxelIndex[2]; // 범위 밖일 때도 찍히는지 확인용



    // 3. 인덱스가 볼륨 범위 내에 있는지 확인 (Bounding Box Check)
    if (voxelIndex[0] < 0 || voxelIndex[0] >= dims[0] ||
        voxelIndex[1] < 0 || voxelIndex[1] >= dims[1] ||
        voxelIndex[2] < 0 || voxelIndex[2] >= dims[2]) {
        return false;
    }

    return true;
}




void BrushInteractorStyle::PaintVoxels(int* voxelIndex) {
    if (!m_maskData) return;
	int centerI = *(voxelIndex);
	int centerJ = *(voxelIndex + 1);
	int centerK = *(voxelIndex + 2);


    // 1. 볼륨의 실제 물리적 간격(Spacing)을 가져옵니다.
    double spacing[3];
    m_maskData->GetSpacing(spacing);
    int dims[3];
    m_maskData->GetDimensions(dims);

    // 2. 브러시 크기를 '복셀 개수'가 아닌 '물리적 mm 단위'로 지정합니다.
    double physicalRadius = 3.0; // 3.0mm 크기의 둥근 브러시

    // 3. 탐색을 최적화하기 위해 탐색 범위(Loop range)를 복셀 단위로 역산합니다.
    int radiusI = std::ceil(physicalRadius / spacing[0]);
    int radiusJ = std::ceil(physicalRadius / spacing[1]);
    int radiusK = std::ceil(physicalRadius / spacing[2]);

    bool modified = false;

    // 계산된 복셀 범위만큼만 순회
    for (int k = centerK - radiusK; k <= centerK + radiusK; ++k) {
        for (int j = centerJ - radiusJ; j <= centerJ + radiusJ; ++j) {
            for (int i = centerI - radiusI; i <= centerI + radiusI; ++i) {
                if (i >= 0 && i < dims[0] && j >= 0 && j < dims[1] && k >= 0 && k < dims[2]) {

                    // ★ 핵심: 인덱스 차이에 Spacing을 곱해 실제 물리적 거리(mm)를 구함
                    double dx = (i - centerI) * spacing[0];
                    double dy = (j - centerJ) * spacing[1];
                    double dz = (k - centerK) * spacing[2];

                    // 물리적 거리의 제곱 합이 브러시 반경의 제곱보다 작거나 같으면 색칠
                    if (dx * dx + dy * dy + dz * dz <= physicalRadius * physicalRadius) {
                        unsigned char* pixel = static_cast<unsigned char*>(m_maskData->GetScalarPointer(i, j, k));
                        if (pixel && *pixel != 1) {
                            *pixel = 1;
                            modified = true;
                        }
                    }
                }
            }
        }
    }

    if (modified) {
        // VTK 파이프라인에 데이터가 수정되었음을 알림
        m_maskData->Modified();

        // 2D 렌더윈도우 즉시 갱신
        if (this->GetDefaultRenderer()) {
            this->GetDefaultRenderer()->GetRenderWindow()->Render();
        }
        else if (this->GetInteractor() && this->GetInteractor()->GetRenderWindow()) {
            this->GetInteractor()->GetRenderWindow()->Render();
        }
    }
}