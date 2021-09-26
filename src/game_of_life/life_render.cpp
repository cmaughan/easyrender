#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/functions.hpp>

#include "device.h"
#include "bitmap_utils.h"

BufferData* screenBufferData;

uint32_t displayLifeBuffer = 0;
std::vector<uint32_t> lifeBuffers[2];

// Get a life value from a life buffer
uint32_t& life_at(uint32_t buffer, int x, int y)
{ 
    if (x < 0) x = screenBufferData->BufferWidth - 1;
    if (y < 0) y = screenBufferData->BufferHeight - 1;
    if (y >= screenBufferData->BufferHeight) y = 0;
    if (x >= screenBufferData->BufferWidth) x = 0;
    return lifeBuffers[buffer][int(screenBufferData->BufferWidth * y + x)]; 
}

void render_init()
{
    deviceParams.pName = "Game Of Life";
}

void render_destroy()
{
    device_buffer_destroy(screenBufferData);
    screenBufferData = nullptr;
}

void render_update()
{
    if (screenBufferData == nullptr)
        return;

    // Get the last not displayed buffer and make it the target
    auto targetBuffer = displayLifeBuffer == 1 ? 0 : 1;
    auto sourceBuffer = displayLifeBuffer;

    // Copy the life data to the new generation
    for (int y = 0; y < screenBufferData->BufferHeight; y++)
    {
        for (int x = 0; x < screenBufferData->BufferWidth; x++)
        {
            int count = 0;
            const int coords[] = { -1, 0, 1 };
           
            // Count surrounds
            for (auto xx : coords)
            {
                for (auto yy : coords)
                {
                    count += life_at(sourceBuffer, xx + x, yy + y);
                }
            }

            // Get center, subract from count
            auto val = life_at(sourceBuffer, x, y);
            count -= val;

            // Less than 2, greater than 3 cell dies
            if (count < 2 || count > 3)
            {
                val = 0;
            }
            // Exactly 3, cell lives
            else if (count == 3)
            {
                val = 1;
            }
            life_at(targetBuffer, x, y) = val;
        }
    }

    // Swap the displayed buffer
    displayLifeBuffer = targetBuffer;
}

void render_redraw()
{
    auto pData = screenBufferData->buffer;

    // Access a single value
    auto at = [pData](int x, int y) -> glm::vec4& 
    {
        return pData[y * screenBufferData->BufferWidth + x];
    };

    // Fill the display buffer with black or white pixels
    for (int y = 0; y < screenBufferData->BufferHeight; y++)
    {
        for (int x = 0; x < screenBufferData->BufferWidth; x++)
        {
            at(x, y) = glm::vec4(glm::vec3(life_at(displayLifeBuffer, x, y) ? 1.0f : 0.0f), 1.0f);
        }
    }

    // Copy the buffer to the display staging area
    device_buffer_set_to_display(screenBufferData);
}

void render_resized(int x, int y)
{
    if (!screenBufferData)
    {
        screenBufferData = device_buffer_create();
    }
    device_buffer_ensure_screen_size(screenBufferData);

    lifeBuffers[0].resize(screenBufferData->BufferWidth * screenBufferData->BufferHeight);
    lifeBuffers[1].resize(screenBufferData->BufferWidth * screenBufferData->BufferHeight);

    // Fill with random
    for (int yy = 0; yy < y; yy++)
    {
        for (int xx = 0; xx < x; xx++)
        {
            life_at(displayLifeBuffer, xx, yy) = (uint32_t)((rand() / (float)RAND_MAX) > .5 ? 1 : 0);

            float fx = xx / (float)screenBufferData->BufferWidth;
            float fy = yy / (float)screenBufferData->BufferHeight;

            // Remove all but the top left corner
            if ((fx * fx + fy * fy) > .5) 
                life_at(displayLifeBuffer, xx, yy) = 0;
        }
    }
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

void render_mouse_down(const glm::vec2& pos, bool right)
{
}

void render_mouse_up(const glm::vec2& pos)
{
}

