#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#define _CRT_SECURE_NO_WARNINGS
#include <QApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);


#include "controller/MainController.h"
#include "model/VolumeModel.h"
#include "viewer/DicomVolumeViewer.h"
// [여기에 기존 Notion 문서의 모든 #pragma comment(lib, "...") 복사 붙여넣기]
#pragma comment(lib, "gdcmMSFF.lib")
#pragma comment(lib, "gdcmDICT.lib")
#pragma comment(lib, "gdcmDSED.lib")
#pragma comment(lib, "gdcmCommon.lib")
#pragma comment(lib, "gdcmIOD.lib")

#pragma comment(lib, "vtkIOXML-9.7d.lib") // 사용 중이신 VTK 버전에 맞게 설정

// VTK Library Link
#pragma comment(lib, "vtksys-9.7d.lib")
#pragma comment(lib, "vtkCommonCore-9.7d.lib")
#pragma comment(lib, "vtkCommonDataModel-9.7d.lib")
#pragma comment(lib, "vtkCommonExecutionModel-9.7d.lib")
#pragma comment(lib, "vtkIOCore-9.7d.lib")
#pragma comment(lib, "vtkIOImage-9.7d.lib")
#pragma comment(lib, "vtkRenderingCore-9.7d.lib")
#pragma comment(lib, "vtkRenderingOpenGL2-9.7d.lib")
#pragma comment(lib, "vtkRenderingVolumeOpenGL2-9.7d.lib")
#pragma comment(lib, "vtkInteractionStyle-9.7d.lib")
#pragma comment(lib, "vtkGUISupportQt-9.7d.lib")

#pragma comment(lib, "vtkFiltersCore-9.7d.lib")

// Qt Library Link
#pragma comment(lib, "Qt6Cored.lib")
#pragma comment(lib, "Qt6Guid.lib")
#pragma comment(lib, "Qt6Widgetsd.lib")
#pragma comment(lib, "Qt6OpenGLd.lib")
#pragma comment(lib, "Qt6OpenGLWidgetsd.lib")




int main(int argc, char* argv[]) {
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication a(argc, argv);

    VolumeModel model;
    DicomVolumeViewer viewer;

    // 에러 1 해결: 모델과 뷰어의 주소(&)를 넘겨주며 컨트롤러 생성
    MainController controller(&model, &viewer);

    // 에러 2 해결: 컨트롤러가 아닌 뷰어 객체의 show() 메서드 호출
	viewer.show();  


    return a.exec();

}