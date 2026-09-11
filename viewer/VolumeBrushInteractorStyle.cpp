#include "VolumeBrushInteractorStyle.h"
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <QDebug> // Qt 환경 가정 (필요에 따라 std::cout으로 변경)
#include<vtkRenderWindow.h>


vtkStandardNewMacro(VolumeBrushInteractorStyle);

VolumeBrushInteractorStyle::VolumeBrushInteractorStyle() {
    m_picker = vtkSmartPointer<vtkVolumePicker>::New();

}


void VolumeBrushInteractorStyle::OnMouseMove() {
    if(!isDrawing) {
        vtkInteractorStyleTrackballCamera::OnMouseMove();
        return;
    }
    int voxelIdx[3];
    if (GetVoxels(voxelIdx)) {
        // ★ 마우스가 움직일 때마다 해당 복셀 위치에 브러시 칠하기 수행
        PaintVoxels(voxelIdx);
    }
}


void VolumeBrushInteractorStyle::OnLeftButtonUp() {

    isDrawing = false;
    vtkInteractorStyleTrackballCamera::OnMouseMove();


    //★ 브러시 그리기가 끝났을 때 3D 볼륨 뷰어도 함께 갱신
    //(DicomVolumeViewer 포인터를 스타일 클래스에 주입해 두었다면 아래와 같이 호출 가능)
   // if (m_viewer) {
   //     m_viewer->RenderVolume(m_imageData, m_maskData);
   // }

   //return;

}

void VolumeBrushInteractorStyle::OnLeftButtonDown() {
    int* pos = this->GetInteractor()->GetEventPosition();
    isDrawing = true;
    int* voxels = new int[3];
    if (GetVoxels(voxels)) {
        qDebug() << "  -> Converted Voxel Index: (" << voxels[0] << "," << voxels[1] << "," << voxels[2] << ")";
        PaintVoxels(voxels);
    }

    // ★ 중요: 부모 클래스의 OnLeftButtonDown()을 호출하지 않음으로써 화면 회전을 막습니다.
    // vtkInteractorStyleTrackballCamera::OnLeftButtonDown(); 
}


bool VolumeBrushInteractorStyle::GetVoxels(int * voxels) {
    int* pos = this->GetInteractor()->GetEventPosition();
    vtkRenderer* renderer = this->GetDefaultRenderer();
    if (!renderer || !m_imageData) return false;


    if (m_picker->Pick(pos[0], pos[1], 0.0, renderer)) {
        double worldPos[3];
        m_picker->GetPickPosition(worldPos);

        // 2. World 좌표(mm) -> Voxel Index (I, J, K) 변환 공식
        double origin[3], spacing[3];
        int dims[3];
        m_imageData->GetOrigin(origin);
        m_imageData->GetSpacing(spacing);
        m_imageData->GetDimensions(dims);

        int voxelX = std::round((worldPos[0] - origin[0]) / spacing[0]);
        int voxelY = std::round((worldPos[1] - origin[1]) / spacing[1]);
        int voxelZ = std::round((worldPos[2] - origin[2]) / spacing[2]);
        voxels[0] = voxelX;
        voxels[1] = voxelY;
        voxels[2] = voxelZ;
        qDebug() << "[3D Brush Mode in Volume";
        qDebug() << "  -> Converted Voxel Index: (" << voxels[0] << "," << voxels[1] << "," << voxels[2] << ")";
        if (voxelX < 0 || voxelX >= dims[0] ||
            voxelY < 0 || voxelY >= dims[1] ||
            voxelZ < 0 || voxelZ >= dims[2]) {

            qDebug() << "[3D Brush Mode out  Volume";
            return false;
        }
        else {
            return true;
        }
    }
    return false;

}


void VolumeBrushInteractorStyle::PaintVoxels(int* voxelIndex) {
    qDebug() << "in paint" << m_maskData.Get();

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
                    short huValue = m_imageData->GetScalarComponentAsDouble(i, j, k, 0);
                    
                    if (basePtr[index] != 1 && huValue >= 200) {
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

        qDebug() << "[Brush Success] Painted";

        // 2D 렌더윈도우 즉시 갱신
        if (this->GetDefaultRenderer()) {
            this->GetDefaultRenderer()->GetRenderWindow()->Render();
        }
        else if (this->GetInteractor() && this->GetInteractor()->GetRenderWindow()) {
            this->GetInteractor()->GetRenderWindow()->Render();
        }
    }
}