#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>

// #define PLATFORM_DESKTOP_SDL
#include <raylib.h>
#include <rlgl.h>

#define STB_DS_IMPLEMENTATION
#include "std_ds.h"
// #include "GLFW/glfw3.h"         // Windows/Context and inputs management
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900

#include "rcore.h"

void SetupViewport(int width, int height);

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

SDL_Window* InitSDL(void)
{
    // Initialize SDL internal global state, only required systems
    // NOTE: Not all systems need to be initialized, SDL_INIT_AUDIO is not required, managed by miniaudio

    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER);
    if (result < 0) { TraceLog(LOG_WARNING, "SDL: Failed to initialize SDL"); return NULL; }

    // Initialize graphic device: display/window and graphic context
    //----------------------------------------------------------------------------
    unsigned int flags = 0;
    flags |= SDL_WINDOW_SHOWN;
    flags |= SDL_WINDOW_OPENGL;
    flags |= SDL_WINDOW_INPUT_FOCUS;
    flags |= SDL_WINDOW_MOUSE_FOCUS;
    flags |= SDL_WINDOW_MOUSE_CAPTURE;  // Window has mouse captured

    // flags |= SDL_WINDOW_FULLSCREEN;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    void* window = SDL_CreateWindow("HI", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, flags);
    void* glContext = SDL_GL_CreateContext(window);

    rlLoadExtensions(SDL_GL_GetProcAddress);

    // Disable mouse events being interpreted as touch events
    // NOTE: This is wanted because there are SDL_FINGER* events available which provide unique data
    //       Due to the way PollInputEvents() and rgestures.h are currently implemented, setting this won't break SUPPORT_MOUSE_GESTURES
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    return window;
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name)
{
    // return glfwGetProcAddress(name);
    // return glfwGetProcAddress(name);
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_events(void *ctx) {
    mpv_handle *mpv = ctx;
    while (1) {
        mpv_event *mp_event = mpv_wait_event(mpv, 0);
        if (mp_event->event_id == MPV_EVENT_NONE)
            break;
        if (mp_event->event_id == MPV_EVENT_LOG_MESSAGE) {
            mpv_event_log_message *msg = mp_event->data;
            // Print log messages about DR allocations, just to
            // test whether it works. If there is more than 1 of
            // these, it works. (The log message can actually change
            // any time, so it's possible this logging stops working
            // in the future.)

            if (strstr(msg->text, "DR image"))
                printf("log: %s", msg->text);
            continue;
        }
        printf("event: %s\n", mpv_event_name(mp_event->event_id));
    }
}

static int redraw = 0;


static void on_mpv_render_update(void *ctx) {
    mpv_render_context *mpv_gl = ctx;;
    if (mpv_gl == NULL) { return; }
    uint64_t flags = mpv_render_context_update(mpv_gl);
    if (flags & MPV_RENDER_UPDATE_FRAME) {
        redraw = 1;
    }
}

int InitPlatform(void);

 // Load directory filepaths with extension filtering and recursive directory scan. Use 'DIR' in the filter string to include directories in the result


int InitMyWindow(void) {
    
    const char* title = "MyTitle";
    // Initialize window data
    CORE.Window.screen.width = WINDOW_WIDTH;
    CORE.Window.screen.height = WINDOW_HEIGHT;
    CORE.Window.eventWaiting = false;
    CORE.Window.screenScale = MatrixIdentity();     // No draw scaling required by default
    if ((title != NULL) && (title[0] != 0)) CORE.Window.title = title;

    // Initialize global input state
    memset(&CORE.Input, 0, sizeof(CORE.Input));     // Reset CORE.Input structure to 0
    CORE.Input.Keyboard.exitKey = KEY_ESCAPE;
    CORE.Input.Mouse.scale = (Vector2){ 1.0f, 1.0f };
    CORE.Input.Mouse.cursor = MOUSE_CURSOR_ARROW;
    CORE.Input.Gamepad.lastButtonPressed = GAMEPAD_BUTTON_UNKNOWN;

    // Initialize platform
    //--------------------------------------------------------------
    InitPlatform();
    //--------------------------------------------------------------

    // Initialize rlgl default data (buffers and shaders)
    // NOTE: CORE.Window.currentFbo.width and CORE.Window.currentFbo.height not used, just stored as globals in rlgl
    rlglInit(CORE.Window.currentFbo.width, CORE.Window.currentFbo.height);
    isGpuReady = true; // Flag to note GPU has been initialized successfully

    // Setup default viewport
    SetupViewport(CORE.Window.currentFbo.width, CORE.Window.currentFbo.height);
    TRACELOG(LOG_INFO, "Done SetupViewport");

#if false && defined(SUPPORT_MODULE_RTEXT)
    #if defined(SUPPORT_DEFAULT_FONT)
        // Load default font
        // WARNING: External function: Module required: rtext
        // LoadFontDefault();
        #if defined(SUPPORT_MODULE_RSHAPES)
        // Set font white rectangle for shapes drawing, so shapes and text can be batched together
        // WARNING: rshapes module is required, if not available, default internal white rectangle is used
        Rectangle rec = GetFontDefault().recs[95];
        if (CORE.Window.flags & FLAG_MSAA_4X_HINT)
        {
            // NOTE: We try to maxime rec padding to avoid pixel bleeding on MSAA filtering
            SetShapesTexture(GetFontDefault().texture, (Rectangle){ rec.x + 2, rec.y + 2, 1, 1 });
        }
        else
        {
            // NOTE: We set up a 1px padding on char rectangle to avoid pixel bleeding
            SetShapesTexture(GetFontDefault().texture, (Rectangle){ rec.x + 1, rec.y + 1, rec.width - 2, rec.height - 2 });
        }
        #endif
    #endif
#else
    #if defined(SUPPORT_MODULE_RSHAPES)
    // Set default texture and rectangle to be used for shapes drawing
    // NOTE: rlgl default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
    Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    SetShapesTexture(texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });    // WARNING: Module required: rshapes
    #endif
#endif


    CORE.Time.frameCounter = 0;
    CORE.Window.shouldClose = false;

    // Initialize random seed
    // SetRandomSeed((unsigned int)time(NULL));
    
    TRACELOG(LOG_INFO, "SYSTEM: Working Directory: %s", GetWorkingDirectory());
    TRACELOG(LOG_INFO, "We Done\n");

    // SDL_Window* win = GetWindowHandle();
    // void* gl = SDL_GL_CreateContext(win);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        die("pass a single media file as argument");

    mpv_handle *mpv = mpv_create();
    if (!mpv)
        die("context init failed");

    mpv_set_option_string(mpv, "vo", "libmpv");


    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0)
        die("mpv init failed");

    mpv_request_log_messages(mpv, "debug");


    // InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "KKKK");
    printf("CORE.Window.currentFbo: %d\n" ,CORE.Window.currentFbo);

    // InitPlatform();
    InitMyWindow();

    printf("CORE.Window.currentFbo: %d\n" ,CORE.Window.currentFbo);
    // InitSDL();
    SetWindowTitle("LMAO");

    // SDL_Window *window =
    //     InitSDL();
        // SDL_CreateWindow("hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        //                  1000, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
        //                             SDL_WINDOW_RESIZABLE);
    SetTargetFPS(24);
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &(mpv_opengl_init_params){
            .get_proc_address = get_proc_address_mpv,
        }},
        
        // Tell libmpv that you will call mpv_render_context_update() on render
        // context update callbacks, and that you will _not_ block on the core
        // ever (see <libmpv/render.h> "Threading" section for what libmpv
        // functions you can call at all when this is active).
        // In particular, this means you must call e.g. mpv_command_async()
        // instead of mpv_command().

        // If you want to use synchronous calls, either make them on a separate
        // thread, or remove the option below (this will disable features like
        // DR and is not recommended anyway).
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}},

        {0}

    };

    // This makes mpv use the currently set GL context. It will use the callback
    // (passed via params) to resolve GL builtin functions, as well as extensions.
    mpv_render_context *mpv_gl;
    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        die("failed to initialize mpv GL context");

    // We use events for thread-safe notification of the SDL main loop.
    // Generally, the wakeup callbacks (set further below) should do as least
    // work as possible, and merely wake up another thread to do actual work.
    // On SDL, waking up the mainloop is the ideal course of action. SDL's
    // SDL_PushEvent() is thread-safe, so we use that.
    // wakeup_on_mpv_render_update = SDL_RegisterEvents(1);
    // wakeup_on_mpv_events = SDL_RegisterEvents(1);
    // if (wakeup_on_mpv_render_update == (Uint32)-1 ||
    //     wakeup_on_mpv_events == (Uint32)-1)
    //     die("could not register events");

    // When normal mpv events are available.

    mpv_set_wakeup_callback(mpv, on_mpv_events, mpv);

    // When there is a need to call mpv_render_context_update(), which can
    // request a new frame to be rendered.
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, mpv_gl);

    // Play this file.
    const char *cmd[] = {"loadfile", argv[1], NULL};
    mpv_command_async(mpv, 0, cmd);

    // assert(IsRenderTextureReady(tex));
    // RenderTexture tex = LoadRenderTexture(400, 400);



    while (1) {
        redraw = 0;
        BeginDrawing();
        // ClearBackground(RAYWHITE);
        PollInputEvents();
        if (IsKeyPressed(KEY_SPACE)) {
                const char *cmd_pause[] = {"cycle", "pause", NULL};
                mpv_command_async(mpv, 0, cmd_pause);
        } else if (IsKeyPressed(KEY_S)) {
            // Also requires MPV_RENDER_PARAM_ADVANCED_CONTROL if you want

            // screenshots to be rendered on GPU (like --vo=gpu would do).

            const char *cmd_scr[] = {"screenshot-to-file",
                                     "screenshot.png",
                                     "window",
                                     NULL};
            printf("attempting to save screenshot to %s\n", cmd_scr[1]);
            mpv_command_async(mpv, 0, cmd_scr);
        }
            // Happens when there is new work for the render thread (such as
            // rendering a new video frame or redrawing it).
        // if (redraw) {
            int width = GetScreenWidth();                                   // Get current screen width
            int height = GetScreenWidth();                                   // Get current screen width
            int fbo = 0;
            mpv_render_param params[] = {
                // Specify the default framebuffer (0) as target. This will
                // render onto the entire screen. If you want to show the video
                // in a smaller rectangle or apply fancy transformations, you'll
                // need to render into a separate FBO and draw it manually.

                {MPV_RENDER_PARAM_OPENGL_FBO, &(mpv_opengl_fbo){
                    .fbo = fbo,
                    .w = width,
                    .h = height,
                    // .fbo = tex.id,
                    // .w = tex.texture.width,
                    // .h = tex.texture.height,
                }},
                // Flip rendering (needed due to flipped GL coordinate system).
                {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
                // {0}
                {MPV_RENDER_PARAM_INVALID, NULL},

            };
            // See render_gl.h on what OpenGL environment mpv expects, and
            // other API details.
            // BeginTextureMode(tex);
        
            // assert(0);
            mpv_render_context_render(mpv_gl, params);
            // EndTextureMode();
            // DrawTexture(tex.texture, GetScreenWidth()/2 - tex.texture.width/2, GetScreenHeight()/2 - tex.texture.height/2 - 40, WHITE);
            // DrawTexture(tex.texture, GetScreenWidth()/2 - tex.texture.width/2, GetScreenHeight()/2 - tex.texture.height/2 - 40, WHITE);
        // DrawCircle(500, 500, 15, RED);
        // EndDrawing();
        SwapScreenBuffer();
        // }
    }

done:

    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    mpv_render_context_free(mpv_gl);

    mpv_destroy(mpv);

    printf("properly terminated\n");
    return 0;
}
void SetupViewport(int width, int height)
{
    CORE.Window.render.width = width;
    CORE.Window.render.height = height;

    TRACELOG(LOG_INFO, "height %d",      CORE.Window.render.height);
    TRACELOG(LOG_INFO, "width %d",       CORE.Window.render.width);

    TRACELOG(LOG_INFO, "renderoffset%d", CORE.Window.renderOffset.x);
    TRACELOG(LOG_INFO, "renderoffset%d", CORE.Window.renderOffset.x);

    TRACELOG(LOG_INFO, "renderWidth%d",  CORE.Window.render.width);
    TRACELOG(LOG_INFO, "renderHeight%d", CORE.Window.render.height);

    // Set viewport width and height
    // NOTE: We consider render size (scaled) and offset in case black bars are required and
    // render area does not match full display area (this situation is only applicable on fullscreen mode)
#if defined(__APPLE__)
    Vector2 scale = GetWindowScaleDPI();
    rlViewport(CORE.Window.renderOffset.x/2*scale.x, CORE.Window.renderOffset.y/2*scale.y, (CORE.Window.render.width)*scale.x, (CORE.Window.render.height)*scale.y);
#else
    rlViewport(CORE.Window.renderOffset.x/2, CORE.Window.renderOffset.y/2, CORE.Window.render.width, CORE.Window.render.height);
#endif

    rlMatrixMode(RL_PROJECTION);        // Switch to projection matrix
    rlLoadIdentity();                   // Reset current matrix (projection)

    // Set orthographic projection to current framebuffer size
    // NOTE: Configured top-left corner as (0, 0)
    rlOrtho(0, CORE.Window.render.width, CORE.Window.render.height, 0, 0.0f, 1.0f);


    rlMatrixMode(RL_MODELVIEW);         // Switch back to modelview matrix
    rlLoadIdentity();                   // Reset current matrix (modelview)
}


