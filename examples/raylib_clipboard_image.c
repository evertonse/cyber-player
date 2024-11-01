#include <stdio.h>
#include "raylib.h"
#include <stdlib.h>

static Image img = {0};
int main(int argc, char *argv[]) {

    InitWindow(800, 450, "[core] raylib clipboard image");
    SetTraceLogLevel(LOG_TRACE);
    SetTargetFPS(60);

    Texture tex = {0};
    while(!WindowShouldClose()) {
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
            img = GetClipboardImage();
            // UnloadImage(img);
            tex = LoadTextureFromImage(img);
            if(!IsTextureReady(tex)) {
                exit(98);
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            if (IsTextureReady(tex)) {
                DrawTexture(tex, 0, 0, WHITE);
            }
            DrawText("Print Screen and Crtl+V", 10, 10, 21, BLACK);
        EndDrawing();
    }
}
