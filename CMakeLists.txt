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
src/render.h
)

# Ray Tracer
SET(RAYTRACER_SOURCES 
${COMMON_SOURCES}
src/raytracer/ray_render.cpp
src/raytracer/sceneobjects.h
)
INCLUDE_DIRECTORIES(src/raytracer)
ADD_EXECUTABLE (raytracer WIN32 ${RAYTRACER_SOURCES} ${DEVICE_SOURCES}) # Win32 ignored on non-windows

# Empty example
SET(EMPTY_SOURCES 
src/empty/render.cpp
src/utils/bitmap_utils.h
)
INCLUDE_DIRECTORIES(src/empty)
ADD_EXECUTABLE (empty WIN32 ${EMPTY_SOURCES} ${DEVICE_SOURCES}) # Win32 ignored on non-windows

# Game of Life example
SET(GOL_SOURCES 
src/game_of_life/life_render.cpp
)
INCLUDE_DIRECTORIES(src/game_of_life)
ADD_EXECUTABLE (game_of_life WIN32 ${GOL_SOURCES} ${DEVICE_SOURCES}) # Win32 ignored on non-windows


SOURCE_GROUP(Device REGULAR_EXPRESSION ".*(device)+")
