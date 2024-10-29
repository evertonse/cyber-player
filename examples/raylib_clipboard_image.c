#include <stdio.h>
#include "raylib.h"
#include "clipboard.h"


static Image img = {0};
int main(int argc, char *argv[]) {
    printf("got here=%s\n", "hi");

    InitWindow(600, 600, "HELLO");

    SetTraceLogLevel(LOG_TRACE);
    SetTargetFPS(60);

    int width, height;
    Texture tex = {0};
    while(!WindowShouldClose()) {
        ClearBackground(WHITE);
        BeginDrawing();
        DrawFPS(10, 10);

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
            long long data_size;
            
            // User would have to call free on the pointer If clipboard is not an image it returns NULL
            char* pixels = GetClipboardImage(&width, &height, &data_size);
            img = LoadImageFromMemory(".bmp", pixels, data_size);
            printf("width=%d\n", width);
            printf("height=%d\n", height);
            tex = LoadTextureFromImage(img);
        }
        bool fine = IsTextureReady(tex);
        if (fine) {
            DrawTexture(tex, 0, 0, WHITE);
        } else {
            TraceLog(LOG_INFO, "fine %d", fine);
        }
        EndDrawing();
    }
}
