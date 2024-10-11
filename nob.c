#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define NOB_IMPLEMENTATION
#include "./nob.h"

#define BIN_DIR ".bin"
#define BIN "main"

bool build_cyber_player(void) {
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_Procs procs = {0};
    cmd.count = 0;


    nob_cmd_append(&cmd, "cc");
    nob_cmd_append(&cmd, "sr.c", "progress.c");

    nob_cmd_append(&cmd, nob_temp_sprintf("-o%s/%s", BIN_DIR, BIN));

    nob_cmd_append(&cmd, "-O3");

    // OpenGL renderer is broken rn
    nob_cmd_append(&cmd, "-DSOFTWARE_RENDERER");
    nob_cmd_append(&cmd, "-D_REENTRANT");
    // nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");

    nob_cmd_append(&cmd,  "-Ivendor", "-I/usr/include/glib-2.0", "-I/usr/lib/glib-2.0/include" ,"-I/usr/include/sysprof-6", "-pthread","-lglib-2.0");

#if defined(__MINGW64__)
    nob_cmd_append(&cmd, "-I/usr/include/SDL2", "-I/mingw64/include");
    nob_cmd_append(&cmd, "-lglfw3", "-lopengl32");
#else
    nob_cmd_append(&cmd, "-lglfw", "-ldl");
#endif

    nob_cmd_append(&cmd, "-lmpv", "-lSDL2", "-lm", "-lraylib", "-lpthread");

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

    const char *run = "";
    if (argc > 0) {
        run = nob_shift_args(&argc, &argv);
    }

    const char *file = "";
    if (argc > 0) {
        file = nob_shift_args(&argc, &argv);
    }

    nob_log(NOB_INFO, "Making directory for build `%s`", BIN_DIR);
    if (!nob_mkdir_if_not_exists(BIN_DIR)) return false;


    if (!build_cyber_player()) return 1;

    if (nob_sv_eq(nob_sv_from_cstr(run), nob_sv_from_cstr("run"))) {
        Nob_Cmd cmd = {0};

    #if defined(__MINGW64__)
        nob_cmd_append(&cmd, nob_temp_sprintf("%s.exe", BIN_DIR "/" BIN));
        nob_cmd_append(&cmd, "/e/Torrents/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.COMPLETE.REMUX.HDR.ENG.LATINO.FRENCH.ITALIAN.POLISH.JAPANESE.TrueHD.Atmos.7.1.H265-BEN.THE.MEN/Kingdom.Of.The.Planet.Of.The.Apes.2024.2160p.BluRay.mkv");
    #else 
        nob_cmd_append(&cmd, BIN_DIR "/" BIN);
        nob_cmd_append(&cmd, "~/media/videos/life_is_everything.mp4");
    #endif

        if (!nob_cmd_run_sync(cmd)) return -1;
    }


    return 0;
}
