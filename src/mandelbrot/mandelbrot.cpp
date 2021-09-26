#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <complex>
#include <algorithm>

#include "device.h"
#include "bitmap_utils.h"

BufferData* screenBufferData;

std::complex<double> TopLeft = std::complex<double>(-2.0f, -1.0f);
std::complex<double> BottomRight = std::complex<double>(2.0f, 1.0f);

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

std::complex<double> screen_to_complex(int x, int y)
{
    double xf = x / (double)screenBufferData->BufferWidth;
    double yf = y / (double)screenBufferData->BufferHeight;
    return std::complex<double>(xf * (real(BottomRight) - real(TopLeft)) + real(TopLeft),
        yf * (imag(BottomRight) - imag(TopLeft)) + imag(TopLeft));
}
void render_redraw()
{
    auto pData = screenBufferData->buffer;
    
    // Access a single value
    auto at = [pData](int x, int y) -> glm::vec4&
    {
        return pData[y * screenBufferData->BufferWidth + x];
    };


    auto scale = float(std::abs(std::hypot(real(BottomRight - TopLeft), imag(BottomRight - TopLeft))));
    int partitions = 32;
    std::vector<std::thread> threads;
    for (int t = 0; t < partitions; t++)
    {
        threads.push_back(std::thread([=]() {
            for (int y = t; y < screenBufferData->BufferHeight; y+=partitions)
            {
                for (int x = 0; x < screenBufferData->BufferWidth; x++)
                {
                    auto c = screen_to_complex(x, y);
                    auto current = std::complex<double>(0.0f, 0.0f);
                    int i = 0;
                    while (i < 1000)
                    {
                        current = current * current + c;
                        if (std::norm(current) > 4.0)
                            break;
                        i++;
                    }

                    if (i < 100)
                    {
                        at(x, y) = glm::vec4(std::min(1.0f, i / 10.0f), std::min(1.0f, i / 100.0f), 1.0f - std::min(1.0f, i / 20.0f), 1.0f);
                    }
                    else
                    {
                        at(x, y) = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    }
                }
            }
            }));
    }
    
    for (auto& t : threads)
    {
        t.join();
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
    }
    else if (key == '-')
    {
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
    auto range = BottomRight - TopLeft;
    auto c = screen_to_complex(int(pos.x), int(pos.y));

    if (!right)
    {
        TopLeft = c - range * .33;
        BottomRight = c + range * .33;
    }
    else
    {
        TopLeft = c - range ;
        BottomRight = c + range;
    }
    
    auto cNew = screen_to_complex(int(pos.x), int(pos.y));
    auto diff = c - cNew;
    TopLeft += diff;
    BottomRight += diff;
}

void render_mouse_up(const glm::vec2& pos)
{
}

