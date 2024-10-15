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

// Gen without the []
void gen_compile_commands_json(String_Builder* sb, Cmd cmd) {

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
        int*items;
        size_t count;
        size_t capacity;
    } cfiles;


    for (int i = 0; i < cmd.count; ++i) {
        const char *comma = i == cmd.count - 1 ? "" : ",";
        char *ext = strrchr(cmd.items[i], '.');
        if (ext != NULL && strcmp(ext, ".c") == 0 ) {
            da_append(&cfiles, i);
        }

        sb_append_cstr(sb, "    \"");
        sb_append_cstr(sb, cmd.items[i]);
        sb_printf(sb, "\"%s\n", comma);
    }

    const char *comma = cfiles.count == 0 ? "" : ",";
    // TODO: Make a more robust sb_printf not using tpritnf ok?
    sb_printf(sb, "    ]%s\n", comma);
    for (size_t i = 0; i < cfiles.count; ++i) {
        const char *comma = i == cfiles.count - 1 ? "" : ",";
        // char cfile[PATH_MAX]; realpath(cmd.items[cfiles.items[i]], cfile);
        const char* cfile = cmd.items[cfiles.items[i]];
        sb_printf(sb, "    \"file\": \"%s\"", cfile);
        sb_printf(sb, "%s\n", comma);
    }
    sb_append_cstr(sb, "  }\n");



// "   \"cc\",\n"
// "   \"-c\",\n"
// "   \"-std=c99\",\n"
// "   \"-pedantic\",\n"
// "   \"-Wall\",\n"
// "   \"-Wno-unused-function\",\n"
// "   \"-Wno-deprecated-declarations\",\n"
// "   \"-Os\",\n"
// "   \"-I/usr/X11R6/include\",\n"
// "   \"-I/usr/include/freetype2\",\n"
// "   \"-D_DEFAULT_SOURCE\",\n"
// "   \"-D_BSD_SOURCE\",\n"
// "   \"-D_XOPEN_SOURCE=700L\",\n"
// "   \"-DVERSION=\\"6.4\"",\n"
// "   \"-DXINERAMA\",\n"
// "   \"util.c\"\n"



}

bool build_raylib() {
    bool result = true;
    Cmd cmd = {0};
    File_Paths object_files = {0};

    const char *build_path = RAYLIB_BUILD_DIR;

    if (!mkdir_if_not_exists(build_path)) {
        return_defer(false);
    }

    Procs procs = {0};

    for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path =
            temp_sprintf("./src/vendor/raylib/src/%s.c", raylib_modules[i]);
        const char *output_path =
            temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
        output_path =
            temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        da_append(&object_files, output_path);

        if (needs_rebuild(output_path, &input_path, 1)) {
            cmd.count = 0;
            cmd_append(&cmd, CC);
            cmd_append(&cmd, /* "-ggdb", */ "-DPLATFORM_DESKTOP", "-fPIC",
                           "-DSUPPORT_FILEFORMAT_FLAC=1");
#if !defined(__MINGW64__)
            cmd_append(&cmd, "-D_GLFW_X11");
#else
#endif

            cmd_append(&cmd, "-I./vendor/raylib/src/external/glfw/include");

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

    if (needs_rebuild(libraylib_path, object_files.items,
                          object_files.count)) {
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

    const char *source = NULL;
    for (int i = 0; (source = sources[i]); ++i) {
        nob_log(INFO, "Appending Source `%s`\n", source);
        cmd_append(&cmd, source,);
    }


    cmd_append(&cmd, temp_sprintf("-o%s/%s", BUILD_DIR, BIN));


    // OpenGL renderer is broken rn
    cmd_append(&cmd, "-DSOFTWARE_RENDERER");
    cmd_append(&cmd, "-D_REENTRANT");
    // cmd_append(&cmd, "-O3");
    // cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");

    // "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include","-I/usr/include/sysprof-6","-lglib-2.0"
    cmd_append(&cmd, "-I.", "-I./src/include/","-I./vendor/raylib/src/", "-I./src/vendor/", "-pthread");
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

    String_Builder sb = {0};
    sb_append_cstr(&sb, "[\n");
    nob_log(INFO, "`gen_compile_commands_json`");
    gen_compile_commands_json(&sb, cmd);
    sb_append_cstr(&sb, "\n]\0");
    // sb_append_null(&sb);

    if (write_entire_file("compile_commands.json", sb.items, sb.count)) {
        nob_log(INFO, "`compile_commands.json` generated with success");
    } else {
        nob_log(ERROR, "`compile_commands.json` could not be generated");
    }

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
