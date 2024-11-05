#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#define IsTextureValid IsTextureReady

// bin/x86_64-w64-mingw32-gcc ./examples/raylib_clipboard_image.c -I ./src/vendor/raylib/src -L./ -l:./concat.a -lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 -lversion -lsetupapi -lwinmm -Wl,--allow-shlib-undefined -Wl,--unresolved-symbols=ignore-all -luuid

int formats[] = {
    PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
    PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,    // 8*2 bpp (2 channels)
    PIXELFORMAT_UNCOMPRESSED_R5G6B5,        // 16 bpp
    PIXELFORMAT_UNCOMPRESSED_R8G8B8,        // 24 bpp
    PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,      // 16 bpp (1 bit alpha)
    PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,      // 16 bpp (4 bit alpha)
    PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,      // 32 bpp
    PIXELFORMAT_UNCOMPRESSED_R32,           // 32 bpp (1 channel - float)
    // PIXELFORMAT_UNCOMPRESSED_R32G32B32,     // 32*3 bpp (3 channels - float)
    // PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,  // 32*4 bpp (4 channels - float)
    PIXELFORMAT_UNCOMPRESSED_R16,           // 16 bpp (1 channel - half float)
    PIXELFORMAT_UNCOMPRESSED_R16G16B16,     // 16*3 bpp (3 channels - half float)
    // PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,  // 16*4 bpp (4 channels - half float)
    PIXELFORMAT_COMPRESSED_DXT1_RGB,        // 4 bpp (no alpha)
    PIXELFORMAT_COMPRESSED_DXT1_RGBA,       // 4 bpp (1 bit alpha)
    PIXELFORMAT_COMPRESSED_DXT3_RGBA,       // 8 bpp
    PIXELFORMAT_COMPRESSED_DXT5_RGBA,       // 8 bpp
    PIXELFORMAT_COMPRESSED_ETC1_RGB,        // 4 bpp
    PIXELFORMAT_COMPRESSED_ETC2_RGB,        // 4 bpp
    PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,   // 8 bpp
    PIXELFORMAT_COMPRESSED_PVRT_RGB,        // 4 bpp
    PIXELFORMAT_COMPRESSED_PVRT_RGBA,       // 4 bpp
    PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,   // 8 bpp
    PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA    // 2 bpp
};

int curr = 0 % (24);

static Image img = {0};
int main(int argc, char *argv[]) {
    InitWindow(800, 450, "[core] raylib clipboard image");
    SetTargetFPS(60);
    Texture tex = {0};
    while(!WindowShouldClose()) {
        if (IsKeyPressed(KEY_H)) {
            curr = (curr-1) % (24);
            img.format = formats[curr];
            printf("curr=%d\n", curr);
        } else if (IsKeyPressed(KEY_L)) {
            printf("img.format=%d\n", img.format);
            curr = (curr+1) % (24);
            img.format = formats[curr];
        }


        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
            ImageColorContrast(&img, 40);
            // SetClipboardImage(img);
        } else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V) || IsKeyPressed(KEY_V)) {
            #ifdef _WIN32
            img = GetClipboardImage();
            tex = LoadTextureFromImage(img);
            if(!IsTextureValid(tex)) {
                static float time  = 0;
                DrawText("Invalid", 10, GetScreenHeight() -21, 21, RED);
                // exit(98);
            } else {
                ExportImage(img, "Debug.bmp");
            }
            #endif
        }

        BeginDrawing();
            ClearBackground(GetColor(0xeeeeeeFF));
            if (IsImageReady(img)) {
                tex = LoadTextureFromImage(img);
            }
            if (IsTextureValid(tex)) {
                DrawTexture(tex, 0, 10 + 21, WHITE);
            }
            DrawText("Print Screen and Crtl+V and Crtl+C to copy gray image to clipbaord", 10, 10, 21, BLACK);
        EndDrawing();
    }
}


