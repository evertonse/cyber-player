#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "RenderTexture Blend Example");
    SetTargetFPS(60);

    // Create a render texture that will support transparency
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);

    // Important: Clear the render texture with transparent black
    BeginTextureMode(target);
        ClearBackground(BLANK); // BLANK is (0,0,0,0) - fully transparent
    EndTextureMode();

    while (!WindowShouldClose()) {
        // Update the render texture only when needed
        // BeginTextureMode(target);
        //     // Draw only where you want content, the rest stays transparent
        //
        //     Rectangle bounds = {100, 100, 200, 150};
        //     DrawRectangleRec(bounds, ColorAlpha(BLUE, 0.5f)); // Semi-transparent blue
        //     DrawCircle(GetMouseX(), GetMouseY(), 20, RED);
        //
        //     bounds.x += bounds.width;
        //     DrawRectangleRec(bounds, ColorAlpha(BLACK, 0.5f)); // Semi-transparent blue
        //
        // EndTextureMode();

        BeginDrawing();
            // First draw background/direct content
            ClearBackground(RAYWHITE);

            // Draw some background elements that will show through transparent areas
            DrawText("This is drawn directly to screen", 250, 200, 20, DARKGRAY);
            DrawCircle(400, 300, 100, GREEN);

            // Draw UI or overlays
            DrawFPS(10, 10);

            BeginTextureMode(target);
                ClearBackground(BLANK); // BLANK is (0,0,0,0) - fully transparent
                // Draw only where you want content, the rest stays transparent

                Rectangle bounds = {100, 100, 200, 150};
                DrawRectangleRec(bounds, ColorAlpha(BLUE, 0.5f)); // Semi-transparent blue
                DrawCircle(GetMouseX(), GetMouseY(), 20, RED);

                bounds.x += bounds.width;
                DrawRectangleRec(bounds, ColorAlpha(BLACK, 0.5f)); // Semi-transparent blue
            EndTextureMode();

            // Draw the render texture with blending
            // Note: we flip the texture vertically because RenderTextures are inverted
            DrawTextureRec(target.texture,
                          (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height },
                          (Vector2){ 0, 0 },
                          WHITE);  // WHITE preserves alpha channel
        EndDrawing();


        if (IsKeyPressed(KEY_R)) {
            BeginTextureMode(target);
                ClearBackground(BLANK); // BLANK is (0,0,0,0) - fully transparent
            EndTextureMode();
        }


    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
