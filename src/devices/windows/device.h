#pragma once

#include <vector>
#include <glm/glm.hpp>

enum class DeviceKeyType
{
    Ctrl
};
struct BufferData
{
    int BufferWidth = 1024;
    int BufferHeight = 768;
    std::vector<glm::vec4> buffer;
};

extern BufferData bufferData;

void device_show();
void device_init();
void device_copy_buffer();

bool device_is_key_down(DeviceKeyType type);


