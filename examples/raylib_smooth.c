#include "raylib.h"
#include <math.h>

// Structure to store previous mouse position for smooth lines
typedef struct {
    Vector2 position;
    bool active;
} PreviousMouseState;

// Function to draw a smooth line between two points
void DrawSmoothLine(Vector2 start, Vector2 end, float thickness, Color color) {
    Vector2 delta = { end.x - start.x, end.y - start.y };
    float length = sqrtf(delta.x * delta.x + delta.y * delta.y);
    
    if (length > 0) {
        float segments = length / (thickness * 0.5f);
        Vector2 step = { delta.x / segments, delta.y / segments };
        Vector2 current = start;
        
        for (int i = 0; i <= segments; i++) {
            DrawCircle(current.x, current.y, thickness/2, color);
            current.x += step.x;
            current.y += step.y;
        }
    }
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Paint with Transparency Example");
    SetTargetFPS(60);

    RenderTexture2D canvas = LoadRenderTexture(screenWidth, screenHeight);
    
    // Initialize canvas with transparency
    BeginTextureMode(canvas);
        ClearBackground(BLANK);
    EndTextureMode();

    PreviousMouseState prevMouse = { 0 };
    bool isErasing = false;
    float brushSize = 20.0f;
    Color brushColor = RED;
    float alpha = 1.0f;

    while (!WindowShouldClose()) {
        // Handle input
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            prevMouse.position = GetMousePosition();
            prevMouse.active = true;
        }
        
        if (IsKeyPressed(KEY_E)) isErasing = !isErasing;
        
        // Adjust brush size
        brushSize += GetMouseWheelMove() * 2;
        if (brushSize < 1) brushSize = 1;
        if (brushSize > 50) brushSize = 50;
        
        // Adjust transparency
        if (IsKeyDown(KEY_UP)) alpha += 0.02f;
        if (IsKeyDown(KEY_DOWN)) alpha -= 0.02f;
        if (alpha > 1.0f) alpha = 1.0f;
        if (alpha < 0.0f) alpha = 0.0f;

        // Update canvas
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 currentMouse = GetMousePosition();
            
            BeginTextureMode(canvas);
                if (prevMouse.active) {
                    Color currentColor = isErasing ? BLANK : ColorAlpha(brushColor, alpha);
                    DrawSmoothLine(prevMouse.position, currentMouse, brushSize, currentColor);
                }
            EndTextureMode();
            
            prevMouse.position = currentMouse;
            prevMouse.active = true;
        } else {
            prevMouse.active = false;
        }

        // Drawing
        BeginDrawing();
            // Draw background pattern to show transparency
            ClearBackground(RAYWHITE);
            for (int i = 0; i < screenWidth; i += 20) {
                for (int j = 0; j < screenHeight; j += 20) {
                    if ((i + j) % 40 == 0) {
                        DrawRectangle(i, j, 20, 20, LIGHTGRAY);
                    }
                }
            }
            
            // Draw some background elements
            DrawCircle(400, 225, 150, GREEN);
            DrawRectangle(100, 100, 200, 200, BLUE);
            
            // Draw the canvas with transparency
            DrawTextureRec(canvas.texture,
                          (Rectangle){ 0, 0, (float)canvas.texture.width, (float)-canvas.texture.height },
                          (Vector2){ 0, 0 },
                          WHITE);

            // Draw UI
            DrawText(TextFormat("Brush Size: %.0f", brushSize), 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Alpha: %.2f", alpha), 10, 35, 20, DARKGRAY);
            DrawText(isErasing ? "Mode: Eraser (E)" : "Mode: Paint (E)", 10, 60, 20, DARKGRAY);
            DrawText("Use Mouse Wheel to adjust brush size", 10, 85, 20, DARKGRAY);
            DrawText("Up/Down arrows to adjust transparency", 10, 110, 20, DARKGRAY);
            
            // Draw brush preview
            DrawCircleLines(GetMouseX(), GetMouseY(), brushSize/2, isErasing ? RED : brushColor);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
