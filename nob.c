#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "./nob.h"

#define BUILD_DIR ".build"
#define SRC_DIR "./src"
#define BIN "main"



// #define AR     "x86_64-w64-mingw32-ar"
#define AR "ar"
// #define PLATFORM_SDL
// #define PLATFORM_RGFW

#define RAYLIB_BUILD_DIR BUILD_DIR "/raylib"
#define RAYLIB_SOURCE_DIR "./src/vendor/raylib/src"


static const char *raylib_modules[] = {
    "rcore",   "raudio", "rglfw",     "rmodels",
    "rshapes", "rtext",  "rtextures", "utils",
};

#if (defined(__MINGW64__) || defined(__MINGW32__))
    static const char *CC = "x86_64-w64-mingw32-gcc";
#else
    static const char *CC = "gcc";
#endif

static struct {
    bool run;
    bool examples;
    bool cyber_player;
    bool clip;
    bool mingw;
    struct {
        bool mpv;
        bool glib;
        bool raylib;
    } libs;
} flags = {0};

static const char *libraylib_path = NULL;

// regex: "raylib_.*\.c"
static bool filter_base_name_starts_with_raylib_and_ends_with_dot_c(const char *path, void *user_data)
{
    const char *section        = "raylib_";

    const size_t section_count = strlen(section);
    const size_t path_count    = strlen(path);
    const size_t ext_count     = strlen(".c");

    if (path_count < section_count || ext_count > section_count) {
        return false;
    }

    bool starts = memcmp(nob_base_name(path), section, section_count)      == 0;
    bool ends   = memcmp(path + (path_count - ext_count), ".c", ext_count) == 0;
    return starts && ends;
}


#if defined(_WIN32) && (defined(__MINGW64__) || defined(__MINGW32__))
// #   include "dirent.h" // Conflicts with defined in nob.h only in mingw
#   define realpath(N, R) _fullpath((R), (N), PATH_MAX)
#else
#   include "dirent.h"
#endif

typedef Nob_Cmd CStr_Array;
// Gen without the []
void gen_compile_commands_json(String_Builder* sb, Cmd cmd, File_Paths hfiles) {

    // char* out =  tprintf("%s%s\0",
    char root_dir[PATH_MAX];
    realpath("./", root_dir);

    sb_printf(sb,
        "  {\n"
        "    \"directory\": \"%s\",\n"
        "    \"arguments\": [\n" ,
                root_dir
    );

    struct {
        int* items;
        size_t count;
        size_t capacity;
    } cfiles_indices = {0};

    const char *comma = cmd.count > 0 ? "," : "";

    { // Extra Defines for debug  in .h files

        sb_append_cstr(sb, "    \"");
        static const bool working_on_mingw = true;
        if (working_on_mingw) {
            sb_printf(sb, "%s", "x86_64-w64-mingw32-gcc");
        } else {
            sb_printf(sb, "%s", "cc");
        }

        sb_printf(sb, "\"%s\n", comma);

        CStr_Array extra_defines = {0};
        const char *extra_defines_cstrs[] = {
            "GUI_IMPLEMENTATION", "NOB_STRIP_PREFIX", "NOB_IMPLEMENTATION",
            "RAYGUI_IMPLEMENTATION", "RAYGUI_MO_IMPLEMENTATION",
            "RLGL_IMPLEMENTATION",

            "GUI_FILE_DIALOG_IMPLEMENTATION",
            "GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION",

            /* "_WIN32", */ "__MINGW64__"
        };
        size_t extra_defines_cstrs_count = sizeof(extra_defines_cstrs) / sizeof(extra_defines_cstrs[0]);

        if(extra_defines_cstrs_count != 6)
            nob_log(WARNING, "extra_defines_cstrs_count %zu", extra_defines_cstrs_count);


        // da_append_all(&extra_defines, "anther", "thing"); // idlike to make this work? is there anyway?
        da_append_many(&extra_defines, extra_defines_cstrs, extra_defines_cstrs_count);
        da_for(&extra_defines) {
            sb_append_cstr(sb, "    \"");
            sb_printf(sb, "-D%s", it);
            nob_log(INFO, "define %s", it);
            sb_printf(sb, "\"%s\n", comma);
        }


    }

    // Skip first expects compiler ( we replace with "cc")
    for (int i = 1; i < cmd.count; ++i) {
        char *ext = strrchr(cmd.items[i], '.');
        if (ext != NULL && strcmp(ext, ".c") == 0 ) {
            da_append(&cfiles_indices, i);
        }

        comma = i == cmd.count - 1 ? "" : ",";

        sb_append_cstr(sb, "    \"");
        sb_append_cstr(sb, cmd.items[i]);
        sb_printf(sb, "\"%s\n", comma);
    }

    comma = cfiles_indices.count == 0 ? "" : ",";
    // TODO: Make a more robust sb_printf not using tpritnf ok?
    sb_printf(sb, "    ]%s\n", comma);



    for (size_t i = 0; i < hfiles.count; ++i) {
        const char *comma = i == (hfiles.count + cfiles_indices.count) - 1 ? "" : ",";
        const char* hfile = hfiles.items[i];
        sb_printf(sb, "    \"file\": \"%s\"", hfile);
        sb_printf(sb, "%s\n", comma);
    }

    for (size_t i = 0; i < cfiles_indices.count; ++i) {
        const char *comma = i == cfiles_indices.count - 1 ? "" : ",";
        // char cfile[PATH_MAX]; realpath(cmd.items[cfiles.items[i]], cfile);
        const char* cfile = cmd.items[cfiles_indices.items[i]];
        sb_printf(sb, "    \"file\": \"%s\"", cfile);
        sb_printf(sb, "%s\n", comma);
    }
    sb_append_cstr(sb, "  }\n");
}

bool build_clipboard() {
    Cmd cmd = {0};

    bool result = true;
    const char* dir = BUILD_DIR"/clipboard";
    nob_mkdir_if_not_exists(dir);

    const char *objpath = tprintf("%s/clipboard.o", dir);
    const char *src = tprintf("%s/clipboard.c", SRC_DIR);

    {
        cmd_append(&cmd, CC, "-c", src);
        cmd_append(&cmd, "-fPIC", tprintf("%s%s","-o", objpath), "-lgdi32");
        if (!cmd_run_sync(cmd)) return_defer(false);
        nob_log(INFO, "OK: %s %s",CC, __PRETTY_FUNCTION__);
    }

    {
        cmd.count = 0;
        const char *libpath = tprintf("%s/libclipboard.a", dir);
        File_Paths object_files = {0};
        da_append(&object_files, objpath);
        if (needs_rebuild(libpath, object_files.items, object_files.count)) {
            cmd_append(&cmd, AR, "-crs", libpath);

            for (int idx = 0; idx < object_files.count; idx += 1) {
                cmd_append(&cmd, object_files.items[idx]);
            }

            if (!cmd_run_sync(cmd)) return_defer(false);
        }
        nob_log(INFO, "OK: %s %s", AR, __PRETTY_FUNCTION__);
    }

defer:
    cmd_free(cmd);
    return result;
}

bool build_raylib() {
    bool result = true;
    Cmd cmd = {0};
    File_Paths object_files = {0};
    File_Paths header_files = {0};
    File_Paths c_files = {0};

    read_entire_dir_filtered(RAYLIB_SOURCE_DIR, &header_files, true, nob_filter_by_extension, ".h");
    read_entire_dir_filtered(RAYLIB_SOURCE_DIR, &c_files, true, nob_filter_by_extension, ".c");
    read_entire_dir_filtered(RAYLIB_SOURCE_DIR"/platforms", &c_files, true, nob_filter_by_extension, ".c");

    // if (!mkdir_if_not_exists(RAYLIB_BUILD_DIR)) {
    //     return_defer(false);
    // }

    const char *build_path = "";
    if (flags.mingw) {
        build_path = tprintf("%s"PATH_SEPARATOR"%s", RAYLIB_BUILD_DIR, "mingw");
    } else {
        build_path = tprintf("%s"PATH_SEPARATOR"%s", RAYLIB_BUILD_DIR, "linux");
    }


    if (!nob_mkdir_if_not_exists_recursive(build_path)) {
        return_defer(false);
    }

    Procs procs = {0};

    for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path =
            temp_sprintf(RAYLIB_SOURCE_DIR"/%s.c", raylib_modules[i]);
        const char *output_path =
            temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        da_append(&object_files, output_path);

        if (
            needs_rebuild(output_path, header_files.items, header_files.count)
            || needs_rebuild(output_path, c_files.items, c_files.count)
            || needs_rebuild(output_path, &input_path, 1)
        ) {
            cmd.count = 0;
            cmd_append(&cmd, CC);
            if (flags.mingw) cmd_append(&cmd, "-mwindows");

            cmd_append(&cmd,
                       "-fPIC",
        #ifdef PLATFORM_SDL
                       "-DPLATFORM_DESKTOP_SDL",
        #elif defined(PLATFORM_RGFW)
                       "-DPLATFORM_DESKTOP_RGFW",
        #else
                       "-DPLATFORM_DESKTOP",
        #endif

                       "-DSUPPORT_CLIPBOARD_IMAGE=1",
                       "-DSUPPORT_FILEFORMAT_BMP=1",
                       #ifdef DEBUG_RAYLIB
                           "-ggdb",
                           "-DRLGL_SHOW_GL_DETAILS_INFO=1"
                        #endif
                       "-DSUPPORT_FILEFORMAT_FLAC=1"
            );

            cmd_append(&cmd, "-fmax-errors=5");
            if (!flags.mingw) {
                cmd_append(&cmd, "-D_GLFW_X11");
            }

            // cmd_append(&cmd, "-I"RAYLIB_SOURCE_DIR"external/glfw/include");
#define CURRENT_FULL_PATH "C:/msys64/home/Administrator/code/unamed-video"
        #ifdef PLATFORM_SDL
            cmd_append(&cmd,
                   // "-l:C:/msys64/mingw64/lib/libSDL2.a",
                   // "-IC:/msys64/mingw64/include/SDL2"
                   // "-lSDL2", "-lSDL2Main",


                   "-l:"CURRENT_FULL_PATH"/src/vendor/SDL3-x64-windows/lib/SDL3.lib",
                   "-I"CURRENT_FULL_PATH"/src/vendor/SDL3-x64-windows/include/SDL3"
                   // "-I"CURRENT_FULL_PATH"/src/vendor/SDL3-x64-windows/include/"
                   // "-I/mingw64/include/SDL2"
            );

        #elif defined(PLATFORM_RGFW)
            cmd_append(&cmd, "-lgdi32", "-lopengl32");
        #endif
            cmd_append(&cmd, tprintf("-I%s", RAYLIB_SOURCE_DIR"/external/glfw/include"));
            cmd_append(&cmd, "-c", input_path);
            cmd_append(&cmd, "-o", output_path);

            if(false || flags.mingw) {
                cmd_append(&cmd, "-lwinmm", "-lgdi32");
                cmd_append(&cmd, "-static");
            }

            Proc proc = cmd_run_async(cmd);
            da_append(&procs, proc);
        }
    }

    cmd.count = 0;

    if (!procs_wait(procs)) return_defer(false);

    nob_log(INFO, "OK: waited procs %s", __PRETTY_FUNCTION__);

    libraylib_path = strdup(tprintf("%s/libraylib.a", build_path));

    if (needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        cmd_append(&cmd, AR, "-crs", libraylib_path);
        for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path = tprintf("%s/%s.o", build_path, raylib_modules[i]);
            cmd_append(&cmd, input_path);
        }

        if (!cmd_run_sync(cmd)) return_defer(false);
    }
    nob_log(INFO, "OK: create AR %s", __PRETTY_FUNCTION__);


defer:
    cmd_free(cmd);
    da_free(object_files);
    return result;
}

bool build_with_libs(const char *sources[]) {
    bool should_build_clipboard = false;
    bool result = true;
    Cmd cmd = {0};
    Procs procs = {0};
    cmd.count = 0;

    cmd_append(&cmd, CC);

    #ifdef _WIN32
        if (should_build_clipboard && !build_clipboard()) return 1;
    #endif
    // "sr.c", "progress.c";

    const char * output_arg = NULL;

    const char *source = NULL;
    for (int i = 0; (source = sources[i]); ++i) {
        if (sv_starts_with(sv_from_cstr(source), sv_from_cstr("-o"))) {
            if(output_arg)  {
                nob_log(ERROR, "Output path chosen twice first time `%s`, now ``.",output_arg, source);
                exit(69);
            }
            output_arg = source;
        }
        nob_log(INFO, "Appending Source `%s`\n", source);
        cmd_append(&cmd, source,);
    }


    if (output_arg == NULL)  {
        cmd_append(&cmd, temp_sprintf("-o%s/%s", BUILD_DIR, BIN));
    }


    cmd_append(&cmd, "-fmax-errors=5");
    // OpenGL renderer is broken rn
    cmd_append(&cmd, "-DSOFTWARE_RENDERER");
    cmd_append(&cmd, "-D_REENTRANT");
    cmd_append(&cmd, "-DDEBUG");




    cmd_append(&cmd,
        // "-Wall",
        // "-Wextra"
    );
    #if defined(DEBUG)
        cmd_append(&cmd, "-O0", "-ggdb");
    #elif  defined(RELEASE)
        cmd_append(&cmd, "-O3");
    #endif

    // "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include","-I/usr/include/sysprof-6","-lglib-2.0"
    cmd_append(&cmd, "-I.", "-I./src/include/",  "-pthread");

    if (flags.libs.glib)   cmd_append(&cmd,  "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include" ,"-I/usr/include/sysprof-6", "-pthread", "-lglib-2.0");
    if (flags.libs.mpv)    cmd_append(&cmd, "-lmpv");
    // if (flags.libs.raylib) cmd_append(&cmd, "-I" RAYLIB_SOURCE_DIR, "-I./src/vendor/", "-L./",  "-l:./"RAYLIB_BUILD_DIR"/libraylib.a", "-lpthread", "-lm");
    if (flags.libs.raylib) {
        assert(libraylib_path != NULL);
        cmd_append(&cmd, "-I" RAYLIB_SOURCE_DIR, "-I./src/vendor/", "-L./",  tprintf("-l:%s", libraylib_path), "-lpthread", "-lm");
    }


        #ifdef PLATFORM_SDL
            cmd_append(&cmd,
                   // "-LC:/msys64/mingw64/lib/",
                   // "-l:C:/msys64/mingw64/lib/libSDL2.a",
                   "-lSDL2", "-lSDL2Main",
                   // "-IC:/msys64/mingw64/include/SDL2"
                   // "-I/mingw64/include/SDL2"
            );
        #elif defined(PLATFORM_RGFW)
            cmd_append(&cmd,
                   "-lgdi32",
                   "-lopengl32",
            );
        #endif

    if (flags.mingw) {
        if (should_build_clipboard) {
            cmd_append(&cmd,"-DSUPPORT_CLIPBOARD_IMAGE=1");
            cmd_append(&cmd,"-L./", "-l:./"BUILD_DIR"/clipboard/libclipboard.a");
        }
        cmd_append(&cmd, "-L/mingw64/bin/", "-L/mingw64/lib/", "-I/usr/include/SDL2", "-I/mingw64/include");
        // cmd_append(&cmd, "-lglfw3");
        // cmd_append(&cmd, "-lopengl32");
        cmd_append(&cmd, "-lwinmm", "-lgdi32");
        //
        // NOTE: It can't be static since we don't compile mpv into a lib.a
        // cmd_append(&cmd, "-static");
        //
    } else {
        cmd_append(&cmd, "-lglfw", "-ldl");
    }


    if (!cmd_run_sync(cmd)) return_defer(false);

    nob_log(INFO, "OK: %s", __PRETTY_FUNCTION__);




#if !(defined(__MINGW64__) || defined(__MINGW32__))
    File_Paths hfiles = {0};
    // nob_read_entire_dir("./src/vendor/", &hfiles);
    // nob_read_entire_dir("./", &hfiles);
    read_entire_dir_filtered("./", &hfiles, true, nob_filter_by_extension, ".h");
    read_entire_dir_filtered("./src/vendor/", &hfiles, true, nob_filter_by_extension, ".h");
    read_entire_dir_filtered("./src/vendor/raylib/src/", &hfiles, true, nob_filter_by_extension, ".h");
    read_entire_dir_filtered("./src/include", &hfiles, true, nob_filter_by_extension, ".h");

    for (int i = 0; i < hfiles.count; ++i) {
        char* ext = strrchr(hfiles.items[i], '.');
        nob_log(INFO, "filtered file = `%s` with ext = %s", hfiles.items[i], ext);
    }

    String_Builder sb = {0};
    sb_append_cstr(&sb, "[\n");
    gen_compile_commands_json(&sb, cmd, hfiles);
    sb_append_cstr(&sb, "\n]\0");

    nob_log(INFO, "`gen_compile_commands_json`");

    // sb_append_null(&sb);
    if (write_entire_file("compile_commands.json", sb.items, sb.count)) {
        nob_log(INFO, "`compile_commands.json` generated with success");
    } else {
        nob_log(ERROR, "`compile_commands.json` could not be generated");
    }
#endif

defer:
    cmd_free(cmd);
    da_free(procs);
    return result;
}

bool build_clip(void) {
    build_raylib() || (exit(69), 0);
    Cmd cmd = {0};

    cmd_append(&cmd,
               "gcc", "./src/clipboard_original.c", "-c",
               "-oclipboard_original.o", "-lgdi32", "-lwinmm");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd,
               "gcc", "examples/raylib_clipboard_image.c",
               "clipboard_original.o", "-lraylib", "-lgdi32",
               "-lwinmm"
    );

    return cmd_run_sync(cmd);
}

bool build_examples(void) {
    Cmd cmd = {0};
    File_Paths c_files = {0};

    //
    // Before activating again do this somehow
    // nob_needs_rebuild
    //
    #if 0
    read_entire_dir_filtered(
        "./examples/raylib/text/", &c_files,
        true, nob_filter_by_extension, ".c"
    );
    read_entire_dir_filtered(
        "./examples/raylib/shapes/", &c_files,
        true, nob_filter_by_extension, ".c"
    );
    #endif

    read_entire_dir_filtered(
        "./examples", &c_files,
        true,
        // nob_filter_base_name_starts_with,
        filter_base_name_starts_with_raylib_and_ends_with_dot_c,
        "raylib_"
    );
    // da_append(&c_files, "./examples/rectangle_rounded_gradient.c");

    for (int idx = 0; idx < c_files.count; idx += 1) {
        const char *example = c_files.items[idx];
        char* cstr_example_bin = basename((char*)strdup(example));
        String_View example_sv = sv_from_cstr(example);
        char *ext = strrchr(cstr_example_bin, '.');
        if (ext) {
            *ext = '\0'; // Terminate the string at the '.' charact
        }
        String_View example_bin = sv_from_cstr(cstr_example_bin);
        const char *dst = tprintf("%s/"SV_Fmt, BUILD_DIR, SV_Arg(example_bin));
        const char *sources[] = {example, tprintf("-o%s", dst), NULL};
        if (needs_rebuild(dst, sources, ARRAY_LEN(sources) - 1/* dont count null terminator*/)) {
            bool ok = build_with_libs(sources);
            if (!ok) {
                nob_log(ERROR, "We errored out on example=%s dst=%s", example, dst);
                exit(1);
            }
        }
    }
    return true;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    // printf("nob_type=%s\n", nob_get_file_type("dashdjhasdash"));

    const char *program = shift_args(&argc, &argv);

    flags.libs.raylib = true;
    if (argc == 0) {
        flags.cyber_player = true;
    }
    #if defined (__MINGW64__)
            flags.mingw = true;
    #endif


    const char* arg = "";
    while (argc > 0) {
        arg = shift_args(&argc, &argv);
        String_View arg_sv = sv_from_cstr(arg);

        if(sv_eq(arg_sv, sv_from_cstr("mingw"))) {
            CC = "x86_64-w64-mingw32-gcc";
            flags.mingw = true;
        } if(sv_eq(arg_sv, sv_from_cstr("run"))) {
            flags.run = true;
        } else if(sv_eq(arg_sv, sv_from_cstr("clip"))) {
            flags.clip = true;
        } else if(sv_eq(arg_sv, sv_from_cstr("examples"))) {
            if (argc == 0) {
                flags.examples = true;
            } else {
                nob_log(ERROR, "Expected not more satuff");
                exit(1);
            }
            // run = true;
        } else if(sv_eq(arg_sv, sv_from_cstr("cc="))) {
            if (argc == 0) {
                nob_log(ERROR, "Expected compiler name");
                exit(1);
            } else {
                CC = shift_args(&argc, &argv);
                flags.examples = true;
            }

        }
    }

    const char *file = "";
    if (argc > 0) {
        file = shift_args(&argc, &argv);
    }

    const char *raylib = "";
    if (argc > 0) {
        file = shift_args(&argc, &argv);
    }

    nob_log(INFO, "Making directory for build `%s`", BUILD_DIR);
    if (!mkdir_if_not_exists(BUILD_DIR)) return false;

    if (!build_raylib()) return 1;

    if (flags.cyber_player) {

        flags.libs.mpv = true;
        {
            const char *sources[] = {"./src/cyber-player.c", "./src/progress.c", NULL};
            if (!build_with_libs(sources)) return 1;
        }
        flags.libs.mpv = false;
    }

    if (flags.examples) {
        build_examples();
    }

    if (flags.run) {
        Cmd cmd = {0};

#if (defined(__MINGW64__) || defined(__MINGW32__))
        const char *playfile =
            "\\Users\\Administrator\\Downloads\\cat_falls_ass_on_camera.mp4";
        cmd_append(&cmd, temp_sprintf("%s.exe", BUILD_DIR "/" BIN));
#else
        const char *playfile = "~/media/videos/life_is_everything.mp4";
        cmd_append(&cmd, BUILD_DIR "/" BIN);
#endif
        cmd_append(&cmd, playfile);
        if (!cmd_run_sync(cmd)) return -1;
    }

    return 0;
}
