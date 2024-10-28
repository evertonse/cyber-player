
#include <stdio.h>
#include "raylib.h"
#include "rlgl.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#ifndef NULL
#define NULL 0
#endif

#include <sys/stat.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>

float get_value(float x, float lox, float hix) {
    if (x < lox) x = lox;
    if (x > hix) x = hix;
    x -= lox;
    x /= hix - lox;
    return x;
}

static Shader circle = { -1, NULL};
static int circle_radius_location;
static int circle_power_location;




int needs_rebuild_by_time(const char *input_path, /*inout*/ int* old_time) {
    struct stat statbuf = {0};
    if (stat(input_path, &statbuf) < 0) {
        // NOTE: non-existing input is an error cause it is needed for building in the first place
        nob_log(NOB_ERROR, "could not stat %s: %s", input_path, strerror(errno));
        exit(69);
        return -1;
    }

    int input_path_time = statbuf.st_mtime;
    // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
    if (input_path_time > *old_time) {
        printf("input_path_time=%d *old_time=%d\n",input_path_time, *old_time);
        *old_time = input_path_time;
        return 1;
    }
    return 0;
}

void GuiMoDrawCircle(Vector2 center, float radius, int borderWidth, Color border, Color color) {
    static const char* circle_src =
    "   #version 330                                                                        \n"
    "   in vec2 fragTexCoord;                                                               \n"
    "   in vec4 fragColor;                                                                  \n"
    "   out vec4 finalColor;                                                                \n"
    "   void main() {                                                                       \n"
    "       float usedPower = 0.95f;                                                        \n"
    "       float r = 0.48;                                                                 \n"
    "       vec2 p = fragTexCoord - vec2(0.5);                                              \n"
    "       if (length(p) <= 0.5) {                                                         \n"
    "           float s = length(p) - r;                                                    \n"
    "           if (s <= 0) {                                                               \n"
    "               finalColor = fragColor * 1.5;                                           \n"
    "           } else {                                                                    \n"
    "               float t = 1 - s / (0.5 - r);                                            \n"
    "               finalColor =                                                            \n"
    "                   mix(vec4(fragColor.xyz, 0), fragColor * 1.5, pow(t, usedPower));    \n"
    "           }                                                                           \n"
    "       } else {                                                                        \n"
    "           finalColor = vec4(0);                                                       \n"
    "       }                                                                               \n"
    "   }                                                                                   \n";
    static Shader circle = { 0 };
    if (!IsShaderReady(circle)) {
        circle = LoadShaderFromMemory(0, circle_src);
        assert(IsShaderReady(circle));
    }
    

    BeginShaderMode(circle);
    Rectangle source  = {0, 0, 1, 1};
    Texture2D texture = {rlGetTextureIdDefault(), source.width, source.height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture2D shapeTex = GetShapesTexture();
    printf("texture.id=%d\n", texture.id);
    printf("shapeTex.id=%d\n", shapeTex.id);
    printf("GetShapesTexture().id=%d\n", GetShapesTexture().id);
    
    Vector2   origin  = {0, 0};

    {
        {
            Rectangle dest = {
                center.x - radius,
                center.y - radius,
                radius * 2,
                radius * 2
            };
            DrawTexturePro(texture, source, dest, origin, 0, border);
        }
        {
            Rectangle dest = {center.x - (radius - borderWidth),
                              center.y - (radius - borderWidth),
                              (radius - borderWidth) * 2,
                              (radius - borderWidth) * 2
            };
            DrawTexturePro(texture, source, dest, origin, 0, color);
        }
    }

    EndShaderMode();
}

bool GuiMoHorzSlider(Rectangle bounds, float *value, bool *dragging) {

    bool updated = false;

    Vector2 mouse = GetMousePosition();

    Vector2 startPos = {
        .x = bounds.x + bounds.height/2,
        .y = bounds.y + bounds.height/2,
    };
    Vector2 endPos = {
        .x = bounds.x + bounds.width - bounds.height/2,
        .y = bounds.y + bounds.height/2,
    };
    Color color = WHITE;
    DrawLineEx(startPos, endPos, bounds.height*0.10, color);
    Vector2 center = {
        .x = startPos.x + (endPos.x - startPos.x)*(*value),
        .y = startPos.y,
    };
    float radius = bounds.height/4;
    {
        if (IsShaderReady(circle)) {
            SetShaderValue(circle, circle_radius_location, (float[1]){0.43f}, SHADER_UNIFORM_FLOAT);
            SetShaderValue(circle, circle_power_location, (float[1]){2.0f}, SHADER_UNIFORM_FLOAT);
            BeginShaderMode(circle);
            Rectangle source = {0, 0, 1, 1};
            Rectangle dest = {center.x - radius, center.y - radius, radius * 2,
                              radius * 2};
            Vector2 origin = {0};

            Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
            DrawTexturePro(texture, source, dest, origin, 0, color);
            EndShaderMode();
        }
    }

    if (!*dragging) {
        if (CheckCollisionPointCircle(mouse, center, radius)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                *dragging = true;
            }
        } else {
            if (CheckCollisionPointRec(mouse, bounds)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    *value = get_value(mouse.x, startPos.x, endPos.x);
                    updated = true;
                    *dragging = true;
                }
            }
        }
    } else {
        *value = get_value(mouse.x, startPos.x, endPos.x);
        updated = true;

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            *dragging = false;
        }
    }
    return updated;
}

Rectangle MiddleOf(Rectangle source, float width, float height) {
    return CLITERAL(Rectangle){
        source.x + source.width/2.0 - width/2.0,
        source.y + source.height/2.0 - height/2.0,
        width, height
    };
}

Vector2 MiddleOfV(Rectangle source) {
    return CLITERAL(Vector2){
        source.x + source.width/2.0,
        source.y + source.height/2.0
    };
}

#define CIRCLE_SHADER "./res/shaders/circle.fs"

void StartShader(void) {
    if (IsShaderReady(circle)) {
    };
    UnloadShader(circle);
    circle = LoadShader(0, CIRCLE_SHADER);
    circle_radius_location = GetShaderLocation(circle, "radius");
    circle_power_location = GetShaderLocation(circle, "power");
    printf("circle_power_location=%d\n", circle_power_location);
    printf("circle_radius_location=%d\n", circle_radius_location);
    printf("circle.id=%d\n", circle.id);

    if (!IsShaderReady(circle)) {
        circle = LoadShader(0, 0);
        return;
    }


    SetShaderValue(circle, circle_radius_location, (float[1]){ 0.07f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(circle, circle_power_location, (float[1]){ 5.0f }, SHADER_UNIFORM_FLOAT);

}

int main(int argc, char *argv[])
{

    InitWindow(900, 900,"Circle");

    StartShader();

    bool drag = false;
    int rebuild_time = 0;
    float value = 20.5;
    while (!WindowShouldClose()) {

        if (needs_rebuild_by_time(CIRCLE_SHADER, &rebuild_time) || IsKeyPressed(KEY_R)) {
            StartShader();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        {
            GuiMoDrawCircle((Vector2){GetScreenWidth()/2.0, GetMouseY()}, 100.f, 50.0, BLUE, RED);
            GuiMoDrawCircle((Vector2){GetScreenWidth()/2.0, GetMouseY()/2}, 50.f, 1.0, PINK, YELLOW);
            Rectangle bounds = MiddleOf((Rectangle){0,0, GetScreenWidth(), GetScreenHeight()}, 300, 300);
            // printf(
            //     "bounds.x=%.2f bounds.y=%.2f bounds.width=%.2f, "
            //     "bounds.height==%.2f\n",
            //     bounds.x, bounds.y, bounds.width, bounds.height);

            DrawRectangle(bounds.x, bounds.y, bounds.width, bounds.height+bounds.height, Fade(RED, 0.4));
            Vector2 center = MiddleOfV(bounds);
            DrawCircleV(center, 40, RED);

            bounds.y += bounds.height;

            SetShaderValue(circle, circle_radius_location, (float[1]){ 0.07f }, SHADER_UNIFORM_FLOAT);
            GuiMoHorzSlider(bounds, &value, &drag);

            bounds.x += bounds.height;
            SetShaderValue(circle, circle_radius_location, (float[1]){ 0.01f }, SHADER_UNIFORM_FLOAT);

            GuiMoHorzSlider(bounds, &value, &drag);
        }
        EndDrawing();
    }

    
}
