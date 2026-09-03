cmake_minimum_required(VERSION 3.16)
project(BuildTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. 라이브러리 경로 변수 설정 (본인 PC 경로에 맞게 수정)
set(QT_BIN_DIR "C:/Qt/6.7.3/msvc2019_64/bin")
set(VTK_BIN_DIR "C:/VTK/build/bin")
set(GDCM_DIR "C:/GDCM-3.2.7-Windows-x86_64")
set(VTK_DIR "C:/VTK/build")
# Qt6 찾기
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

# VTK 찾기
find_package(VTK REQUIRED COMPONENTS
    CommonCore CommonDataModel CommonExecutionModel
    IOCore IOImage IOXML RenderingCore RenderingOpenGL2
    RenderingVolumeOpenGL2 InteractionStyle GUISupportQt
)

# GDCM 포함 및 라이브러리 경로 설정
include_directories(${GDCM_DIR}/include/gdcm-3.2)
link_directories(${GDCM_DIR}/lib)

# 헤더 파일 폴더 지정
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 2. 실행 파일 생성 및 소스 파일 묶기
file(GLOB_RECURSE MODEL_SRC "model/*.cpp" "model/*.h")
file(GLOB_RECURSE CONTROLLER_SRC "controller/*.cpp" "controller/*.h")
file(GLOB_RECURSE VIEWER_SRC "viewer/*.cpp" "viewer/*.h")
file(GLOB_RECURSE SERVICE_SRC "service/*.cpp" "service/*.h")

# 긁어온 변수들을 통째로 실행 파일에 집어넣음
add_executable(${PROJECT_NAME}
    main.cpp
    ${MODEL_SRC}
    ${CONTROLLER_SRC}
    ${VIEWER_SRC}
    ${SERVICE_SRC}
)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer
    ${CMAKE_CURRENT_SOURCE_DIR}/controller
    ${CMAKE_CURRENT_SOURCE_DIR}/service
)

# 3. 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
    ${VTK_LIBRARIES}
    gdcmMSFF gdcmDICT gdcmDSED gdcmCommon gdcmIOD
)

# 4. [핵심 최적화] Visual Studio 실행/디버깅 시에만 DLL 경로 임시 참조
# 이렇게 하면 robocopy로 무거운 DLL을 복사할 필요가 아예 사라집니다.
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_ENVIRONMENT 
    "PATH=${GDCM_DIR}/bin;${VTK_BIN_DIR};${QT_BIN_DIR};%PATH%"
)
file(GLOB VTK_DEBUG_DLLS "${VTK_BIN_DIR}/*-9.7d.dll")
file(GLOB GDCM_DEBUG_DLLS "${GDCM_DIR}/bin/*.dll") # GDCM 디버그용 DLL 전체 수집

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Debug 모드일 때 수집한 GDCM 및 VTK 모든 디버그 DLL을 실행 폴더로 일괄 복사
    COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> -E copy_if_different ${GDCM_DEBUG_DLLS} $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> -E copy_if_different ${VTK_DEBUG_DLLS} $<TARGET_FILE_DIR:${PROJECT_NAME}>
    
    COMMENT "Bulk copying all GDCM and VTK debug DLLs..."
)