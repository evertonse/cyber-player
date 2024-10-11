// TODO: Use nob.h
// TODO: Make seeking be based on percentage of file total duration
// TODO: Mark seen parts
// TODO: Start from where we left off


// #define GLFW_INCLUDE_NONE       // Disable the standard OpenGL header inclusion on GLFW3
//                                 // NOTE: Already provided by rlgl implementation (on glad.h)
// #include "GLFW/glfw3.h"         // GLFW3 library: Windows, OpenGL context and Input management

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include <string.h>
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"


#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 900

#define PROGRESS_BAR_WIDTH  200
#define PROGRESS_BAR_HEIGHT 30

#include <dirent.h>
#include "raylib.h"
//#include "rcore.h"

typedef struct {
    double *items;
    size_t count;
    size_t capacity;
} SeenPositionPercentages;

typedef struct {
    double start;
    double end;
} Segment;


void* glfwGetProcAddress(const char* procname);
static void* get_proc_address_mpv(void *ctx, const char *name) {
    rlLoadExtensions(glfwGetProcAddress);
    return glfwGetProcAddress(name);
}

#define MAX_SEGMENTS 1000

static Segment* watched_segments = NULL;

static int segment_count = 0;
static double duration = 0;
static double last_position = -1;
static double segment_start = -1;

void add_watched_segment(double start, double end) {
    if (segment_count > MAX_SEGMENTS) {
        TraceLog(LOG_WARNING, "Darn too many danm segments (%d of max expected %d", segment_count, MAX_SEGMENTS);
    }
    arrput(watched_segments, (CLITERAL(Segment){start, end}));
    segment_count = arrlen(watched_segments);
}

void merge_segments() {
    if (segment_count <= 1) return;

    int i, j;
    for (i = 0, j = 1; j < segment_count; j++) {
        if (watched_segments[i].end >= watched_segments[j].start) {
            watched_segments[i].end = fmax(watched_segments[i].end, watched_segments[j].end);
        } else {

            i++;
            watched_segments[i] = watched_segments[j];

        }
    }
    segment_count = i + 1;
}




// #define NOB_IMPLEMENTATION
// #include "nob.h"
static RenderTexture2D progress_bar_render_texture;


#if defined(__MINGW64_VERSION_MAJOR) || defined(__MINGW64__) || defined(__MINGW32__)
    #define MINGW
#endif


#if !defined(MINGW)
    #include <glib.h>
    GMutex  mutex;
#endif


static double* percent_positions = NULL;
static const char* currently_playing_path = NULL;
static double percent_position;

void player_load_file(void* ctx, const char* file_path)  {
    const char *cmd[] = {"loadfile", file_path, NULL};
    currently_playing_path = file_path;
    mpv_command_async(ctx, 0, cmd);

    BeginTextureMode(progress_bar_render_texture);
    {
        ClearBackground(BLACK);
    }
    EndTextureMode();

    arrsetlen(watched_segments, 0);
    segment_count = 0;
    duration = 0;
    last_position = -1;
    segment_start = -1;
}

int wait_for_property(mpv_handle *ctx, const char *name) {
    mpv_observe_property(ctx, 0, name, MPV_FORMAT_NONE);
    
    while (1) {
        mpv_event *event = mpv_wait_event(ctx, 10);
        if (event->event_id == MPV_EVENT_NONE) {
            continue;
        }
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            mpv_event_property *prop = event->data;

            if (strcmp(prop->name, name) == 0) {
                mpv_unobserve_property(ctx, 0);
                return 0;
            }
        }
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            return -1;

        }
    }
}

double mpv_get_property_double(mpv_handle *ctx, const char *name) {
    double value;
    int err = mpv_get_property(ctx, name, MPV_FORMAT_DOUBLE, &value);
    if (err < 0) {
        printf("Failed to get %s: %s\n", name, mpv_error_string(err));
        return -1;

    }
    return value;
}

static int64_t mpv_get_property_int64(mpv_handle *ctx, const char *name) {
    int64_t value;
    int err = mpv_get_property(ctx, name, MPV_FORMAT_INT64, &value);
    if (err < 0) {
        printf("Failed to get %s: %s\n", name, mpv_error_string(err));
        return -1;
    }
    return value;
}


static inline void mpv_check_error(int status) {
    if (status < 0) {
        printf("mpv API error: %s\n", mpv_error_string(status));
        exit(1);
    }
}

static void on_property_change(mpv_event_property *prop) {
    // MPV_FORMAT_INT64            = 4,
    // MPV_FORMAT_DOUBLE           = 5,

    if (prop == NULL)
        return;

    TraceLog(LOG_INFO, "on_property_change event->name=%s", prop->name);
    static int last_time = 0;
    if (strcmp(prop->name, "percent-pos") == 0) {
        if(prop->format == MPV_FORMAT_DOUBLE) {
            percent_position = *(double *)prop->data;
            arrput(percent_positions, percent_position);

            double time_position = *(double *)prop->data;
            TraceLog(LOG_INFO,"percent-pos:%.2f%%\n", percent_position);

            BeginTextureMode(progress_bar_render_texture);
            {
                DrawRectangle((percent_position/100)*PROGRESS_BAR_WIDTH, 0, 2, PROGRESS_BAR_HEIGHT, GREEN);
            }
            EndTextureMode();
        }
    }

    if (strcmp(prop->name, "volume") == 0) {
        if(prop->format == MPV_FORMAT_DOUBLE) {
            double volume = *(double *)prop->data;
            assert(MPV_FORMAT_DOUBLE == prop->format);
            TraceLog(LOG_INFO, "Current %s: %.2f\n", prop->name, volume);
        }
    }


    #define PLAYBACK_GAP_SIZE 1.0
    // playback-time does the same thing as time-pos but works for streaming media
    if (strcmp(prop->name, "playback-time") == 0) {

        if(prop->format == MPV_FORMAT_DOUBLE) {
            double playback_time = *(double *)prop->data;
            double time_position = playback_time;
            TraceLog(LOG_INFO, "Current %s: %f\n", prop->name, playback_time);

            if (last_position < 0) {
                segment_start = time_position;
            } else if ((time_position < segment_start) ||(fabsf(time_position - last_position) > PLAYBACK_GAP_SIZE)) {
                // Gap in playback, end current segment
                add_watched_segment(segment_start, last_position);
                segment_start = time_position;
            }

            last_position = time_position;
        }
    }

    // playback-time does the same thing as time-pos but works for streaming media
    if (strcmp(prop->name, "osd-dimensions") == 0) {
        if(prop->format == MPV_FORMAT_INT64) {
            int64_t osd_dimensions = *(int64_t *)prop->data;
            assert(MPV_FORMAT_INT64 == prop->format);
            TraceLog(LOG_INFO, "Current %s: %ld\n", prop->name, osd_dimensions);
        }
    }

    if (strcmp(prop->name, "duration") == 0) {
        if(prop->format == MPV_FORMAT_DOUBLE) {
            duration = *(double *)prop->data;
            TraceLog(LOG_INFO, "Current %s: %f\n", prop->name, duration);
        }
        // assert(0);
    }

    if (strcmp(prop->name, "time-pos") == 0) {
        if(prop->format == MPV_FORMAT_DOUBLE) {
            double time_position = *(double *)prop->data;
            TraceLog(LOG_INFO, "Current %s: %f\n", prop->name, time_position);
            // if (last_position < 0) {
            //     segment_start = time_position;
            // } else if (time_position - last_position > 1.0) {
            //     // Gap in playback, end current segment
            //     add_watched_segment(segment_start, last_position);
            //     segment_start = time_position;
            // }
            //
            // last_position = time_position;
        }
    }

}


static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static int redraw = 0;

static void on_mpv_render_update(void *ctx) {
    mpv_render_context *mpv_rd = ctx;
    if (mpv_rd == NULL) { return; }
    uint64_t flags = mpv_render_context_update(mpv_rd);
    if (flags & MPV_RENDER_UPDATE_FRAME) {

#if !defined(MINGW)
       g_mutex_lock(&mutex);
#endif
       redraw = 1;
#if !defined(MINGW)
       g_mutex_unlock(&mutex);
#endif

    }
}

static int wakeup = 0;

static void on_mpv_events(void *ctx) {

#if !defined(MINGW)
       g_mutex_lock(&mutex);
#endif
       wakeup = 1;
#if !defined(MINGW)
       g_mutex_unlock(&mutex);
#endif
}

static void handle_mpv_events(mpv_handle *mpv) {
    static int times_called = 0;
    times_called += 1;
    while (mpv) {
        mpv_event *mp_event = mpv_wait_event(mpv, 0);
        // TraceLog(LOG_ERROR, "times_called:%d event:%s\n", times_called, mpv_event_name(mp_event->event_id));

        if (mp_event == NULL ||mp_event->event_id == MPV_EVENT_NONE)
            break;

        /**
         * The meaning and contents of the data member depend on the event_id:
         *  MPV_EVENT_GET_PROPERTY_REPLY:     mpv_event_property*
         *  MPV_EVENT_PROPERTY_CHANGE:        mpv_event_property*
         *  MPV_EVENT_LOG_MESSAGE:            mpv_event_log_message*
         *  MPV_EVENT_CLIENT_MESSAGE:         mpv_event_client_message*
         *  MPV_EVENT_START_FILE:             mpv_event_start_file* (since v1.108)
         *  MPV_EVENT_END_FILE:               mpv_event_end_file*
         *  MPV_EVENT_HOOK:                   mpv_event_hook*
         *  MPV_EVENT_COMMAND_REPLY*          mpv_event_command*
         *  other: NULL
         *
         * Note: future enhancements might add new event structs for existing or new
         *       event types.
         */
    

        if (mp_event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            printf("uint64_t reply_userdata = %d;\n", mp_event->reply_userdata);
            printf("uint64_t error = %d;\n", mp_event->error);

            mpv_event_property*  prop = mp_event->data;
            on_property_change(prop);
        }

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
        if (mp_event->event_id == MPV_EVENT_IDLE) {
        }
    }
}

bool IsKeyPressedOrRepeat(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

RenderTexture2D mpv_texture;
FilePathList mp4files;
int main(int argc, char *argv[]) {
    if (argc != 2)
        die("pass a single media file as argument");

    mpv_handle *mpv = mpv_create();
    if (!mpv)
        die("context init failed");


    mpv_set_option_string(mpv, "force-window", "yes");
    int keep_open = 1;
    mpv_set_option(mpv, "keep-open", MPV_FORMAT_FLAG, &keep_open);
    mpv_set_option_string(mpv, "osc", "no");  // Disable on-screen controller
    mpv_set_option_string(mpv, "osd-level", "0");  // Disable on-screen display

    mpv_set_option_string(mpv, "input-default-bindings", "no");  // Disable default key bindings
    mpv_set_option_string(mpv, "input-vo-keyboard", "no");  // Disable keyboard input on video output
    mpv_set_option_string(mpv, "audio-display", "no");  // Disable audio visualization

    mpv_check_error(mpv_set_option_string(mpv, "vo", "libmpv"));


    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0)
        die("mpv init failed");
    TraceLog(LOG_INFO, "MPV initialized with success");
    // get length

        // set mpv options
    // int64_t wid;
    // mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);
    mpv_set_option_string(mpv, "input-cursor", "no");   // no mouse handling
    mpv_set_option_string(mpv, "cursor-autohide", "no");// no cursor-autohide, we handle that

    mpv_set_option_string(mpv, "ytdl", "yes"); // youtube-dl support
    mpv_set_option_string(mpv, "sub-auto", "fuzzy"); // Automatic subfile detection
    mpv_set_option_string(mpv, "audio-client-name", "our-mplayer"); // show correct icon in e.g. pavucontrol


    mpv_request_log_messages(mpv, "debug");

    mpv_check_error(
        mpv_observe_property(mpv, 69, "volume", MPV_FORMAT_DOUBLE)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "sid", MPV_FORMAT_INT64)
    );
    // mpv_check_error(
    //     mpv_observe_property(mpv, 69, "aid", MPV_FORMAT_INT64)
    // );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "sub-visibility", MPV_FORMAT_FLAG)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "mute", MPV_FORMAT_FLAG)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "core-idle", MPV_FORMAT_FLAG)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "idle-active", MPV_FORMAT_FLAG)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "pause", MPV_FORMAT_FLAG)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "paused-for-cache", MPV_FORMAT_FLAG)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "percent-pos", MPV_FORMAT_DOUBLE)
    );
    mpv_check_error(
        mpv_observe_property(mpv, 69, "pause", MPV_FORMAT_INT64)
    );

    mpv_check_error(
        mpv_observe_property(mpv, 420, "playback-time", MPV_FORMAT_DOUBLE)
    );

    mpv_check_error(
        mpv_observe_property(mpv, 420, "time-pos", MPV_FORMAT_DOUBLE)
    );


    mpv_check_error(
        mpv_observe_property(mpv, 69, "osd-dimensions", MPV_FORMAT_INT64)
    );

    mpv_check_error(
        mpv_observe_property(mpv, 69, "duration", MPV_FORMAT_DOUBLE)
    );



#define FILES_LISTING_LIMIT  10

#if defined(MINGW)
    // const char* dir = "E:\\Torrents\\Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN";
    const char* dir = "C:\\Users\\Administrator\\Downloads";
    // TODO: Add better file filtering for Raylib
    mp4files = LoadDirectoryFilesEx(dir, ".mp4", true);
    // mp4files = LoadDirectoryFiles(dir);
#else
    mp4files = LoadDirectoryFilesEx("/home/excyber/media/videos/", ".mp4", true);
#endif
    mp4files.count =  mp4files.count < FILES_LISTING_LIMIT ? mp4files.count: FILES_LISTING_LIMIT;
    
    printf("mp4files.count: %d\n", mp4files.count);
    for (int i = 0; i < mp4files.count ; ++i) {
        printf("%s\n", mp4files.paths[i]);
    }
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MINGW");

    Font jetbrains = LoadFontEx("./assets/fonts/JetBrainsMonoNerdFont-Medium.ttf", 128, NULL, 0);


    mpv_texture = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    SetWindowState(
        FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN
        | FLAG_WINDOW_HIGHDPI

        // support mouse passthrough, only supported when FLAG_WINDOW_UNDECORATED
        // | FLAG_WINDOW_UNDECORATED 
        // | FLAG_WINDOW_MOUSE_PASSTHROUGH
        | FLAG_VSYNC_HINT

        | FLAG_MSAA_4X_HINT
        | FLAG_INTERLACED_HINT
        // run program in borderless windowed mode
        // | FLAG_BORDERLESS_WINDOWED_MODE
    );
    SetConfigFlags(
        FLAG_MSAA_4X_HINT
        | FLAG_INTERLACED_HINT
    );
    progress_bar_render_texture = LoadRenderTexture(PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT);

    TraceLog(LOG_INFO, "Screen Size %dx%d", GetScreenWidth(), GetScreenHeight());

#if defined(SOFTWARE_RENDERER)

    mpv_render_context *mpv_rd;
    {

        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_SW},
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

        // SwapScreenBuffer();
        // PollInputEvents();
        // WaitTime(0);

        mpv_check_error(mpv_render_context_create(&mpv_rd, mpv, params));
        printf("MPV Context created just fine\n");
    }

#else
    mpv_render_context *mpv_gl;

    {
        mpv_opengl_init_params gl_init_params = {
            .get_proc_address = get_proc_address_mpv,
        };
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
            // {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}},
            {0}
        };
        if (mpv_render_context_create(&mpv_gl, mpv, params) < 0) {
            printf("Failed to initialize MPV GL context\n");
            return 1;
        }

    }

#endif


    printf("Setting up mpv_set_wakeup_callback");
    mpv_set_wakeup_callback(mpv, on_mpv_events, mpv);
    printf("Setting up mpv_render_context_set_update_callback");
#if defined(SOFTWARE_RENDERER)
    mpv_render_context_set_update_callback(mpv_rd, on_mpv_render_update, mpv_rd);
#else
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update, mpv_gl);
#endif

    printf("About to load texture\n");
    Image image = { NULL, WINDOW_WIDTH, WINDOW_HEIGHT, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    Texture tex = LoadTextureFromImage(image);
    UnloadImage(image);

    printf("Loaded Texture just fine\n");

    // Play this file.
    if (!FileExists(argv[1])) {

        TraceLog(LOG_ERROR, "File %s does not exist, nothing is being played", argv[1]);

    } else {
        arrsetlen(percent_positions, 0);
        player_load_file(mpv, argv[1]);
    }
    
    // Wait until the file is loaded

    mpv_wait_event(mpv, -1);

    // double pfps = mpv_get_property_double(mpv, "container-fps");

    
    // int64_t total_frames = (int64_t)(duration * pfps);

    // printf("duration:%f fps:%f total_frames:%ld\r", duration, pfps, total_frames);
    // #define FPS 75
    // #define FPS 60
    #define FPS 35
    SetTargetFPS(FPS);
    char fps[123];


    BeginDrawing();
        BeginTextureMode(progress_bar_render_texture);
        {
            // Claer only once
            ClearBackground(RAYWHITE);
        }
    EndDrawing();

    void* pixels = malloc(sizeof(char)*4*2000*2000);
    bool draw_file_list = false;
    while (!WindowShouldClose()) {
        BeginDrawing();
        EndTextureMode();
        ClearBackground(RAYWHITE);

        SetTraceLogLevel(LOG_NONE);
        Vector2 mouse_pos = GetMousePosition();
        Vector2 mouse_wheel = GetMouseWheelMoveV();
        TraceLog(LOG_INFO, "mouse_wheel = {%f, %f};", mouse_wheel.x, mouse_wheel.y);


        if (IsKeyPressedOrRepeat(KEY_LEFT)) {
            const char *mpv_cmd[] = {"seek", "-5", "relative", "keyframes", NULL};
            mpv_command_async(mpv, 1, mpv_cmd);
        } else if (IsKeyPressedOrRepeat(KEY_RIGHT)) {
            const char *mpv_cmd[] = {"seek", "5", "relative", "keyframes", NULL};
            mpv_command_async(mpv, 0, mpv_cmd);
        } else if (IsKeyPressedOrRepeat(KEY_UP)) {
            const char *mpv_cmd[] = {"osd-auto", "add", "volume", "6", NULL};
            mpv_command_async(mpv, 0, mpv_cmd);
        } else if (IsKeyPressedOrRepeat(KEY_DOWN)) {
            const char *mpv_cmd[] = {"osd-auto", "add", "volume", "-5", NULL};
            mpv_command_async(mpv, 0, mpv_cmd);
        } else if (IsKeyPressedOrRepeat(KEY_M)) {
            const char *mpv_cmd[] = {"cycle", "mute", NULL};
            mpv_command_async(mpv, 0, mpv_cmd);
        } else if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                SetWindowPosition(mouse_pos.x, mouse_pos.y);
            }
            if (IsKeyPressed(KEY_C)) {
                goto done;
            }
        }

        {
            if (mouse_wheel.y != 0.0) {
                char temp[5];
                int diff = (int)(5.0 * mouse_wheel.y);
                snprintf(temp, sizeof(temp), "%d", diff);
                const char *cmd_pause[] = {"osd-auto", "add", "volume", temp, NULL};
                mpv_command_async(mpv, 0, cmd_pause);
            }
        }

        if (IsKeyPressed(KEY_SPACE)) {
            const char *cmd_pause[] = {"cycle", "pause", NULL};
            mpv_command_async(mpv, 0, cmd_pause);
        } else if(IsKeyPressed(KEY_S)) {
            // Also requires MPV_RENDER_PARAM_ADVANCED_CONTROL if you want
            // screenshots to be rendered on GPU (like --vo=gpu would do).
            const char *cmd_scr[] = {"screenshot-to-file",
                                     "screenshot.png",
                                     "window",
                                     NULL};
            printf("attempting to save screenshot to %s\n", cmd_scr[1]);

            mpv_command_async(mpv, 0, cmd_scr);
        }

        if (wakeup) {
            handle_mpv_events(mpv);
            wakeup = 0;
        }
        if (redraw) {
        #if defined(SOFTWARE_RENDERER)
            int w = GetScreenWidth();
            int h = GetScreenHeight();
            int pitch = w*4;

            mpv_render_param params[] = {
                {MPV_RENDER_PARAM_SW_SIZE, (int[2]){w, h}},
                {MPV_RENDER_PARAM_SW_FORMAT, "rgba"},
                {MPV_RENDER_PARAM_SW_STRIDE, &(size_t){pitch}},
                {MPV_RENDER_PARAM_SW_POINTER, pixels},
                {0}
            };
            int r = mpv_render_context_render(mpv_rd, params);
            if (r < 0) {
                printf("mpv_render_context_render error: %s\n",
                       mpv_error_string(r));
                exit(1);
            }
            image.data = pixels;
            image.width = w;
            image.height = h;
        #else
            // Render video frame to texture
            mpv_opengl_fbo fbo = {
                .fbo = mpv_texture.id,
                .w = mpv_texture.texture.width,
                .h = mpv_texture.texture.height,
            };
            mpv_render_param render_params[] = {
                {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                {0}
            };
            mpv_render_context_render(mpv_gl, render_params);

        #endif

        }
          // Create a texture from the RGBA pixel data
        SetTraceLogLevel(LOG_NONE);
        //
        // TODO: Change to UpdateTexture in a way that it works with
        // change in widht and height, rn we need to Load Everytime, which is wasteful as shit
        // we can even print something to check if we're getting new texture id's all the time
        // problably yeah
        //
        // example: UpdateTextureRec(tex, CLITERAL(Rectangle){0, 0, image.width, image.height}, image.data);
        //


        if (mouse_pos.x > GetScreenWidth()/1.4) {
           draw_file_list = false;
        }
        #if defined(SOFTWARE_RENDERER)
            tex = LoadTextureFromImage(image);
            SetTraceLogLevel(LOG_ERROR);
            // Draw video texture
            if (mouse_pos.x < 20) {
                DrawTextureEx(tex, CLITERAL(Vector2){GetScreenWidth()*0.5, GetScreenHeight()*0.5-(tex.height)}, 0, 0.5, RAYWHITE);
                draw_file_list = true;
            } else {
                DrawTexture(tex, 0, 0, RAYWHITE);
            }
        #else
            DrawTextureRec(mpv_texture.texture,
                           (Rectangle){0, 0, (float)mpv_texture.texture.width, (float)mpv_texture.texture.height},
                           (Vector2){0, 0}, WHITE);
        #endif

            


        if (draw_file_list) {
            for (int i = 0; i < mp4files.count ; ++i) {
                const char* file_path = mp4files.paths[i];
                const int   font_size = 20;
                const int   spacing = 0;
                // const Font  font = GetFontDefault();
                const Font  font = jetbrains;
                Vector2 text_pos = {0, i*font_size};
                // RLAPI void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint); // Draw text using font and additional parameters
                DrawTextEx(font, file_path, text_pos, font_size, spacing, RED);
                Vector2 text_size = MeasureTextEx(font, file_path, font_size, spacing);
                //TRACELOG(LOG_INFO, "Mouse position = %d,%d", (int)mouse_pos.x, (int)mouse_pos.y);
                Rectangle text_rect = {text_pos.x, text_pos.y, text_size.x, text_size.y};
                // Check if point is inside rectangle
                //printf("text_size = {%d %d};\n", (int)text_size.x, (int)text_size.y);
                if (CheckCollisionPointRec(mouse_pos, text_rect)) {
                    DrawTextEx(font, file_path, text_pos, font_size, spacing, BLUE);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        player_load_file(mpv, file_path);
                    }
                }


            }
        }

        TraceLog(LOG_TRACE, "mouse_pos = {%d %d};\n", (int)mouse_pos.x, (int)mouse_pos.y);



        DrawCircle((int)mouse_pos.x, (int)mouse_pos.y, 4, RED);

        snprintf(fps, sizeof(fps), " %d fps", GetFPS());
        SetWindowTitle(fps);

        DrawTexture(progress_bar_render_texture.texture, 0, 0, WHITE);

        // TraceLog(LOG_NONE, "FPS:%d; Target_FPS=%d",GetFPS(), FPS);

        // Draw progress bar background
        {


            Rectangle playback_rect = {0, GetScreenHeight() - PROGRESS_BAR_HEIGHT, GetScreenWidth(), 40};

            int playback_current_x = (int)((double)playback_rect.width)*(percent_position/100);
            DrawRectangle(playback_rect.x, playback_rect.y, playback_rect.width, playback_rect.height, BLACK);

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, playback_rect)) {
                double new_playback_percentage = (double) (mouse_pos.x - playback_rect.x)/playback_rect.width;
                char temp[128];
                snprintf(temp, sizeof(temp), "%f", new_playback_percentage*100);
                // const char *cmd_seek[] = {"seek", "10%","absolute", NULL};
                const char *cmd_seek[] = {"seek", temp, "absolute-percent", "exact", NULL};
                printf("seek %s absolute\n", temp);

                bool sync = false;
                if (sync) {
                    mpv_command(mpv, cmd_seek);
                } else {
                    mpv_command_async(mpv, 420, cmd_seek);
                }

            }

            // Merge overlapping segments
            // merge_segments();

            // Draw watched segments

            if (duration > 0) {
                for (int i = 0; i < segment_count; i++) {
                    int start_x = playback_rect.x + (watched_segments[i].start / duration) * playback_rect.width;
                    int end_x = playback_rect.x + (watched_segments[i].end / duration) * playback_rect.width;
                    DrawRectangle(start_x, playback_rect.y, end_x - start_x, PROGRESS_BAR_HEIGHT, BLUE);
                }

                int start_x = playback_rect.x + (segment_start/ duration) * playback_rect.width;
                int end_x = playback_rect.x + (last_position / duration) * playback_rect.width;
                if (start_x < end_x) {
                    DrawRectangle(start_x, playback_rect.y, end_x - start_x, PROGRESS_BAR_HEIGHT, BLUE);
                }
            }
            // Draw current position
            // if (duration > 0 && last_position >= 0) {
            //     int pos_x = playback_rect.x + (last_position / duration) * playback_rect.width;
            //     // DrawRectangle(pos_x - 2, playback_rect.y, 4, PROGRESS_BAR_HEIGHT + 10, RED);
            // }

            DrawRectangle(playback_current_x, playback_rect.y, 2, playback_rect.height, RED);

        }


        EndDrawing();
    }

done:
    // Add final segment
    if (segment_start >= 0 && last_position >= 0) {
        add_watched_segment(segment_start, last_position);
    }

    // Merge overlapping segments
    merge_segments();

    // Print final watched segments
    printf("Watched segments from duration %f:\n", duration);
    for (int i = 0; i < segment_count; i++) {
        printf("%.2f - %.2f\n", watched_segments[i].start, watched_segments[i].end);
    }
    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.

    #ifdef SOFTWARE_RENDERER
    mpv_render_context_free(mpv_rd);
    #else
    mpv_render_context_free(mpv_gl);
    #endif

    mpv_destroy(mpv);


    printf("properly terminated\n");
    return 0;
}

