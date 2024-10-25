#include "raylib.h"

int main(int argc, char *argv[])
{
    // RLAPI void SetWindowState(unsigned int flags);                    // Set window configuration state using flags (only PLATFORM_DESKTOP)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1600, 900, "title");

    double width = 300, height = 150;
    while (!WindowShouldClose()) {
        BeginDrawing();
        Rectangle rec = {GetScreenWidth()/2.0 - width, GetScreenHeight()/2.0 - height, width, height};

        DrawRectangleRoundedGradientH(rec, 0.5, 1.5, 36, RED, PINK);

        rec.y += rec.height;
        DrawRectangleRoundedGradientH(rec, 0.5, 1.5, 36, RED, BLUE);

        rec.y += rec.height;
        DrawRectangleGradientEx(rec, RED, BLUE, GREEN, PINK);

        rec.y += rec.height;
        DrawRectangleRounded(rec, 0.5, 36, RED);

        EndDrawing();
    }
    return 0;
}
