#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include "device.h"
#include "bitmap_utils.h"

BufferData* screenBufferData;

void render_init()
{
    deviceParams.pName = "Sample Empty Demo";
}

void render_destroy()
{
    device_buffer_destroy(screenBufferData);
    screenBufferData = nullptr;
}

void render_update()
{
}

void render_redraw()
{
    auto pData = screenBufferData->buffer;

    // This function called every frame, so lets make something that moves
    static float offset = .1f;
    offset += .005f;

    for (int y = 0; y < screenBufferData->BufferHeight; y++)
    {
        for (int x = 0; x < screenBufferData->BufferWidth; x++)
        {
            float xf = x / (float)screenBufferData->BufferWidth;
            float yf = y / (float)screenBufferData->BufferHeight;

            xf += offset;
            yf += offset;

            // Access a single value
            auto at = [pData](int x, int y) -> glm::vec4& 
            {
                return pData[y * screenBufferData->BufferWidth + x];
            };

            // Fill with a color gradient...
            //at(x, y) = glm::vec4(xf, yf, 1.0f, 1.0f);
            // or .. some noise
            // NOTE: This is CPU rendering, and unoptimized at that.
            // If you want speed, you need to think about how uch work you are doing per pixel, and 
            // if you could thread the rendering to parallelize it (see the raytrace sample)
            at(x, y) = glm::vec4(glm::perlin(glm::vec2(xf * 4, yf * 4)),
                glm::perlin(glm::vec2(xf * 6, yf * 6)),
                glm::perlin(glm::vec2(xf * 12, yf * 12)), 1.0f);
        }
    }

    device_buffer_set_to_display(screenBufferData);
}

void render_resized(int x, int y)
{
    if (!screenBufferData)
    {
        screenBufferData = device_buffer_create();
    }
    device_buffer_ensure_screen_size(screenBufferData);
}

void render_key_pressed(char key)
{
    if (key == 'b')
    {
        auto pBitmap = bitmap_create_from_buffer(screenBufferData->buffer, screenBufferData->BufferWidth, screenBufferData->BufferHeight);
        bitmap_write(pBitmap, "empty_out.bmp");
    }
    else if (key == '+')
    {
        deviceParams.zoomFactor += .1f;
    }
    else if (key == '-')
    {
        deviceParams.zoomFactor -= .1f;
    }
    else if (key == 'd')
    {
        deviceParams.offset.x += .1f;
    }
    else if (key == 'a')
    {
        deviceParams.offset.x -= .1f;
    }
}

void render_key_down(char key)
{
}

void render_mouse_move(const glm::vec2& pos)
{
}

void render_mouse_down(const glm::vec2& pos)
{
}

void render_mouse_up(const glm::vec2& pos)
{
}

