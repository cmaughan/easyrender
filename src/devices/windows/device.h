#pragma once

#include <vector>
#include <glm/glm.hpp>

enum class DeviceKeyType
{
    Ctrl
};

struct BufferData
{
    int BufferWidth;
    int BufferHeight;
    glm::vec4* buffer;
};

struct DeviceParams
{
    float zoomFactor = 1.0f;
    glm::vec2 offset = glm::vec2(0.0f);
    const char* pName = "EasyRender";
};
extern DeviceParams deviceParams;

BufferData* device_buffer_create(int width = 0, int height = 0);
void device_buffer_destroy(BufferData* pData);
void device_buffer_ensure_screen_size(BufferData* pData);
void device_buffer_resize(BufferData* pData, int width, int height);
void device_copy_buffer(BufferData* buffer);
bool device_is_key_down(DeviceKeyType type);


