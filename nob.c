#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h>

#define NOB_IMPLEMENTATION
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

bool build_raylib() {
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_File_Paths object_files = {0};

    const char *build_path = RAYLIB_BUILD_DIR;

    if (!nob_mkdir_if_not_exists(build_path)) {
        nob_return_defer(false);
    }

    Nob_Procs procs = {0};

    for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path =
            nob_temp_sprintf("./vendor/raylib/src/%s.c", raylib_modules[i]);
        const char *output_path =
            nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
        output_path =
            nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        nob_da_append(&object_files, output_path);

        if (nob_needs_rebuild(output_path, &input_path, 1)) {
            cmd.count = 0;
            nob_cmd_append(&cmd, CC);
            nob_cmd_append(&cmd, /* "-ggdb", */ "-DPLATFORM_DESKTOP", "-fPIC",
                           "-DSUPPORT_FILEFORMAT_FLAC=1");
#if !defined(__MINGW64__)
            nob_cmd_append(&cmd, "-D_GLFW_X11");
#else
#endif

            nob_cmd_append(&cmd, "-I./vendor/raylib/src/external/glfw/include");

            nob_cmd_append(&cmd, "-c", input_path);
            nob_cmd_append(&cmd, "-o", output_path);

            Nob_Proc proc = nob_cmd_run_async(cmd);
            nob_da_append(&procs, proc);
        }
    }

    cmd.count = 0;

    if (!nob_procs_wait(procs)) nob_return_defer(false);

    nob_log(NOB_INFO, "OK: waited procs %s", __PRETTY_FUNCTION__);

    const char *libraylib_path = nob_temp_sprintf("%s/libraylib.a", build_path);

    if (nob_needs_rebuild(libraylib_path, object_files.items,
                          object_files.count)) {
        nob_cmd_append(&cmd, AR, "-crs", libraylib_path);
        for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path =
                nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
            nob_cmd_append(&cmd, input_path);
        }
        if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);
    }
    nob_log(NOB_INFO, "OK: create AR %s", __PRETTY_FUNCTION__);

defer:
    nob_cmd_free(cmd);
    nob_da_free(object_files);
    return result;
}

bool build_cyber_player(const char *sources[]) {
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_Procs procs = {0};
    cmd.count = 0;

    nob_cmd_append(&cmd, CC);
    
    // "sr.c", "progress.c";

    const char *source = NULL;
    for (int i = 0; source = sources[i]; ++i) {
        nob_log(NOB_INFO, "Appending Source `%s`\n", source);
        nob_cmd_append(&cmd, source,);
    }

    
    nob_cmd_append(&cmd, nob_temp_sprintf("-o%s/%s", BUILD_DIR, BIN));


    // OpenGL renderer is broken rn
    nob_cmd_append(&cmd, "-DSOFTWARE_RENDERER");
    nob_cmd_append(&cmd, "-D_REENTRANT");
    // nob_cmd_append(&cmd, "-O3");
    // nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");

    // "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include","-I/usr/include/sysprof-6","-lglib-2.0"
    nob_cmd_append(&cmd, "-I./vendor/raylib/src/", "-I./vendor/", "-pthread");
    nob_cmd_append(&cmd,  "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include" ,"-I/usr/include/sysprof-6", "-pthread","-lglib-2.0");



    nob_cmd_append(&cmd, "-L/mingw64/bin/", "-L/mingw64/lib/", "-lmpv", "-lSDL2", "-lm", "-L./",
                   "-l:./" RAYLIB_BUILD_DIR "/libraylib.a", "-lpthread");
#if defined(__MINGW64__)
    nob_cmd_append(&cmd, "-I/usr/include/SDL2", "-I/mingw64/include");

    // nob_cmd_append(&cmd, "-lglfw3");
    // nob_cmd_append(&cmd, "-lopengl32");
    nob_cmd_append(&cmd, "-lwinmm", "-lgdi32");
    // nob_cmd_append(&cmd, "-static");
#else
    nob_cmd_append(&cmd, "-lglfw", "-ldl");
#endif


    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

    nob_log(NOB_INFO, "OK: %s", __PRETTY_FUNCTION__);

defer:
    nob_cmd_free(cmd);
    nob_da_free(procs);
    return result;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);


    const char *program = nob_shift_args(&argc, &argv);

    const char* arg = "";
    bool run = false;
    while (argc > 0) {
        arg = nob_shift_args(&argc, &argv);
        Nob_String_View arg_sv = nob_sv_from_cstr(arg);
        if(nob_sv_eq(arg_sv, nob_sv_from_cstr("run"))) {
            run = true;
        } else if(nob_sv_eq(arg_sv, nob_sv_from_cstr("example"))) {
            if (argc > 0) {
                const char* example = nob_shift_args(&argc, &argv);
                const char* sources[] = {example, NULL};
                build_cyber_player(sources);

                const char *src = nob_temp_sprintf("%s/%s", BUILD_DIR, BIN);

                Nob_String_View example_sv = nob_sv_from_cstr(example);
                
                // malloc(example_sv.count);
                const char* cstr_example_bin = basename((char*)example);
                char *ext = strrchr(cstr_example_bin, '.');
                if (ext) {
                    *ext = '\0'; // Terminate the string at the '.' charact
                }
                Nob_String_View example_bin = nob_sv_from_cstr(cstr_example_bin);
                // example_bin.count -= 2;

                printf("my sv"SV_Fmt"\n", SV_Arg(example_sv));
                printf("my bin"SV_Fmt"\n", SV_Arg(example_bin));
                printf("my bin %s\n", cstr_example_bin);


                const char *dst = nob_temp_sprintf("%s/"SV_Fmt, BUILD_DIR, SV_Arg(example_bin));
                bool ok  = nob_copy_file(src, dst);
                NOB_ASSERT(ok);
            } else {
                nob_log(NOB_ERROR, "no example provided");
                exit(1);
            }
            run = true;
        }
    }

    const char *file = "";
    if (argc > 0) {
        file = nob_shift_args(&argc, &argv);
    }

    const char *raylib = "";
    if (argc > 0) {
        file = nob_shift_args(&argc, &argv);
    }

    nob_log(NOB_INFO, "Making directory for build `%s`", BUILD_DIR);
    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return false;

    
    if (!build_raylib()) return 1;

    const char *sources[] = {"sr.c", "progress.c", NULL};
    if (!build_cyber_player(sources)) return 1;

    if (run) {
        Nob_Cmd cmd = {0};

#if defined(__MINGW64__)
        // const char* playfile =
        // "/e/Torrents/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.mkv";
        const char *playfile =
            "\\Users\\Administrator\\Downloads\\cat_falls_ass_on_camera.mp4";
        nob_cmd_append(&cmd, nob_temp_sprintf("%s.exe", BUILD_DIR "/" BIN));
#else
        const char *playfile = "~/media/videos/life_is_everything.mp4";
        nob_cmd_append(&cmd, BUILD_DIR "/" BIN);
#endif
        nob_cmd_append(&cmd, playfile);

        if (!nob_cmd_run_sync(cmd)) return -1;
    }

    return 0;
}
