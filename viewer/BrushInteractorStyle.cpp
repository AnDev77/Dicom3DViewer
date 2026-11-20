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

    bool modified = false;

    int radiusI = std::ceil(physicalRadius / spacing[0]);
    int radiusJ = std::ceil(physicalRadius / spacing[1]);
    int radiusK = std::ceil(physicalRadius / spacing[2]);

    // ★ 1. 마스크 데이터의 버퍼 시작 포인터와 슬라이스 크기를 미리 계산
    unsigned char* basePtr = static_cast<unsigned char*>(m_maskData->GetScalarPointer());
    if (!basePtr) return;

    int sliceSize = dims[0] * dims[1]; // Z축 1장당 복셀 개수

    // 탐색 범위 제한 (Bounding Box)
    int minK = std::max(0, centerK - radiusK), maxK = std::min(dims[2] - 1, centerK + radiusK);
    int minJ = std::max(0, centerJ - radiusJ), maxJ = std::min(dims[1] - 1, centerJ + radiusJ);
    int minI = std::max(0, centerI - radiusI), maxI = std::min(dims[0] - 1, centerI + radiusI);

    for (int k = minK; k <= maxK; ++k) {
        double dz = (k - centerK) * spacing[2];
        double dz2 = dz * dz;
        int kOffset = k * sliceSize;

        for (int j = minJ; j <= maxJ; ++j) {
            double dy = (j - centerJ) * spacing[1];
            double dy2 = dy * dy;
            int jOffset = j * dims[0];

            for (int i = minI; i <= maxI; ++i) {
                double dx = (i - centerI) * spacing[0];

                if (dx * dx + dy2 + dz2 <= physicalRadius * physicalRadius) {
                    // ★ 2. GetScalarPointer() 대신 1차원 인덱스 오프셋으로 직접 메모리 접근
                    int index = i + jOffset + kOffset;
                    if (basePtr[index] != 1) {
                        basePtr[index] = 1;
                        modified = true;
                    }
                }
            }
        }
    }

    if (modified) {
        // VTK 파이프라인에 데이터가 수정되었음을 알림
        m_maskData->Modified();

        // 현재의 렌더윈도우 즉시 갱신
        if (this->GetDefaultRenderer()) {
            this->GetDefaultRenderer()->GetRenderWindow()->Render();
        }
        else if (this->GetInteractor() && this->GetInteractor()->GetRenderWindow()) {
            this->GetInteractor()->GetRenderWindow()->Render();
        }
    }
}