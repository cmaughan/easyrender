#include <windows.h>
#include <windowsx.h>

#include <gdiplus.h>
#include <memory>
#include <cassert>
#include "common.h"

#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;

#include <chrono>

#include "device.h"
#include "render.h"

BufferData bufferData;

namespace
{
HWND hWnd;
std::shared_ptr<Bitmap> spBitmap;
}

void device_copy_buffer()
{
    if (spBitmap)
    {
        auto pData = &bufferData.buffer[0];

        BitmapData writeData;
        Rect lockRect(0, 0, bufferData.BufferWidth, bufferData.BufferHeight);
        if (spBitmap->LockBits(&lockRect, ImageLockModeWrite, PixelFormat32bppARGB, &writeData) == 0)
        {
            for (int y = 0; y < int(writeData.Height); y++)
            {
                for (auto x = 0; x < int(writeData.Width); x++)
                {
                    glm::u8vec4* pTarget = (glm::u8vec4*)((uint8_t*)writeData.Scan0 + (y * writeData.Stride) + (x * 4));
                    glm::vec4 source = bufferData.buffer[(y * bufferData.BufferWidth) + x];
                    source = glm::clamp(source, glm::vec4(0.0f), glm::vec4(1.0f));

                    source = glm::u8vec4(source * 255.0f);
                    *pTarget = source;
                }
            }

            spBitmap->UnlockBits(&writeData);
        }
    }
    InvalidateRect(hWnd, NULL, TRUE);
}

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);

    graphics.SetSmoothingMode(SmoothingMode::SmoothingModeDefault);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    RectF dest;
    dest.X = 0;
    dest.Y = 0;
    dest.Width = float(bufferData.BufferWidth);
    dest.Height = float(bufferData.BufferHeight);
    if (spBitmap)
    {
        graphics.DrawImage(spBitmap.get(), dest, 0.0f, 0.0f, float(bufferData.BufferWidth), float(bufferData.BufferHeight), Unit(UnitPixel));
    }
    else
    {
        graphics.Clear(Color::Beige);
    }
}

void size_changed()
{
    RECT rc;
    GetClientRect(hWnd, &rc);

    int x = rc.right - rc.left;
    int y = rc.bottom - rc.top;
    
    bufferData.BufferWidth = x;
    bufferData.BufferHeight = y;

    spBitmap = std::make_shared<Bitmap>(bufferData.BufferWidth, bufferData.BufferHeight, PixelFormat32bppPARGB);
    bufferData.buffer.resize(bufferData.BufferWidth * bufferData.BufferHeight, glm::vec4(0));

    render_resized();
}

bool device_is_key_down(DeviceKeyType type)
{
    if (type == DeviceKeyType::Ctrl)
    {
        return GetAsyncKeyState(VK_LCONTROL) & 0x8000;
    }
    return false;
}

//            SetWindowTextA(hWnd, std::to_string(currentSample).c_str());
LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (message)
    {
    case WM_CHAR:
    {
        render_key_pressed(char(wParam));
    }
    break;

    case WM_KEYDOWN:
    {
        render_key_down(char(wParam));
    }
    break;

    case WM_SYSCOMMAND:
    {
        switch (LOWORD(wParam))
        {
        case SC_MAXIMIZE:
            spBitmap.reset();
            break;
        default:
            break;
        }
    }
    break;

    case WM_MOUSEMOVE:
    {
        auto xPos = GET_X_LPARAM(lParam);
        auto yPos = GET_Y_LPARAM(lParam);
        render_mouse_move(glm::vec2(xPos, yPos));
    }
    break;

    case WM_LBUTTONDOWN:
    {
        auto xPos = GET_X_LPARAM(lParam);
        auto yPos = GET_Y_LPARAM(lParam);
        render_mouse_down(glm::vec2(xPos, yPos));
        SetCapture(hWnd);
    }
    break;

    case WM_LBUTTONUP:
    {
        auto xPos = GET_X_LPARAM(lParam);
        auto yPos = GET_Y_LPARAM(lParam);
        render_mouse_up(glm::vec2(xPos, yPos));
        ReleaseCapture();
    }
    break;

    case WM_SIZE:
    {
        spBitmap.reset();
    }
    break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        spBitmap.reset();
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
} // WndProc

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    MSG                 msg;
    WNDCLASS            wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;

    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("RayTracer");

    RegisterClass(&wndClass);

    // Make a window in the center of the screen, with an square client rect
    const int WindowSize = 500;
    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    hWnd = CreateWindow(
        TEXT("RayTracer"),   // window class name
        TEXT("RayTracer"),  // window caption
        WS_OVERLAPPEDWINDOW,      // window style
        (nScreenWidth / 2) - (WindowSize / 2),            // initial x position
        (nScreenHeight / 2) - (WindowSize / 2),            // initial y position
        WindowSize,            // initial x size
        WindowSize + GetSystemMetrics(SM_CYCAPTION),            // initial y size
        NULL,                     // parent window handle
        NULL,                     // window menu handle
        hInstance,                // program instance handle
        NULL);                    // creation parameters

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    render_init();

    msg.message = 0;
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
        {
            //Translate message
            TranslateMessage(&msg);

            //Dispatch message
            DispatchMessage(&msg);
        }
        else
        {
            if (spBitmap == nullptr)
            {
                size_changed();
            }
            render_redraw();
        }
    }

    GdiplusShutdown(gdiplusToken);

    return 0;
}  // WinMain
