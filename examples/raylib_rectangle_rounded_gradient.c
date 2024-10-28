#include "raylib.h"

int main(int argc, char *argv[]) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(600, 900, "Rectangle Rounded Gradient");
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    double width = 600/2, height = 900/6;

    while (!WindowShouldClose()) {
        BeginDrawing();
            // ClearBackground(RAYWHITE);
            Rectangle rec = {
                GetScreenWidth() / 2.0 - width/2,
                GetScreenHeight() / 2.0 - (5)*(height/2),
                width, height
            };
            DrawRectangleRoundedGradientH(rec, 0.8, 0.8, 36, BLUE, RED);

            rec.y += rec.height + 1;
            DrawRectangleRoundedGradientH(rec, 0.5, 1.0, 36, RED, PINK);

            rec.y += rec.height + 1;
            DrawRectangleRoundedGradientH(rec, 1.0, 0.5, 36, RED, BLUE);

            rec.y += rec.height + 1;
            DrawRectangleRoundedGradientH(rec, 0.0, 1.0, 36, BLUE, BLACK);

            rec.y += rec.height + 1;
            DrawRectangleRoundedGradientH(rec, 1.0, 0.0, 36, BLUE, PINK);
        EndDrawing();
    }
    return 0;
}
