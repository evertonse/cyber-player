// DONE: Use nob.h
// TODO: Make seeking be based on percentage of file total duration
// TODO: Start from where we left off

// #define GLFW_INCLUDE_NONE       // Disable the standard OpenGL header
// inclusion on GLFW3
//                                 // NOTE: Already provided by rlgl
//                                 implementation (on glad.h)
// #include "GLFW/glfw3.h"         // GLFW3 library: Windows, OpenGL context and
// Input management

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "resources.h"

#define TRACE(level, ...) TraceLog(level, __VA_ARGS__)
#define TRACEINFO(...) TraceLog(LOG_INFO, __VA_ARGS__)

#if defined(__MINGW64_VERSION_MAJOR) || defined(__MINGW64__) ||  defined(__MINGW32__)
#   define MINGW
#endif
/*
 . Create a realpath replacement macro for when compiling under mingw
 . Based upon
 https://stackoverflow.com/questions/45124869/cross-platform-alternative-to-this-realpath-definition
*/
#if defined(_WIN32) && defined(__MINGW64__)
// #   include "dirent.h" // Conflicts with defined in nob.h only in mingw
#   define realpath(N, R) _fullpath((R), (N), PATH_MAX)
#else
#   include "dirent.h"
#   include <linux/limits.h>
#endif

#include <math.h>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include <pthread.h>




#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "header.h"

#define GUI_IMPLEMENTATION
#include "gui.h"
// #include "rcore.h" // For debugginh purposes

float get_value(float x, float lox, float hix) {
    if (x < lox) x = lox;
    if (x > hix) x = hix;
    x -= lox;
    x /= hix - lox;
    return x;
}


#pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wno-padded"
#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"
#pragma clang diagnostic pop


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define DEFAULT_LOG_LEVEL LOG_NONE

#define HASH_DOES_NOT_EXIST (-1)
#define FILE_PROGRESS "progress.bin"



#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900

#define PROGRESS_BAR_WIDTH 200
#define PROGRESS_BAR_HEIGHT 5

#define pthread_rwlock_rdlock(lock)
#define pthread_rwlock_unlock(lock)
#define pthread_rwlock_wrlock(lock)

void *glfwGetProcAddress(const char *procname);
static void *get_proc_address_mpv(void *ctx, const char *name) {
    rlLoadExtensions(glfwGetProcAddress);
    return glfwGetProcAddress(name);
}

#define MAX_SEGMENTS 1000

static struct {
    bool             paused;
    const            char *absolute_path; //  It's   not absolute rn
    Segment          *watched_segments;
    pthread_rwlock_t *rwlock;             //  NOT Needed rn
    int              segment_count;
    double           duration;
    double           last_position;
    double           segment_start;
} sc /* segment_context */ = {
    .absolute_path    = NULL,
    .paused           = false,
    .watched_segments = NULL,
    .rwlock           = &(pthread_rwlock_t){},
    .segment_count    = 0,
    .duration         = 0,
    .last_position    = -1,
    .segment_start    = -1,
};

void add_watched_segment(double start, double end) {
    if (!(start >= 0 && end >= 0)) {
        TRACE(LOG_ERROR, "%s start (%f) or end (%f)is negative",
              __PRETTY_FUNCTION__, start, end);
        return;
    }
    if (end < start) {
        TRACE(LOG_ERROR, "%s end (%f) is  bigger than start",
              __PRETTY_FUNCTION__, end, start);
        return;
    }

    if (sc.segment_count > MAX_SEGMENTS) {
        TRACE(LOG_WARNING, "Darn too many danm segments (%d of max expected %d",
              sc.segment_count, MAX_SEGMENTS);
    }
    arrput(sc.watched_segments, (CLITERAL(Segment){start, end}));
    sc.segment_count = arrlen(sc.watched_segments);
}

int compare_segments(const void *a, const void *b) {
    const Segment *seg_a = (const Segment *)a;
    const Segment *seg_b = (const Segment *)b;
    if (seg_a->start < seg_b->start) {
        return -1;
    } else if (seg_a->start > seg_b->start) {
        return 1;
    } else {
        return 0;
    }
}

Segment *merge_segments_with_args(Segment *segments, size_t num_segments) {
    // Sort the segments by start time
    qsort(segments, num_segments, sizeof(Segment), compare_segments);

    size_t write = 0;
    for (size_t read = 0; read < num_segments; read++) {
        if (write == 0 || segments[read].start > segments[write - 1].end) {
            segments[write++] = segments[read];
        } else {
            segments[write - 1].end =
                fmax(segments[write - 1].end, segments[read].end);
        }
    }

    // Truncate the array to the new size
    arrsetlen(segments, write);
    return segments;
}

void merge_segments() {
    if (sc.segment_count <= 1) return;

    sc.watched_segments =
        merge_segments_with_args(sc.watched_segments, sc.segment_count);
    sc.segment_count = arrlen(sc.watched_segments);
}


static pthread_mutex_t *render_update_mutex = &(pthread_mutex_t){},
                       *event_wakeup_mutex = &(pthread_mutex_t){};

static double *percent_positions = NULL;
static const char *currently_playing_path = NULL;
static double percent_position;
static double volume;
static bool draw_file_list = false;
static bool seeking = false;

static FileProgress *file_progress_darray = NULL;

// From key file name to value that index is file_progress_darray;
static struct {
    char *key;
    int value;
} *file_progress_hash_map = NULL;

void collect_file_progress_info(void) {
    // Add final segment

    // Acquire "rights" to read
    double start = sc.segment_start, last_position = sc.last_position;

    add_watched_segment(start, last_position);

    merge_segments();

    char buffer[PATH_MAX];
    // Assuming it's null terminated
    realpath(currently_playing_path, buffer);
    int fp_index = shget(file_progress_hash_map, buffer);
    if (fp_index == HASH_DOES_NOT_EXIST) {
        FileProgress temp = {.segments = sc.watched_segments,

                             .segment_count = arrlen(sc.watched_segments)};
        // If RESOLVED is null, the result is malloc'd
        realpath(currently_playing_path, temp.filename);
        // Alternatively: TextCopy(temp.filename, currently_playing_path);
        TextCopy(temp.filename, currently_playing_path);

        SetTraceLogLevel(LOG_ERROR);
        TRACE(LOG_ERROR, "temp.filename = %s", temp.filename);
        arrput(file_progress_darray, temp);
        fp_index = arrlen(file_progress_darray) - 1;
        shput(file_progress_hash_map, temp.filename, fp_index);
    }
    file_progress_darray[fp_index].segments = sc.watched_segments;
    file_progress_darray[fp_index].segment_count = arrlen(sc.watched_segments);
    file_progress_darray[fp_index].volume = volume;
}

void reset_segments_info(void) {
    // FIX:  This does not work
    sc.watched_segments = NULL;
    sc.segment_count = 0;
    sc.duration = 0;
    sc.last_position = -1;
    sc.segment_start = -1;
}

void player_unpause(void *ctx) {
    const char *cmd[] = {"set", "pause", "no", NULL};
    mpv_command_async(ctx, 0, cmd);
}

void player_set_volume(void *ctx, double volume) {
    // const char *cmd[] = {"set_property", "volume", "50", NULL};
    // mpv_command_async(ctx, 0, cmd);
    mpv_set_property_async(ctx, 69, "volume", MPV_FORMAT_DOUBLE, &volume);
}

// const char *cmd[] = {"pause", NULL};
void player_pause(void *ctx) {
    const char *cmd[] = {"set", "pause", "yes", NULL};
    mpv_command_async(ctx, 0, cmd);
}

void player_load_file(void *ctx, const char *file_path) {
    if (currently_playing_path != NULL) {
        collect_file_progress_info();
    }

    const char *cmd[] = {"loadfile", file_path, NULL};
    currently_playing_path = file_path;
    volume = 50;
    mpv_command_async(ctx, 0, cmd);

    reset_segments_info();

    char buffer[PATH_MAX];
    // Assuming it's null terminated
    realpath(file_path, buffer);
    int index = shget(file_progress_hash_map, buffer);

    for (int i = 0; i < shlen(file_progress_hash_map); ++i) {
        if (file_progress_hash_map[i].value != HASH_DOES_NOT_EXIST) {
            printf("\nvalue:%d | key:%s | found_idx:%d\n",
               file_progress_hash_map[i].value,
               file_progress_hash_map[i].key,
               index);
        }
    }

    if (index != HASH_DOES_NOT_EXIST) {
        FileProgress fp = file_progress_darray[index];
        printf("\nfpcount:%d\n", fp.segment_count);
        for (int i = 0; i < fp.segment_count; ++i) {
            Segment s = fp.segments[i];
            printf("\nfpstart:%f | fpend:%f\n", fp.segments[i].start,
                   fp.segments[i].end);
            add_watched_segment(s.start, s.end);
        }
        volume = fp.volume;
    }

    printf("\n len of watched:%ld\n", arrlen(sc.watched_segments));
    for (int i = 0; i < arrlen(sc.watched_segments); ++i) {
        printf("\nstart:%f | end:%f\n", sc.watched_segments[i].start,
               sc.watched_segments[i].end);
    }

    player_set_volume(ctx, volume);
    player_unpause(ctx);
}

int wait_for_property(mpv_handle *ctx, const char *name) {
    mpv_observe_property(ctx, 0, name, MPV_FORMAT_NONE);

    while (1) {
        mpv_event *event = mpv_wait_event(ctx, 0);
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

    if (prop == NULL) return;

    TRACE(LOG_INFO, "on_property_change event->name=%s", prop->name);
    static int last_time = 0;
    if (strcmp(prop->name, "percent-pos") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
            percent_position = *(double *)prop->data;
            arrput(percent_positions, percent_position);

            double time_position = *(double *)prop->data;
            TRACE(LOG_INFO, "percent-pos:%.2f%%\n", percent_position);
        }
    }

    if (strcmp(prop->name, "pause") == 0) {
        assert(prop->format == MPV_FORMAT_FLAG);

        sc.paused = *(bool *)prop->data;
        TRACE(LOG_INFO, "Current %s: %s\n", prop->name, sc.paused ? "true" : "false");
    }

    if (strcmp(prop->name, "volume") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
            volume = *(double *)prop->data;
            TRACE(LOG_INFO, "Current %s: %.2f\n", prop->name, volume);
        }
    }

#define PLAYBACK_GAP_SIZE 2.0
    // playback-time does the same thing as time-pos but works for streaming
    // media
    if (strcmp(prop->name, "playback-time") == 0) {
        // NOTE: Even if we say we only want double format
        // sometimes its format is something else
        if (prop->format == MPV_FORMAT_DOUBLE) {
            double playback_time = *(double *)prop->data;
            double time_position = playback_time;
            TRACE(
                LOG_ERROR,
                "Current %s:%.2f sc.segment_start:%.2f sc.last_position:%.2f\n",
                prop->name, playback_time,
                sc.segment_start, sc.last_position
            );

            double start = sc.segment_start, last_position = sc.last_position;
            bool paused = sc.paused;

            if (paused || seeking) {
                if (start < last_position) {
                    add_watched_segment(start, last_position);
                }
                sc.segment_start = -1;
                sc.last_position = -1;
                goto playback_time_done;
            }

            if (last_position < 0, sc.segment_start < 0) {
                sc.segment_start = time_position;
                sc.last_position = time_position;
            } else if ((time_position < start)
                    || (fabs(time_position - last_position) > PLAYBACK_GAP_SIZE)) {
                // Gap in playback, end current segment
                add_watched_segment(start, last_position);

                sc.segment_start = time_position;
            }

            sc.last_position = time_position;
        }
    }
playback_time_done:

    // playback-time does the same thing as time-pos but works for streaming
    // media
    if (strcmp(prop->name, "osd-dimensions") == 0) {
        if (prop->format == MPV_FORMAT_INT64) {
            int64_t osd_dimensions = *(int64_t *)prop->data;
            assert(MPV_FORMAT_INT64 == prop->format);
            TRACE(LOG_INFO, "Current %s: %ld\n", prop->name, osd_dimensions);
        }
    }

    if (strcmp(prop->name, "duration") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
            sc.duration = *(double *)prop->data;
            TRACE(LOG_INFO, "Current %s: %f\n", prop->name, sc.duration);
        }
        // assert(0);
    }

    if (strcmp(prop->name, "time-pos") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
            double time_position = *(double *)prop->data;
            TRACE(LOG_INFO, "Current %s: %f\n", prop->name, time_position);
            // if (segment_control.last_position < 0) {
            //     segment_start = time_position;
            // } else if (time_position - segment_control.last_position > 1.0) {
            //     // Gap in playback, end current segment
            //     add_watched_segment(segment_start,
            //     segment_control.last_position); segment_start =
            //     time_position;
            // }
            //
            // segment_control.last_position = time_position;
        }
    }
}

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static int redraw = 1;

static void on_mpv_render_update(void *ctx) {
    mpv_render_context *mpv_rd = ctx;
    if (mpv_rd == NULL) {
        return;
    }
    uint64_t flags = mpv_render_context_update(mpv_rd);
    if (flags & MPV_RENDER_UPDATE_FRAME) {
        pthread_mutex_lock(render_update_mutex);
        redraw = 1;
        pthread_mutex_unlock(render_update_mutex);
    }
}

static int wakeup = 0;

static void on_mpv_events(void *ctx) {
    pthread_mutex_lock(event_wakeup_mutex);
    wakeup = 1;
    pthread_mutex_unlock(event_wakeup_mutex);
}

static void handle_mpv_events(mpv_handle *mpv) {
    static int times_called = 0;
    times_called += 1;
    while (mpv) {
        mpv_event *mp_event = mpv_wait_event(mpv, 0.0);
        // TRACE(LOG_ERROR, "times_called:%d event:%s\n", times_called,
        // mpv_event_name(mp_event->event_id));

        if (mp_event == NULL || mp_event->event_id == MPV_EVENT_NONE) break;

        /**
         * The meaning and contents of the data member depend on the event_id:
         *  MPV_EVENT_GET_PROPERTY_REPLY:     mpv_event_property*
         *  MPV_EVENT_PROPERTY_CHANGE:        mpv_event_property*
         *  MPV_EVENT_LOG_MESSAGE:            mpv_event_log_message*
         *  MPV_EVENT_CLIENT_MESSAGE:         mpv_event_client_message*
         *  MPV_EVENT_START_FILE:             mpv_event_start_file* (since
         *  MPV_EVENT_END_FILE:               mpv_event_end_file*
         *  MPV_EVENT_HOOK:                   mpv_event_hook*
         *  MPV_EVENT_COMMAND_REPLY*          mpv_event_command*
         *  other: NULL
         *
         * Note: future enhancements might add new event structs for existing or
         * new event types.
         */

        if (mp_event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            TRACE(LOG_INFO, "uint64_t reply_userdata = %ld;\n",
                  mp_event->reply_userdata);
            TRACE(LOG_INFO, "uint64_t error = %d;\n", mp_event->error);

            mpv_event_property *prop = mp_event->data;
            on_property_change(prop);
        }

        if (mp_event->event_id == MPV_EVENT_LOG_MESSAGE) {
            mpv_event_log_message *msg = mp_event->data;
            // Print log messages about DR allocations, just to
            // test whether it works. If there is more than 1 of
            // these, it works. (The log message can actually change
            // any time, so it's possible this logging stops working
            // in the future.)

            if (strstr(msg->text, "DR image")) printf("log: %s", msg->text);
            continue;
        }
        if (mp_event->event_id == MPV_EVENT_IDLE) {
        }
    }
}

bool IsKeyPressedOrRepeat(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static RenderTexture2D mpv_texture;
static RenderTexture2D gui_texture;
FilePathList mp4files;

int main(int argc, char *argv[]) {
    pthread_mutex_init(event_wakeup_mutex, NULL);
    pthread_mutex_init(render_update_mutex, NULL);
    pthread_rwlock_init(sc.rwlock, NULL);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    shdefault(file_progress_hash_map, HASH_DOES_NOT_EXIST);

    int len = load_progress_data(FILE_PROGRESS, &file_progress_darray);
    TRACE(LOG_INFO, "len = %d\n", len);
    for (int i = 0; i < arrlen(file_progress_darray); i++) {
        printf("File: %s\n", file_progress_darray[i].filename);
        printf("Segments:\n");
        for (int j = 0; j < file_progress_darray[i].segment_count; j++) {
            printf("  %.2f - %.2f\n", file_progress_darray[i].segments[j].start,
                   file_progress_darray[i].segments[j].end);
        }
        printf("\n");
        shput(file_progress_hash_map, strdup(file_progress_darray[i].filename), i);
    }

    if (argc != 2) die("pass a single media file as argument");
    mpv_handle *mpv = mpv_create();

    if (!mpv) die("context init failed");

    mpv_set_option_string(mpv, "force-window", "yes");
    int keep_open = 1;
    mpv_set_option(mpv, "keep-open", MPV_FORMAT_FLAG, &keep_open);
    mpv_set_option_string(mpv, "osc", "no");  // Disable on-screen controller
    mpv_set_option_string(mpv, "osd-level", "0");  // Disable on-screen display

    mpv_set_option_string(mpv, "input-default-bindings",
                          "no");  // Disable default key bindings
    mpv_set_option_string(mpv, "input-vo-keyboard",
                          "no");  // Disable keyboard input on video output
    mpv_set_option_string(mpv, "audio-display",
                          "no");  // Disable audio visualization

    mpv_check_error(mpv_set_option_string(mpv, "vo", "libmpv"));

    // mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);
    mpv_set_option_string(mpv, "input-cursor", "no");  // no mouse handling
    mpv_set_option_string(mpv, "cursor-autohide",
                          "no");  // no cursor-autohide, we handle that

    mpv_set_option_string(mpv, "ytdl", "yes");  // youtube-dl support
    mpv_set_option_string(mpv, "sub-auto",
                          "fuzzy");  // Automatic subfile detection
    mpv_set_option_string(
        mpv, "audio-client-name",
        "our-mplayer");  // show correct icon in e.g. pavucontrol

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0) die("mpv init failed");
    TRACE(LOG_INFO, "MPV initialized with success");
    // get length

    // set mpv options
    // int64_t wid;

    mpv_request_log_messages(mpv, "debug");

    mpv_check_error(mpv_observe_property(mpv, 69, "volume", MPV_FORMAT_DOUBLE));
    mpv_check_error(mpv_observe_property(mpv, 69, "sid", MPV_FORMAT_INT64));
    mpv_check_error(mpv_observe_property(mpv, 69, "aid", MPV_FORMAT_INT64));
    mpv_check_error(
        mpv_observe_property(mpv, 69, "sub-visibility", MPV_FORMAT_FLAG));
    mpv_check_error(mpv_observe_property(mpv, 69, "mute", MPV_FORMAT_FLAG));
    mpv_check_error(
        mpv_observe_property(mpv, 69, "core-idle", MPV_FORMAT_FLAG));
    mpv_check_error(
        mpv_observe_property(mpv, 69, "idle-active", MPV_FORMAT_FLAG));
    mpv_check_error(mpv_observe_property(mpv, 69, "pause", MPV_FORMAT_FLAG));
    mpv_check_error(
        mpv_observe_property(mpv, 69, "paused-for-cache", MPV_FORMAT_FLAG));
    mpv_check_error(
        mpv_observe_property(mpv, 69, "percent-pos", MPV_FORMAT_DOUBLE));

    mpv_check_error(
        mpv_observe_property(mpv, 420, "playback-time", MPV_FORMAT_DOUBLE));

    mpv_check_error(
        mpv_observe_property(mpv, 420, "time-pos", MPV_FORMAT_DOUBLE));

    mpv_check_error(
        mpv_observe_property(mpv, 69, "osd-dimensions", MPV_FORMAT_INT64));

    mpv_check_error(
        mpv_observe_property(mpv, 69, "duration", MPV_FORMAT_DOUBLE));

// #define FILES_LISTING_LIMIT (8*2 + 3)
#define FILES_LISTING_LIMIT (222)

#if defined(MINGW)
    // const char* dir =
    // "E:\\Torrents\\Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN";
    // const char *dir = "C:\\Users\\Administrator\\Downloads";
    const char *dir = "D:\\Donwloads";
    // TODO: Add better file filtering for Raylib
    mp4files = LoadDirectoryFilesEx(dir, ".mp4", true);
    // mp4files = LoadDirectoryFiles(dir);
#else
    mp4files =
        LoadDirectoryFilesEx("/home/excyber/media/videos/", ".mp4", true);
#endif

    mp4files.count = mp4files.count < FILES_LISTING_LIMIT ? mp4files.count
                                                          : FILES_LISTING_LIMIT;

    printf("mp4files.count: %d\n", mp4files.count);
    for (int i = 0; i < mp4files.count; ++i) {
        printf("%s\n", mp4files.paths[i]);
    }

    // SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MINGW");

    RenderTexture2D target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    // SetTextureFilter(target.texture, TEXTURE_FILTER_ANISOTROPIC_16X);
    // GenTextureMipmaps(&target.texture);

    // Shader fxaaShader = LoadShader(0, "./res/shaders/fxaa-thebennybox.fs");
    Shader fxaaShader = LoadShader(NULL, "./res/shaders/fxaa.fs");
    // Shader fxaaShader = LoadShader(NULL, NULL);
    // Shader shader = LoadShaderFromMemory(NULL, fxaaShader);
    // Shader shader = LoadShader("./res/shaders/base.vs", "./res/shaders/predator.fs");



    // Custom file dialog
    GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
    char fileNameToLoad[PATH_MAX] = { 0 };


#define FONT_SIZE (40/2)

    Font jetbrains = LoadFontEx(
        "./res/fonts/JetBrainsMonoNerdFont-Medium.ttf", FONT_SIZE, NULL, 0);
    GenTextureMipmaps(&jetbrains.texture);
    SetTextureFilter(jetbrains.texture, TEXTURE_FILTER_BILINEAR);


    // FILTER_TRILINEAR requires generated mipmaps
    // SetTextureFilter(jetbrains.texture, TEXTURE_FILTER_TRILINEAR);
    // SetTextureFilter(jetbrains.texture, TEXTURE_FILTER_ANISOTROPIC_16X);
    GuiSetFont(jetbrains);
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_SIZE, FONT_SIZE);


    mpv_texture = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    SetWindowState(
        FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN
        // support mouse passthrough, only supported when
        | FLAG_WINDOW_UNDECORATED
        // FLAG_WINDOW_MOUSE_PASSTHROUGH
        | FLAG_VSYNC_HINT
        // run program in borderless windowed mode
        // | FLAG_BORDERLESS_WINDOWED_MODE
    );


    TRACE(LOG_INFO, "Screen Size %dx%d", GetScreenWidth(), GetScreenHeight());

#if defined(SOFTWARE_RENDERER)

    mpv_render_context *mpv_rd;
    {
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_SW},
            // Tell libmpv that you will call mpv_render_context_update() on
            // render context update callbacks, and that you will _not_ block
            // on the core ever (see <libmpv/render.h> "Threading" section for
            // what libmpv functions you can call at all when this is active).
            // In particular, this means you must call e.g. mpv_command_async()
            // instead of mpv_command().
            // If you want to use synchronous calls, either make them on a
            // separate thread, or remove the option below (this will disable
            // features like DR and is not recommended anyway).

            {MPV_RENDER_PARAM_ADVANCED_CONTROL, &(int){1}},
            {0}};

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
            {0}};
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
    mpv_render_context_set_update_callback(mpv_rd, on_mpv_render_update,
                                           mpv_rd);
#else
    mpv_render_context_set_update_callback(mpv_gl, on_mpv_render_update,
                                           mpv_gl);
#endif

    printf("About to load texture\n");
    Image image = {NULL, WINDOW_WIDTH, WINDOW_HEIGHT, 1,
                   PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture tex = LoadTextureFromImage(image);

    Image shapesImage = {NULL, WINDOW_WIDTH*4, WINDOW_HEIGHT*4, 1,
                   PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture shapesTexture = LoadTextureFromImage(shapesImage);

    if (true) {
        // GenTextureMipmaps(&shapesTexture);
        // SetTextureFilter(shapesTexture, TEXTURE_FILTER_TRILINEAR);
        // SetTextureFilter(shapesTexture, TEXTURE_FILTER_ANISOTROPIC_16X);

        // SetShapesTexture(shapesTexture, GetShapesTextureRectangle());

    }
    UnloadImage(image);

    printf("Loaded Texture just fine\n");

// NOTE: Alternatively, stat() can be used instead of access()
#include <sys/stat.h>
    // struct stat statbuf;
    // if (stat(filename, &statbuf) == 0) result = true;

    if (!nob_file_exists(argv[1]) || !FileExists(argv[1])) {
        TRACE(LOG_ERROR, "File %s does not exist, nothing is being played",
              argv[1]);
    } else {
        percent_positions = NULL;
        arrsetlen(percent_positions, 0);
        player_load_file(mpv, argv[1]);
    }
    player_pause(mpv);

    // Wait until the file is loaded

    mpv_wait_event(mpv, -1.0);

// double pfps = mpv_get_property_double(mpv, "container-fps");

// int64_t total_frames = (int64_t)(duration * pfps);

// printf("duration:%f fps:%f total_frames:%ld\r", duration, pfps,
// total_frames); #define FPS 75 #define FPS 60
#define FPS 35
    SetTargetFPS(FPS);
    char fps[123];

    void *pixels = malloc(sizeof(char) * 4 * 2000 * 2000);
    bool holding_crtl_click_before = false;
    Vector2 holding_crtl_click_init_position = {0,0};

    int active = -1 , focus = 0;
    Vector2 scrollIndex = {4, 4};
    while (!WindowShouldClose()) {
        if (fileDialogState.SelectFilePressed)
        {
            // Load image file (if supported extension)
            if (IsFileExtension(fileDialogState.fileNameText, ".png"))
            {
                static Texture fileDialogTexture = {0};
                strcpy(fileNameToLoad, TextFormat("%s""/""%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
                UnloadTexture(fileDialogTexture);
                fileDialogTexture = LoadTexture(fileNameToLoad);
            }

            fileDialogState.SelectFilePressed = false;
        }

        int resolutionLoc = GetShaderLocation(fxaaShader, "resolution");
        // Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        Vector2 resolution = { (float)target.texture.width, (float)target.texture.height};
        SetShaderValue(fxaaShader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);

        BeginTextureMode(target);
        ClearBackground(BLACK);

        Vector2 mouse_position  = GetMousePosition();
        Vector2 mouse_wheel     = GetMouseWheelMoveV();
        Vector2 window_size     = CLITERAL(Vector2){GetScreenWidth(), GetScreenHeight()};
        Vector2 window_position = GetWindowPosition();
        Vector2 mouse_delta     = GetMouseDelta();

        TRACE(LOG_TRACE, "mouse_wheel = {%f, %f};", mouse_wheel.x,
              mouse_wheel.y);

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
        } else if (holding_crtl_click_before || IsKeyDown(KEY_LEFT_CONTROL)) {
            Vector2 diff;
            // Both work, but is there a difference ?
            // NOTE: Yes there is, demousedelta is janky
            // diff = GetMouseDelta();
            diff = Vector2Subtract(mouse_position, holding_crtl_click_init_position);

            Vector2 result_position = Vector2Add(window_position, diff);

            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (!holding_crtl_click_before) {
                    holding_crtl_click_before = true;
                    holding_crtl_click_init_position = CLITERAL(Vector2){mouse_position.x, mouse_position.y };
                } else {
                    SetWindowPosition(result_position.x, result_position.y);
                }
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
                holding_crtl_click_before = false;

                printf("holding_crtl_click_init_position = {%f, %f}\n", holding_crtl_click_init_position.x, holding_crtl_click_init_position.y);
                printf("curret_position = {%f, %f}\n", mouse_position.x, mouse_position.y);
                printf("diff = {%f, %f}\n", diff.x, diff.y);
                printf("window_position = {%f, %f}\n", window_position.x, window_position.y);
                printf("result_position = {%f, %f}\n", result_position.x, result_position.y);
            }

            if (IsKeyPressed(KEY_C)) {
                goto done;
            }
        }

        {
            if (!draw_file_list && mouse_wheel.y != 0.0) {
                char temp[5];
                int diff = (int)(5.0 * mouse_wheel.y);
                snprintf(temp, sizeof(temp), "%d", diff);
                const char *cmd_pause[] = {"osd-auto", "add", "volume", temp,
                                           NULL};
                mpv_command_async(mpv, 0, cmd_pause);
            }
        }

        if (IsKeyPressed(KEY_SPACE)) {
            const char *cmd_pause[] = {"cycle", "pause", NULL};
            mpv_command_async(mpv, 0, cmd_pause);
        } else if (IsKeyPressed(KEY_S)) {
            // Also requires MPV_RENDER_PARAM_ADVANCED_CONTROL if you want
            // screenshots to be rendered on GPU (like --vo=gpu would do).
            const char *cmd_scr[] = {"screenshot-to-file", "screenshot.png",
                                     "window", NULL};
            printf("attempting to save screenshot to %s\n", cmd_scr[1]);

            mpv_command_async(mpv, 0, cmd_scr);
        }

        pthread_mutex_lock(event_wakeup_mutex);
        bool woke = wakeup;
        wakeup = false;
        pthread_mutex_unlock(event_wakeup_mutex);

        if (woke) {
            handle_mpv_events(mpv);
        }

        pthread_mutex_lock(render_update_mutex);
        bool should_redraw = redraw;
        pthread_mutex_unlock(render_update_mutex);

        if (should_redraw) {
#if defined(SOFTWARE_RENDERER)
            int w = GetScreenWidth();
            int h = GetScreenHeight();
            int pitch = w * 4;

            mpv_render_param params[] = {
                {MPV_RENDER_PARAM_SW_SIZE, (int[2]){w, h}},
                {MPV_RENDER_PARAM_SW_FORMAT, "rgba"},
                {MPV_RENDER_PARAM_SW_STRIDE, &(size_t){pitch}},
                {MPV_RENDER_PARAM_SW_POINTER, pixels},
                {0}};
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
                {MPV_RENDER_PARAM_OPENGL_FBO, &fbo}, {0}};
            mpv_render_context_render(mpv_gl, render_params);

#endif
        }

        if (mouse_position.x > GetScreenWidth() / 1.4) {
            draw_file_list = false;
        }
#if defined(SOFTWARE_RENDERER)
        // Create a texture from the RGBA pixel data
        //
        // TODO: Change to UpdateTexture in a way that it works with
        // change in widht and height, rn we need to Load Everytime, which is
        // wasteful as shit we can even print something to check if we're
        // getting new texture id's all the time problably yeah
        //
        // example: UpdateTextureRec(tex, CLITERAL(Rectangle){0, 0, image.width,
        // image.height}, image.data);
        //

        // tex = LoadTextureFromImage(image);
        UpdateTextureRec(tex, CLITERAL(Rectangle){0, 0, image.width, image.height}, image.data);
        // Draw video texture
        if (mouse_position.x < 20) {
            DrawTextureEx(
                tex,
                CLITERAL(Vector2){GetScreenWidth() * 0.5,
                                  GetScreenHeight() * 0.5 - (tex.height/2)},
                0, 0.5, RAYWHITE);
            draw_file_list = true;
        } else {
            DrawTexture(tex, 0, 0, RAYWHITE);
        }
#else
        DrawTextureRec(
            mpv_texture.texture,
            CLITERAL(Rectangle){0, 0, (float)mpv_texture.texture.width,
                                (float)mpv_texture.texture.height},
            CLITERAL(Vector2){0, 0}, WHITE);
#endif


        TRACE(LOG_TRACE, "mouse_pos = {%d %d};\n", (int)mouse_position.x,
              (int)mouse_position.y);

        {
            float alpha = 2.0;
            GuiMoDrawCircle(mouse_position, 5.f, 1.0, ColorAlpha(WHITE, alpha), ColorAlpha(RED, alpha));
        }


        snprintf(fps, sizeof(fps), " %d fps", GetFPS());
        SetWindowTitle(fps);

        // TRACE(LOG_NONE, "FPS:%d; Target_FPS=%d",GetFPS(), FPS);

        // Draw progress bar background
        {
            int padding = 20;
            Rectangle playback_rect = {
                padding, GetScreenHeight() - PROGRESS_BAR_HEIGHT - padding,
                GetScreenWidth() - 2*padding, PROGRESS_BAR_HEIGHT
            };
            double tracking_percent_position = percent_position;
            Segment current = {sc.segment_start, sc.last_position};


            DrawRectangleRec(Pad(playback_rect, 2.0), Fade(BLACK, 0.3));



            if (false) { // test

                BeginTextureMode(target);
                    ClearBackground(BLANK); // BLANK is (0,0,0,0) - fully transparent
                    // Draw only where you want content, the rest stays transparent

                    Rectangle bounds = {100, 100, 200, 150};
                    DrawRectangleRec(bounds, ColorAlpha(BLUE, 0.5f)); // Semi-transparent blue
                    DrawCircle(GetMouseX(), GetMouseY(), 20, RED);

                    bounds.x += bounds.width;
                    DrawRectangleRec(bounds, ColorAlpha(BLACK, 0.5f)); // Semi-transparent blue
                EndTextureMode();
            }
            int ret = GuiMoTrackingBar(playback_rect, sc.watched_segments,
                                           sc.segment_count, sc.duration, current,
                                           &tracking_percent_position, &seeking);
            if (seeking == true) {
                char temp[128];
                snprintf(temp, sizeof(temp), "%f",
                         tracking_percent_position * 100);
                const char *cmd_seek[] = {"seek", temp, "absolute-percent",
                                          "exact", NULL};
                mpv_command_async(mpv, 420, cmd_seek);
                // mpv_command(mpv, cmd_seek);
            }

            // Merge overlapping segments
            // merge_segments();
        }

        {
            const int volume_pad = 3;
            const int volume_height = 10;
            const int volume_width = 200;

            // DrawRectangle(0, 0, 2, PROGRESS_BAR_HEIGHT/2, BLACK);
            // DrawRectangle(0, 0, ((150.0 / 100.0) * volume_width), volume_height,
            //               RED);
            // DrawRectangle(0, 0, ((120.0 / 100.0) * volume_width), volume_height,
            //               YELLOW);
            // DrawRectangle(0, 0, ((100.0 / 100.0) * volume_width), volume_height, BLACK);
            // DrawRectangle(volume_pad, volume_pad,
            //               ((volume / 100.0) * volume_width) - 2 * volume_pad,
            //               (volume_height - 2 * volume_pad), GREEN);

            Rectangle volumeBounds = { 0, 0, ((100.0 / 100.0) * volume_width), volume_height };
            static bool volumeSeek = false;
            double volumePercentage = volume;
            GuiMoVolume(volumeBounds, &volumePercentage, 130.0, &volumeSeek);
            if (volumeSeek) {
                player_set_volume(mpv, volumePercentage);
            }
        }

        static bool showMenu = false;

        static Vector2 menu_scroll_percent = { 0 } ;
        if (draw_file_list) {
            const char **text = (const char **)mp4files.paths;
            int active_old = active;
            static Vector2 scroll_percentage = {0};
            float list_view_width = window_size.x / 4;
            Rectangle filesBounds =
                CLITERAL(Rectangle){window_size.x / 6 - list_view_width / 2, 20,
                                    list_view_width, window_size.y / 1.5};

            int a = GuiMoListView(filesBounds, text, mp4files.count,
                                  &scroll_percentage, &active, &focus,
                                  !showMenu, 12);
            static bool donel = false;
            if (active_old != active && active >= 0 &&
                active < mp4files.count) {
                player_load_file(mpv, mp4files.paths[active]);
            }
            assert(a == 0);

            {
                static Rectangle menuBounds = {0};

                // Only check for menu if we're in file_list
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
                    && CheckCollisionPointRec(mouse_position, filesBounds)
                ) {
                    menuBounds = CLITERAL(Rectangle){ mouse_position.x, mouse_position.y, 200, 70};
                    showMenu = true;
                }

                int active = -1, focus = 0;
                if (showMenu) {
                    const char *options[] = {
                        "yank path", "huuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuge",
                        "copy file", "copy file", "copy file"
                    };
                    int count = sizeof(options) / sizeof(options[0]);
                    // SCROLL_SLIDER_SIZE
                    GuiSetStyle(SCROLLBAR, SCROLL_SLIDER_SIZE, menuBounds.height/count);
                    int a = GuiMoListView(menuBounds, options, count,
                                          &menu_scroll_percent, &active, &focus,
                                          true, 8);

                    int hit_box_margin = 20;
                    Rectangle men_hit_box = {
                        menuBounds.x - hit_box_margin,
                        menuBounds.y - hit_box_margin,
                        menuBounds.width + 2 * hit_box_margin,
                        menuBounds.height + 2 * hit_box_margin};
                    bool is_mouse_on_menu_bar =
                        CheckCollisionPointRec(mouse_position, men_hit_box);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                        !is_mouse_on_menu_bar) {
                        showMenu = false;
                    }
                }
            }
        } else {
            showMenu = false;
        }

        int drop_box_active = true;
        // GuiDropdownBox(CLITERAL(Rectangle){0,0, 200, 200}, "kkkkkkkkkkkk", &drop_box_active, true);
        // GuiScrollBar(CLITERAL(Rectangle){200, 200, 400, 400},29, 19, 13);
        TRACE(LOG_TRACE, "scrollIndex = %d, active = %d, focus = %d", scrollIndex, active, focus);
        {
            static int show = false;
            Rectangle bounds = {window_size.x/2-100, window_size.y/2-100, 200, 200};

            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_F)) {
                show = !show;
            }

            if (show) {
                static int edit_mode  = true;
                bool secretViewActive = false;
                const char *buttons   = "";


                int  text_size = FONT_SIZE;
                static char text[PATH_MAX];
                int  clicked   = GuiTextBox(bounds, text, text_size, edit_mode);
                // int clicked = GuiTextInputBox(bounds, "title", "",
                //         buttons, text, PATH_MAX,
                //         &secretViewActive);

                static int spinner_value = 29;
                GuiSpinner(CLITERAL(Rectangle){700, 700, 20, 20}, text, &spinner_value, 0, 100, false);
                // printf("text = %s", text);
            }
        }
        static const int close_button_size = 25;

        GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, ColorToInt(CLITERAL(Color){202, 17, 35, 0xFF}));
        GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, ColorToInt(RAYWHITE));
        GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED, ColorToInt(CLITERAL(Color){202, 17, 35, 0xFF}));
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, ColorToInt(CLITERAL(Color){0xFF, 17, 35, 0xFF}));
        GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED, ColorToInt(CLITERAL(Color){0xFF, 17, 35, 0xFF}));
        int xbutton_clicked = GuiButton( CLITERAL(Rectangle){window_size.x-close_button_size, 0, close_button_size, close_button_size},
            GuiIconText(ICON_CROSS_SMALL, NULL));
            // GuiIconText(ICON_CROSS, NULL));
            if (xbutton_clicked) {
                goto done;
            }

        GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, ColorToInt(CLITERAL(Color){22, 17, 35, 0xFF}));
        GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, ColorToInt(RAYWHITE));
        GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED, ColorToInt(CLITERAL(Color){22, 17, 35, 0xFF}));
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, ColorToInt(CLITERAL(Color){0, 17, 35, 0xFF}));
        GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED, ColorToInt(CLITERAL(Color){0, 17, 35, 0xFF}));
        int openFile = GuiButton(CLITERAL(Rectangle){window_size.x-2*close_button_size, 0, close_button_size, close_button_size}, GuiIconText(ICON_FILE_OPEN, ""));
        if (openFile || IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
            //----------------------------------------------------------------------------------
            fileDialogState.windowActive = true;
        }

        if (fileDialogState.windowActive) {
            GuiLock();
        }

        GuiUnlock();
        GuiWindowFileDialog(&fileDialogState);

        if (fileDialogState.SelectFilePressed) {
            if (IsFileExtension(fileDialogState.fileNameText, ".mp4")
                || IsFileExtension(fileDialogState.fileNameText, ".mkv")
            ) {
                strcpy(fileNameToLoad, TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
                player_load_file(mpv, fileNameToLoad);
            }

            fileDialogState.SelectFilePressed = false;
        }

        static float horz_value = 0.2;
        static bool  horz_dragging = false;

    EndTextureMode();

    BeginDrawing();
        // BeginShaderMode(fxaaShader);
        DrawTextureRec(target.texture,
                      (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height },
                      (Vector2){ 0, 0 },
                      WHITE);  // WHITE preserves alpha channel
        // EndShaderMode();

    EndDrawing();
    nob_temp_reset();

    }

done:
    // CloseWindow();
    // Add final segment
    // ■ Label followed by a declaration is a C23 extension
    double start = sc.segment_start, end = sc.last_position;

    add_watched_segment(start, end);
    collect_file_progress_info();
    reset_segments_info();

    print_all_file_progress();

    store_progress_data(FILE_PROGRESS, file_progress_darray,
                        arrlen(file_progress_darray));
    TRACEINFO("Progress file stored in `%s`", FILE_PROGRESS);


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

void print_all_file_progress() {
    printf("TOTAL: %ld\n", arrlen(file_progress_darray));
    for (int i = 0; i < arrlen(file_progress_darray); i++) {
        printf("File: %s volume:\n", file_progress_darray[i].filename);
        printf("Volume: %f\n", file_progress_darray[i].volume);
        printf("Segments:\n");
        for (int j = 0; j < file_progress_darray[i].segment_count; j++) {
            printf("  %.2f - %.2f\n", file_progress_darray[i].segments[j].start,
                   file_progress_darray[i].segments[j].end);
        }
        printf("\n");
    }
}
