cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

PROJECT (RayTracer)

MESSAGE(STATUS " CMakeLists: RayTracer")

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

set (APP_ROOT ${CMAKE_CURRENT_LIST_DIR})

INCLUDE_DIRECTORIES(
    windows
    utils
	raytracer
    .
    ) 

SET(DEVICE_SOURCES
    windows/device.cpp
    windows/device.h
)

SET(SOURCES 
raytracer/render.cpp
raytracer/render.h
raytracer/camera.h
raytracer/manipulator.h
raytracer/render.cpp
raytracer/render.h
raytracer/sceneobjects.h
)

ADD_EXECUTABLE (${PROJECT_NAME} WIN32 ${SOURCES} ${DEVICE_SOURCES}) # Win32 ignored on non-windows

SOURCE_GROUP(Device REGULAR_EXPRESSION "(device)+")
