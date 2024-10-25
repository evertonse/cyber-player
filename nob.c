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
#define BIN "main"


#if defined(__MINGW64__)
#   define CC "x86_64-w64-mingw32-gcc"
#else
#   define CC "gcc"
#endif

// #define AR     "x86_64-w64-mingw32-ar"
#define AR "ar"

#define RAYLIB_BUILD_DIR BUILD_DIR "/raylib"
#define RAYLIB_SOURCE_DIR "./src/vendor/raylib/src"


static const char *raylib_modules[] = {
    "rcore",   "raudio", "rglfw",     "rmodels",
    "rshapes", "rtext",  "rtextures", "utils",
};


#if defined(_WIN32) && defined(__MINGW64__)
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
        sb_printf(sb, "%s", "cc");
        sb_printf(sb, "\"%s\n", comma);

        CStr_Array extra_defines = {0};
        const char *extra_defines_cstrs[] = {
            "GUI_IMPLEMENTATION", "NOB_STRIP_PREFIX", "NOB_IMPLEMENTATION",
            "RAYGUI_IMPLEMENTATION", "RAYGUI_MO_IMPLEMENTATION",

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

bool build_raylib() {
    bool result = true;
    Cmd cmd = {0};
    File_Paths object_files = {0};
    File_Paths header_files = {0};
    File_Paths c_files = {0};

    read_entire_dir_filtered(RAYLIB_SOURCE_DIR, &header_files, true, nob_filter_by_extension, ".h");
    read_entire_dir_filtered(RAYLIB_SOURCE_DIR, &c_files, true, nob_filter_by_extension, ".c");

    const char *build_path = RAYLIB_BUILD_DIR;

    if (!mkdir_if_not_exists(build_path)) {
        return_defer(false);
    }

    Procs procs = {0};

    for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path =
            temp_sprintf(RAYLIB_SOURCE_DIR"/%s.c", raylib_modules[i]);
        const char *output_path =
            temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
        output_path =
            temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        da_append(&object_files, output_path);

        if (
            needs_rebuild(output_path, header_files.items, header_files.count)
            || needs_rebuild(output_path, c_files.items, c_files.count)
            || needs_rebuild(output_path, &input_path, 1)
        ) {
            cmd.count = 0;
            cmd_append(&cmd, CC);
            cmd_append(&cmd, /* "-ggdb", */ "-DPLATFORM_DESKTOP", "-fPIC",
                           "-DSUPPORT_FILEFORMAT_FLAC=1");
#if !defined(__MINGW64__)
            cmd_append(&cmd, "-D_GLFW_X11");
#else
#endif

            cmd_append(&cmd, "-I"RAYLIB_SOURCE_DIR"external/glfw/include");

            cmd_append(&cmd, "-c", input_path);
            cmd_append(&cmd, "-o", output_path);

            Proc proc = cmd_run_async(cmd);
            da_append(&procs, proc);
        }
    }

    cmd.count = 0;

    if (!procs_wait(procs)) return_defer(false);

    nob_log(INFO, "OK: waited procs %s", __PRETTY_FUNCTION__);

    const char *libraylib_path = temp_sprintf("%s/libraylib.a", build_path);

    if (needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        cmd_append(&cmd, AR, "-crs", libraylib_path);
        for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path =
                temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
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

bool build_cyber_player(const char *sources[]) {
    bool result = true;
    Cmd cmd = {0};
    Procs procs = {0};
    cmd.count = 0;

    cmd_append(&cmd, CC);

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
    #elseif  defined(RELEASE)
        cmd_append(&cmd, "-O3");
    #endif

    // "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include","-I/usr/include/sysprof-6","-lglib-2.0"
    cmd_append(&cmd, "-I.", "-I./src/include/", "-I" RAYLIB_SOURCE_DIR, "-I./src/vendor/", "-pthread");
    cmd_append(&cmd,  "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include" ,"-I/usr/include/sysprof-6", "-pthread","-lglib-2.0");



    cmd_append(&cmd, "-lmpv", "-lSDL2", "-lm", "-L./",
                   "-l:./" RAYLIB_BUILD_DIR "/libraylib.a", "-lpthread");

#if defined(__MINGW64__)
    cmd_append(&cmd, "-L/mingw64/bin/", "-L/mingw64/lib/", "-I/usr/include/SDL2", "-I/mingw64/include");
    // cmd_append(&cmd, "-lglfw3");
    // cmd_append(&cmd, "-lopengl32");
    cmd_append(&cmd, "-lwinmm", "-lgdi32");
    //
    // NOTE: It can't be static since we don't compile mpv into a lib.a
    // cmd_append(&cmd, "-static");
    //
#else
    cmd_append(&cmd, "-lglfw", "-ldl");
#endif


    if (!cmd_run_sync(cmd)) return_defer(false);

    nob_log(INFO, "OK: %s", __PRETTY_FUNCTION__);




#if !defined(__MINGW64__)
    File_Paths hfiles = {0};
    // nob_read_entire_dir("./src/vendor/", &hfiles);
    // nob_read_entire_dir("./", &hfiles);
    read_entire_dir_filtered("./", &hfiles, true, nob_filter_by_extension, ".h");
    read_entire_dir_filtered("./src/vendor/", &hfiles, true, nob_filter_by_extension, ".h");
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


int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);


    const char *program = shift_args(&argc, &argv);

    const char* arg = "";
    bool run = false;
    while (argc > 0) {
        arg = shift_args(&argc, &argv);
        String_View arg_sv = sv_from_cstr(arg);
        if(sv_eq(arg_sv, sv_from_cstr("run"))) {
            run = true;
        } else if(sv_eq(arg_sv, sv_from_cstr("example"))) {
            if (argc > 0) {
                const char* example = shift_args(&argc, &argv);
                const char* sources[] = {example, NULL};
                build_cyber_player(sources);

                const char *src = tprintf("%s/%s", BUILD_DIR, BIN);

                String_View example_sv = sv_from_cstr(example);

                //
                // We could, but fuckit
                // malloc(example_sv.count);
                // memset
                //
                char* cstr_example_bin = basename((char*)example);
                char *ext = strrchr(cstr_example_bin, '.');
                if (ext) {
                    *ext = '\0'; // Terminate the string at the '.' charact
                }
                String_View example_bin = sv_from_cstr(cstr_example_bin);
                // example_bin.count -= 2;

                const char *dst = tprintf("%s/"SV_Fmt, BUILD_DIR, SV_Arg(example_bin));
                bool ok  = copy_file(src, dst);
                NOB_ASSERT(ok);
            } else {
                nob_log(ERROR, "no example provided");
                exit(1);
            }
            run = true;
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

    const char *test[] = {"./test.c", "-otest",  NULL};
    if (!build_cyber_player(test)) return 1;

    const char *sources[] = {"./src/cyber-player.c", "./src/progress.c", NULL};
    if (!build_cyber_player(sources)) return 1;


    if (run) {
        Cmd cmd = {0};

#if defined(__MINGW64__)
        // const char* playfile =
        // "/e/Torrents/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.mkv";
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
