#pragma once
#include "device.h"
#include <cstdio>

// This header implements a simple bitmap object, with writing to a file.
// It doesn't require any windows headers.
struct Color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Bitmap
{
    int width;
    int height;
    Color* pData;
};

static Bitmap* bitmap_create(int width, int height)
{
    Bitmap* pBitmap = (Bitmap*)malloc(sizeof(Bitmap));
    pBitmap->width = width;
    pBitmap->height = height;
    pBitmap->pData = (Color*)malloc(sizeof(Color) * width * height);
    return pBitmap;
}

static Bitmap* bitmap_create_from_buffer(BufferData& data)
{
    auto pBitmap = bitmap_create(data.BufferWidth, data.BufferHeight);

    for (int y = 0; y < int(data.BufferHeight); y++)
    {
        for (auto x = 0; x < int(data.BufferWidth); x++)
        {
            glm::u8vec4* pTarget = (glm::u8vec4*)((uint8_t*)pBitmap->pData + (y * pBitmap->width * sizeof(uint32_t) ) + (x * sizeof(uint32_t)));
            glm::vec4 source = data.buffer[(y * bufferData.BufferWidth) + x];
            source = glm::clamp(source, glm::vec4(0.0f), glm::vec4(1.0f));
            source = glm::u8vec4(source * 255.0f);
            *pTarget = source;
        }
    }
    return pBitmap;
}

static void bitmap_destroy(Bitmap* pBitmap)
{
    if (pBitmap)
    {
        free(pBitmap->pData);
        free(pBitmap);
    }
}

// Returns a dummy pixel for out of bounds
static inline Color& bitmap_get_pixel(Bitmap* pBitmap, int x, int y)
{
    if (x >= pBitmap->width ||
        y >= pBitmap->height ||
        x < 0 ||
        y < 0)
    {
        assert(!"GetPixel out of bounds");
        static Color empty;
        return empty;
    }

    Color& col = pBitmap->pData[(pBitmap->width * y) + x];
    return col;
}

// Ignores out of bounds pixels
static inline void bitmap_put_pixel(Bitmap* pBitmap, int x, int y, const Color& color)
{
#ifdef DEBUG
    if (x >= pBitmap->width ||
        y >= pBitmap->height || 
        x < 0 ||
        y < 0)
    {
        assert(!"PutPixel out of bounds");
        return;
    }
#endif
    Color& col = pBitmap->pData[(pBitmap->width * y) + x];
    col.red = color.red;
    col.green = color.green;
    col.blue = color.blue;
}

static void bitmap_set_color(Bitmap* pBitmap, const Color& color)
{
    for (int y = 0; y < pBitmap->height; y++)
    {
        for (int x = 0; x < pBitmap->width; x++)
        {
            bitmap_put_pixel(pBitmap, x, y, color);
        }
    }
}
/*
This rather hacky function to write a bitmap is taken from here.
Just give it the size of your array and the RGB (24Bit)
https://en.wikipedia.org/wiki/User:Evercat/Buddhabrot.c
*/
static void write_bitmap(Bitmap* pBitmap, char * filename)
{
    uint32_t headers[13];
    int extrabytes;
    int paddedsize;
    int x; int y; int n;

    extrabytes = 4 - ((pBitmap->width * 3) % 4);                 // How many bytes of padding to add to each
                                                        // horizontal line - the size of which must
                                                        // be a multiple of 4 bytes.
    if (extrabytes == 4)
        extrabytes = 0;

    paddedsize = ((pBitmap->width * 3) + extrabytes) * pBitmap->height;

    // Headers...
    // Note that the "BM" identifier in bytes 0 and 1 is NOT included in these "headers".

    headers[0] = paddedsize + 54;      // bfSize (whole file size)
    headers[1] = 0;                    // bfReserved (both)
    headers[2] = 54;                   // bfOffbits
    headers[3] = 40;                   // biSize
    headers[4] = pBitmap->width;  // biWidth
    headers[5] = pBitmap->height; // biHeight

                         // Would have biPlanes and biBitCount in position 6, but they're shorts.
                         // It's easier to write them out separately (see below) than pretend
                         // they're a single int, especially with endian issues...

    headers[7] = 0;                    // biCompression
    headers[8] = paddedsize;           // biSizeImage
    headers[9] = 0;                    // biXPelsPerMeter
    headers[10] = 0;                    // biYPelsPerMeter
    headers[11] = 0;                    // biClrUsed
    headers[12] = 0;                    // biClrImportant

    FILE* outfile = nullptr;
    if (fopen_s(&outfile, filename, "wb") != 0)
    {
        assert(!"Failed to write bitmap file!");
        return;
    }

    //
    // Headers begin...
    // When printing ints and shorts, we write out 1 character at a time to avoid endian issues.
    //

    fprintf(outfile, "BM");

    for (n = 0; n <= 5; n++)
    {
        fprintf(outfile, "%c", headers[n] & 0x000000FF);
        fprintf(outfile, "%c", (headers[n] & 0x0000FF00) >> 8);
        fprintf(outfile, "%c", (headers[n] & 0x00FF0000) >> 16);
        fprintf(outfile, "%c", (headers[n] & (uint32_t)0xFF000000) >> 24);
    }

    // These next 4 characters are for the biPlanes and biBitCount fields.

    fprintf(outfile, "%c", 1);
    fprintf(outfile, "%c", 0);
    fprintf(outfile, "%c", 24);
    fprintf(outfile, "%c", 0);

    for (n = 7; n <= 12; n++)
    {
        fprintf(outfile, "%c", headers[n] & 0x000000FF);
        fprintf(outfile, "%c", (headers[n] & 0x0000FF00) >> 8);
        fprintf(outfile, "%c", (headers[n] & 0x00FF0000) >> 16);
        fprintf(outfile, "%c", (headers[n] & (uint32_t)0xFF000000) >> 24);
    }

    //
    // Headers done, now write the data...
    //

    for (y = pBitmap->height - 1; y >= 0; y--)     // BMP image format is written from bottom to top...
    {
        for (x = 0; x <= pBitmap->width - 1; x++)
        {
            // Also, it's written in (b,g,r) format...
            Color& col = bitmap_get_pixel(pBitmap, x, y);
            fprintf(outfile, "%c", col.blue);
            fprintf(outfile, "%c", col.green);
            fprintf(outfile, "%c", col.red);
        }
        if (extrabytes)      // See above - BMP lines must be of lengths divisible by 4.
        {
            for (n = 1; n <= extrabytes; n++)
            {
                fprintf(outfile, "%c", 0);
            }
        }
    }

    fclose(outfile);
    return;
}
