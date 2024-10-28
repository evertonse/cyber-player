// main.c
#include <stdio.h>
#include "raylib.h"

static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 600;

int count = 5;
float spacing = 4;


int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "FXAA Shader Example");
    SetTargetFPS(60);

    // Load shader
    // Shader fxaaShader = LoadShader(0, "./res/shaders/fxaa.fs");
    Shader fxaaShader = LoadShader(0, "./res/shaders/fxaa-thebennybox.fs");
    // Shader fxaaShader = LoadShader(0, 0);
    TraceLog(LOG_INFO, "fxaaShader.id=%d\n", fxaaShader.id);
    TraceLog(LOG_INFO, "*fxaaShader.locs=%d\n", *fxaaShader.locs);
    // Shader fxaaShader = LoadShader(0, 0);
    // Create render texture to apply shader
    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Get shader uniform locations
    int resolutionLoc = GetShaderLocation(fxaaShader, "resolution");

    // Create some example geometry to show anti-aliasing
    Vector3 cubePosition = { SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f, 0.0f };
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        // Rotate cub
        Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(fxaaShader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
        cubePosition.z += 0.5f;
        
        // Draw scene to render texture
        BeginTextureMode(target);
            ClearBackground(SKYBLUE);
            BeginMode3D(camera);
            
                // Draw scene: grid of cube trees on a plane to make a "world"
                DrawPlane((Vector3){ 0, 0, 0 }, (Vector2){ 50, 50 }, BEIGE); // Simple world plane

                for (float x = -count*spacing; x <= count*spacing; x += spacing)
                {
                    for (float z = -count*spacing; z <= count*spacing; z += spacing)
                    {
                        DrawCube((Vector3) { x, 1.5f, z }, 1, 1, 1, LIME);
                        DrawCube((Vector3) { x, 0.5f, z }, 0.25f, 1, 0.25f, BROWN);
                    }
                }

                // Draw a cube at each player's position
                DrawCube(camera.position, 1, 1, 1, RED);
                
            EndMode3D();
            
            Rectangle rec = {GetScreenWidth()/2.0, 50 + GetScreenHeight()/2.0, 400, 400};
            DrawRectangleRoundedGradientH(rec, 0.5, 1.0, 36, Fade(RAYWHITE, 0.8f), RED);

            DrawRectangle(0, 0, GetScreenWidth()/2, 40, Fade(RAYWHITE, 0.8f));
            DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 50, Fade(RAYWHITE, 0.8f));


            // ClearBackground(RAYWHITE);
            // BeginMode3D(camera);
                // DrawCube(cubePosition, 1.0f, 1.0f, 1.0f, RED);
            //     DrawCubeWires(cubePosition, 1.0f, 1.0f, 1.0f, MAROON);
            //     DrawGrid(10, 1.0f);
        EndTextureMode();
        
        // Draw render texture to screen using FXAA shader
        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginShaderMode(fxaaShader);
                DrawTextureRec(target.texture,
                    (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height },
                    (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            
            DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadShader(fxaaShader);
    CloseWindow();
    return 0;
}

int main2(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "FXAA Shader Example");
    SetTargetFPS(60);

    // Load shader
    // Shader fxaaShader = LoadShader(0, "./res/shaders/fxaa.fs");
    Shader fxaaShader = LoadShader(0, "./res/shaders/fxaa-thebennybox.fs");
    // Shader fxaaShader = LoadShader(0, 0);
    TraceLog(LOG_INFO, "fxaaShader.id=%d\n", fxaaShader.id);
    TraceLog(LOG_INFO, "*fxaaShader.locs=%d\n", *fxaaShader.locs);
    // Shader fxaaShader = LoadShader(1, 0);
    // Create render texture to apply shader
    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Get shader uniform locations
    int resolutionLoc = GetShaderLocation(fxaaShader, "resolution");

    // Create some example geometry to show anti-aliasing
    Vector3 cubePosition = { SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f, 0.0f };
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        // Rotate cub
        Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(fxaaShader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
        cubePosition.z += 0.5f;
        
        // Draw scene to render texture
        BeginTextureMode(target);
            ClearBackground(SKYBLUE);
            BeginMode3D(camera);
            
                // Draw scene: grid of cube trees on a plane to make a "world"
                DrawPlane((Vector3){ 0, 0, 0 }, (Vector2){ 50, 50 }, BEIGE); // Simple world plane

                for (float x = -count*spacing; x <= count*spacing; x += spacing)
                {
                    for (float z = -count*spacing; z <= count*spacing; z += spacing)
                    {
                        DrawCube((Vector3) { x, 1.5f, z }, 1, 1, 1, LIME);
                        DrawCube((Vector3) { x, 0.5f, z }, 0.25f, 1, 0.25f, BROWN);
                    }
                }

                // Draw a cube at each player's position
                DrawCube(camera.position, 1, 1, 1, RED);
                
            EndMode3D();
            
            Rectangle rec = {GetScreenWidth()/2.0, 50 + GetScreenHeight()/2.0, 400, 400};
            DrawRectangleRoundedGradientH(rec, 0.5, 1.0, 36, Fade(RAYWHITE, 0.8f), RED);

            DrawRectangle(0, 0, GetScreenWidth()/2, 40, Fade(RAYWHITE, 0.8f));
            DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 50, Fade(RAYWHITE, 0.8f));


            // ClearBackground(RAYWHITE);
            // BeginMode3D(camera);
                // DrawCube(cubePosition, 1.0f, 1.0f, 1.0f, RED);
            //     DrawCubeWires(cubePosition, 1.0f, 1.0f, 1.0f, MAROON);
            //     DrawGrid(10, 1.0f);
        EndTextureMode();
        
        // Draw render texture to screen using FXAA shader
        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginShaderMode(fxaaShader);
                DrawTextureRec(target.texture,
                    (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height },
                    (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            
            DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadShader(fxaaShader);
    CloseWindow();
    return 0;
}
