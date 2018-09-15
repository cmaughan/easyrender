cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

PROJECT (EasyRender)

MESSAGE(STATUS " CMakeLists: EasyRender")

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

set (APP_ROOT ${CMAKE_CURRENT_LIST_DIR})

INCLUDE_DIRECTORIES(
    m3rdparty/glm 
    src
    src/devices
    src/devices/windows
    src/utils
    src/raytracer
    .
    ) 

# Windows builds
SET(DEVICE_SOURCES
    src/devices/windows/device.cpp
    src/devices/windows/device.h
)

SET(COMMON_SOURCES
src/utils/camera.h
src/utils/camera_manipulator.h
src/utils/bitmap_utils.h
)

# Ray Tracer
SET(RAYTRACER_SOURCES 
${COMMON_SOURCES}
src/raytracer/render.cpp
src/raytracer/render.h
src/raytracer/sceneobjects.h
)
ADD_EXECUTABLE (raytracer WIN32 ${RAYTRACER_SOURCES} ${DEVICE_SOURCES}) # Win32 ignored on non-windows

SOURCE_GROUP(Device REGULAR_EXPRESSION ".*(device)+")
