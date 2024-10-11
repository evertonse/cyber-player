#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpv/client.h>
#include <mpv/render.h>
#include <raylib.h>

static void on_mpv_events(void *ctx) {
    // This function can be used to handle mpv events if needed
}

static void on_mpv_render_update(void *ctx) {
    // This function can be used to handle render updates if needed
}

void die(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        die("pass a single media file as argument");

    mpv_handle *mpv = mpv_create();
    if (!mpv)

        die("context init failed");

    mpv_set_option_string(mpv, "vo", "libmpv");


    if (mpv_initialize(mpv) < 0)
        die("mpv init failed");

    mpv_request_log_messages(mpv, "debug");

    // Initialize raylib
    InitWindow(1000, 500, "Video Player with raylib");
    SetTargetFPS(60); // Set the target frames per second

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &(mpv_opengl_init_params){
            .get_proc_address = get_proc_address_mpv,
        }},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}},
        {0}
    };

    mpv_render_context *mpv_gl;
    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        die("failed to initialize mpv GL context");

    // Play this file.
    const char *cmd[] = {"loadfile", argv[1], NULL};

    mpv_command_async(mpv, 0, cmd);

    while (!WindowShouldClose()) {
        // Handle input

        if (IsKeyPressed(KEY_SPACE)) {
            const char *cmd_pause[] = {"cycle", "pause", NULL};
            mpv_command_async(mpv, 0, cmd_pause);
        }
        if (IsKeyPressed(KEY_S)) {
            const char *cmd_scr[] = {"screenshot-to-file", "screenshot.png", "window", NULL};
            printf("attempting to save screenshot to %s\n", cmd_scr[1]);
            mpv_command_async(mpv, 0, cmd_scr);
        }

        // Check for render updates
        uint64_t flags = mpv_render_context_update(mpv_gl);
        if (flags & MPV_RENDER_UPDATE_FRAME) {

            int w, h;
            GetWindowSize(&w, &h);
            mpv_render_param render_params[] = {
                {MPV_RENDER_PARAM_OPENGL_FBO, &(mpv_opengl_fbo){.fbo = 0, .w = w, .h = h}},
                {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
                {0}
            };
            mpv_render_context_render(mpv_gl, render_params);
        }

        // Draw the frame
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Additional drawing can be done here if needed

        EndDrawing();
    }

    // Cleanup
    mpv_render_context_free(mpv_gl);
    mpv_destroy(mpv);
    CloseWindow(); // Close window and OpenGL context

    printf("properly terminated\n");
    return 0;
}

