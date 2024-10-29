#include "raylib.h"
#include "clipboard.h"

int main(int argc, char *argv[])
{
    printf("got here=%s\n", "hi");

    InitWindow(600, 600, "HELLO");
    SetTargetFPS(60);
    while(!WindowShouldClose()) {
        BeginDrawing();
            DrawFPS(10, 10);
        EndDrawing();
    }
}
