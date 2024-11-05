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


static struct {
    bool mingw32;
    bool mingw64;
    bool run;
    bool examples;
    bool cyber_player;
    struct {
        bool mpv;
        bool glib;
        bool raylib;
    } libs;
} flags = {0};

typedef enum {
    PLATFORM_UNSET = 0x0,
    PLATFORM_SDL3,
    PLATFORM_SDL2,
    PLATFORM_RGFW,
    PLATFORM_GLFW,
} Platform;

static struct {
    const char *CC, *AR, *OBJCOPY; // Must be gnu flag compliant for all of these binaries
    const char *libraylib_path; // libraylib.a
    Platform platform;
} config = 
#if defined(__MINGW32__) || defined(__MINGW64__)
{
    // Default
    .CC       = "gcc",
    .AR       = "ar",
    .OBJCOPY  = "objcopy",
    .libraylib_path = NULL,
    .platform = PLATFORM_SDL3
};
#else
{
    // Default
    .CC       = "gcc",
    .AR       = "ar",
    .OBJCOPY  = "objcopy",
    .libraylib_path = NULL,
    .platform = PLATFORM_SDL2
};
#endif



#define SDL3_VERSION "SDL3-3.1.6-x86_64-w64-mingw64-patched" // Fix SDL_GetClipboardData()
    // #define SDL3_VERSION "SDL3-3.1.3-x86_64-w64-mingw32"
    // #define SDL3_VERSION "SDL3-x86_64-w64-mingw32"
// #define PLATFORM_RGFW

#define RAYLIB_SOURCE_DIR "./src/vendor/raylib/src"


static const char *raylib_modules[] = {
    "rcore",   "raudio", "rmodels",
    "rshapes", "rtext",  "rtextures", "utils",
    "rglfw",
};


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

void gen_compile_commands_json(String_Builder* sb, Cmd cmd, File_Paths hfiles) {
#if 0
    char root_dir[PATH_MAX];
    realpath("./", root_dir);
#else
    const char *root_dir = nob_absolute_path("./");
#endif

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
            "SUPPORT_CLIPBOARD_IMAGE",
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


// Use objcopy to redefine conflicting symbols with raylib (--redefine-sym old=new)
bool sdl_redefine_symbol_in_lib(const char *static_lib_path) {
    if (!flags.mingw32) {
        assert(0 && "untested otherwise");
    }
    
    Cmd cmd = {0};
    cmd_append(&cmd, config.OBJCOPY);
    {
        const char *math_symbols[] = {"MatrixIdentity", "MatrixMultiply"};
        for (int idx = 0; idx < ARRAY_LEN(math_symbols); idx += 1) {
            cmd_append(&cmd, "--redefine-sym");
            cmd_append(&cmd,
                tprintf("%s=SDLmath_%s", math_symbols[idx], math_symbols[idx])
            );
        }
    }
    cmd_append(&cmd, static_lib_path);

    if(!cmd_run_sync(cmd)) {
        nob_log(ERROR, "Failed to redefined symbols at `%s`", static_lib_path);
        return false;
    }

    nob_log(INFO, "\nRedefined symbols at `%s` with success\n", static_lib_path);
    return true;
}

//
// Basically this command
// ar crsT libAB.a libA.a libB.a
// MSVC version but unimplemented
// lib.exe /OUT:libab.lib liba.lib libb.lib
//
void concat_static_libs(const char* out, const char** libs) {
    Cmd cmd = {0};
    cmd_append(&cmd, config.AR, "crsT", out);

    const char *lib = NULL;
    while ((lib = libs++[0])) {
        cmd_append(&cmd, lib);
    }

    if(cmd_run_sync(cmd)) {
        nob_log(INFO, "OK: %s", __PRETTY_FUNCTION__);
    }

}


void cmd_append_sdl(Cmd *cmd) {
    if (PLATFORM_SDL3 != config.platform && PLATFORM_SDL2 != config.platform) {
        nob_log(ERROR, "Tried to append SDL libs to a build that is not for SDL");
        exit(420);
    }

    switch (config.platform) {
        case PLATFORM_SDL3: {
            if (flags.mingw32) {
                const char *base_dir =  "src/vendor/SDL3/"SDL3_VERSION;

                // const char *base_dir =  "src/vendor/SDL3/SDL3-3.1.6-x86_64-w64-mingw32";
                cmd_append(cmd, 
                       tprintf("-I./%s/include/SDL3/", base_dir), // Make #include "SDL.h" possible
                       tprintf("-I./%s/include/",      base_dir), // Allow #include  "SLD3/SDL_something.h"
                       tprintf("-L./%s/lib/",          base_dir), // Where the static libs lie
                       #ifdef SDL_STATIC
                       "-l:libSDL3.a",
                       #else
                       "-l:libSDL3.dll.a",
                       #endif
                       "-lgdi32", "-lole32", "-lcfgmgr32",
                       "-limm32", "-loleaut32", "-lversion", "-lsetupapi",
                       "-lwinmm",
                       "-static"
                );
            } else {
                nob_log(INFO, "SDL3 if only supported on mingw rn");
                exit(89);
            }
            break;
        }
        case PLATFORM_SDL2: {
            nob_log(INFO, "SDL2 must be system provided, both lib and headers");
            cmd_append(cmd, "-I./src/vendor/SDL2/include/SDL2/", "-lSDL2", "-lSDL2Main");
            break;
        }
        default: {
            TODO("Better error msg here");
        }
    }
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
    {
        const char *raylib_platform_name = "";
        const char *tools_platform_name = "";
        if (flags.mingw32) {
            tools_platform_name = "mingw32";
        } else if (flags.mingw64) {
            tools_platform_name = "mingw64";
        } else {
            tools_platform_name = "linux";
        }

        switch (config.platform) {
            case PLATFORM_SDL3: raylib_platform_name = "SDL3"; break;
            case PLATFORM_SDL2: raylib_platform_name = "SDL2"; break;
            default: PANIC("Unhandled Raylib Platform");
        }

        build_path = tprintf("%s/%s/%s", BUILD_DIR, tools_platform_name,  raylib_platform_name);

        config.libraylib_path = strdup(tprintf("%s/libraylib.a", build_path));
    }



    if (!nob_mkdir_if_not_exists_recursive(build_path)) {
        return_defer(false);
    }


    //-----------------------------------------------------------------------//
    //-------------------------OBJFILES--------------------------------------//
    //-----------------------------------------------------------------------//

    Procs procs = {0};
    for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
        const char* module = raylib_modules[i];
        bool skip_rglfw = PLATFORM_GLFW != config.platform && 0 == strcmp(module, "rglfw");
        if (skip_rglfw) continue;

        const char *input_path  = tprintf("%s/%s.c", RAYLIB_SOURCE_DIR, module);
        const char *output_path = tprintf("%s/%s.o", build_path, module);

        da_append(&object_files, output_path);

        if (
            needs_rebuild(output_path, header_files.items, header_files.count)
            || needs_rebuild(output_path, c_files.items, c_files.count)
            || needs_rebuild1(output_path, input_path)
        ) {
            cmd.count = 0;
            cmd_append(&cmd, config.CC);

            // cmd_append(&cmd, "-mwindows"); // Entry Point: It changes the entry point of the application from main to WinMain, which is the standard entry point for Windows GUI applications.

            cmd_append(&cmd,
                   "-fPIC",
                   "-DSUPPORT_CLIPBOARD_IMAGE=1",
                   "-DSUPPORT_FILEFORMAT_BMP=1",
                   #ifdef DEBUG_RAYLIB
                       "-ggdb",
                       "-DRLGL_SHOW_GL_DETAILS_INFO=1"
                    #endif
                   "-DSUPPORT_FILEFORMAT_FLAC=1"
                   "-fmax-errors=5"
            );

            switch (config.platform) {
            case PLATFORM_SDL3: {
                // -Wl,--enable-auto-import
                // -Wl,--allow-shlib-undefined -Wl,--unresolved-symbols=ignore-all
                cmd_append(&cmd,
                    "-DPLATFORM_DESKTOP_SDL",
                    "-DSDL_ENABLE_OLD_NAMES" // Compatility Layer for easy of migration
                );
                cmd_append_sdl(&cmd); // Add flags and libs for sdl
                // cmd_append(&cmd, "-lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 " "-lversion -lsetupapi -lwinmm -luuid");
                // TODO: Compile SLD3 statically in mingw
                cmd_append(&cmd, "-static");
                break;
            }
            case PLATFORM_SDL2: {
                cmd_append(&cmd,
                    "-DPLATFORM_DESKTOP_SDL",
                );
                cmd_append_sdl(&cmd); // Add flags and libs for sdl
                // cmd_append(&cmd, "-lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 " "-lversion -lsetupapi -lwinmm -luuid");
                break;
            }
            case PLATFORM_RGFW: {
                TODO("NOOO");
                cmd_append(&cmd,
                    "-DPLATFORM_DESKTOP_RGFW",
                    "-lgdi32", "-lopengl32"
                );
                break;
            } case PLATFORM_GLFW: {
                TODO("NOOO");
                cmd_append(&cmd, tprintf("-I%s", RAYLIB_SOURCE_DIR"/external/glfw/include"));
                cmd_append(&cmd,
                    "-DPLATFORM_DESKTOP",
                );
                break;
            }
                default: PANIC("Unhandled Raylib Platform");
            }
            cmd_append(&cmd, "-c", input_path);
            cmd_append(&cmd, "-o", output_path);

            Proc proc = cmd_run_async(cmd);
            da_append(&procs, proc);
        }
    }


    if (!procs_wait(procs)) return_defer(false);
    nob_log(INFO, "OK: waited procs %s", __PRETTY_FUNCTION__);


    //-----------------------------------------------------------------------//
    //-------------------------(AR)CHIVING-----------------------------------//
    //-----------------------------------------------------------------------//


    const char *tmp_libraylib_path = config.libraylib_path;
    if (PLATFORM_SDL3 == config.platform) {
        tmp_libraylib_path = tprintf("%s/libtmpraylib.a", build_path);
        nob_remove_file_if_exists(tmp_libraylib_path);
    }

    cmd.count = 0;
    if (needs_rebuild(config.libraylib_path, object_files.items, object_files.count)) {
        cmd_append(&cmd, config.AR, "-crs", tmp_libraylib_path);
        for (size_t i = 0; i < ARRAY_LEN(raylib_modules); ++i) {
            const char* module = raylib_modules[i];
            bool skip_rglfw = PLATFORM_GLFW != config.platform && 0 == strcmp(module, "rglfw");
            if (skip_rglfw) continue;

            const char *input_path = tprintf("%s/%s.o", build_path, module);
            cmd_append(&cmd, input_path);
        }
        if (!cmd_run_sync(cmd)) return_defer(false);
    }

    // TODO: Add better caching for the achiving SDL3_STATIC
    if (PLATFORM_SDL3 == config.platform && needs_rebuild(config.libraylib_path, object_files.items, object_files.count)) {
        //
        // We  need remove coliding symbols (with raylib) from static SDL3
        // Then concat both static lib, raylib and SDL3 into a single libraylib.a
        //
        {
            // Preserve the original SDL lib
            const char *libsdl3_path = "./src/vendor/SDL3/" SDL3_VERSION "/lib/libSDL3.a";
            const char *libsdl3_patched_path = tprintf("%s/%s", build_path, "libSDL3.a");
            copy_file(libsdl3_path, libsdl3_patched_path);
            sdl_redefine_symbol_in_lib(libsdl3_patched_path);

            const char *libs[] = {tmp_libraylib_path, libsdl3_patched_path, NULL};
            concat_static_libs(config.libraylib_path, libs);
        }

        {   // if we're linking dynamically and we're on windows we need to do this
            const char *libsdl3_dll_path = "./src/vendor/SDL3/" SDL3_VERSION "/bin/SDL3.dll";
            if (!nob_copy_file(libsdl3_dll_path, BUILD_DIR"/SDL3.dll")) {
                nob_log(ERROR, "Failure copying from %s to %s", libsdl3_dll_path, BUILD_DIR"/SDL3.dll");
                exit(90);
            } else {
                nob_log(INFO, "Success copying from %s to %s", libsdl3_dll_path, BUILD_DIR);
            };
        }
        nob_log(INFO, "OK: create %s %s", config.AR, __PRETTY_FUNCTION__);
    } else {
        nob_log(INFO, "OK %s: %s is up to date", __PRETTY_FUNCTION__, tmp_libraylib_path);
    }


defer:
    cmd_free(cmd);
    da_free(object_files);
    return result;
}

#if 0
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


    #if defined(DEBUG)
        cmd_append(&cmd, "-O0", "-ggdb", "-Wextra" "-Wall",);
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
        cmd_append(&cmd, "-I"RAYLIB_SOURCE_DIR, "-I./src/vendor/", "-L./",  tprintf("-l:%s", libraylib_path), "-lpthread", "-lm");
    }

    #ifdef PLATFORM_SDL3
        cmd_append_sdl(&cmd);
    #elif defined(PLATFORM_RGFW)
        cmd_append(&cmd,
               "-lgdi32",
               "-lopengl32",
        );
    #endif

    if (flags.mingw32) {
        if (should_build_clipboard) {
            cmd_append(&cmd,"-DSUPPORT_CLIPBOARD_IMAGE=1");
            cmd_append(&cmd,"-L./", "-l:./"BUILD_DIR"/clipboard/libclipboard.a");
        }
        cmd_append(&cmd, "-L/mingw64/bin/", "-L/mingw64/lib/", "-I/mingw64/include");
        cmd_append(&cmd, "-lwinmm", "-lgdi32", "-luuid");
        cmd_append(&cmd, "-lopengl32");
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
#endif

bool build_examples(void) {
    Cmd cmd = {0};
    File_Paths c_files = {0};
    if (config.libraylib_path == NULL) {
    }

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

    Procs procs = {0};
    // da_append(&c_files, "./examples/rectangle_rounded_gradient.c");
    for (int idx = 0; idx < c_files.count; idx += 1) {
        cmd.count = 0;
        cmd_append(&cmd, config.CC);
        cmd_append(&cmd, "-fmax-errors=5");

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
            const char* source = NULL;
            for (int i = 0; (source = sources[i]); ++i) {
                cmd_append(&cmd, source);
            }
            // PLATFORM should probably be enum flags
            if (PLATFORM_SDL2 == config.platform || PLATFORM_SDL3 == config.platform) {
                cmd_append_sdl(&cmd);
            }
            cmd_append(&cmd, "-L./", tprintf("-l:%s", config.libraylib_path), "-lm");

            cmd_append(&cmd, "-I./src/include");            // gui.h (our own), our headers etc...
            cmd_append(&cmd, "-I./src/vendor/fuzzy-match"); // fuzzy-match
            cmd_append(&cmd, "-I./src/vendor/");            // raygui.h
            cmd_append(&cmd, "-I./src/vendor/raylib/src");  // raylib.h
            cmd_append(&cmd, "-I./");                       // nob.h

            Proc proc = cmd_run_async(cmd);
            da_append(&procs, proc);
        }
    }

    if (!procs_wait(procs)) return false;

    nob_log(INFO, "OK: waited procs %s", __PRETTY_FUNCTION__);

    return true;
}


int build() {
    nob_log(INFO, "Making directory for build `%s`", BUILD_DIR);
    if (!mkdir_if_not_exists(BUILD_DIR)) return false;

    // Default, always check and build
    if (!build_raylib()) return 1;

    if (flags.cyber_player) {
        TODO("flags.cyber_player");

        flags.libs.mpv = true;
        {
            const char *sources[] = {"./src/cyber-player.c", "./src/progress.c", NULL};
            // if (!build_with_libs(sources)) return 1;
        }
        flags.libs.mpv = false;
    }

    if (flags.examples) {
        build_examples();
    }

    if (flags.run) {
        Cmd cmd = {0};
        TODO("WAITING");

#if (defined(__MINGW64__) || defined(__MINGW32__))
        const char *playfile =
            "\\Users\\Administrator\\Downloads\\cat_falls_ass_on_camera.mp4";
        cmd_append(&cmd, tprintf("%s.exe", BUILD_DIR "/" BIN));
#else
        const char *playfile = "~/media/videos/life_is_everything.mp4";
        cmd_append(&cmd, BUILD_DIR "/" BIN);
#endif
        cmd_append(&cmd, playfile);
        if (!cmd_run_sync(cmd)) return -1;
    }

}

// TODO: clean option
int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program = shift_args(&argc, &argv);

    flags.libs.raylib = true;
    if (argc == 0) {
        // flags.cyber_player = true; // Not enable for now, we're in trasition
    }

    const char* arg = "";
    while (argc > 0) {
        arg = shift_args(&argc, &argv);
        String_View arg_sv = sv_from_cstr(arg);

        if (sv_eq_cstr(arg_sv, "mingw")) {
            config.CC       = "x86_64-w64-mingw32-gcc";
            config.AR       = "x86_64-w64-mingw32-ar";
            config.OBJCOPY  = "x86_64-w64-mingw32-objcopy";
            config.platform = PLATFORM_SDL3;
            flags.mingw32   = true;
        } if (sv_eq_cstr(arg_sv, "run")) {
            flags.run = true;
        } else if (sv_eq_cstr(arg_sv, "clip")) {
            TODO("NO CLIP");
        } else if (sv_eq_cstr(arg_sv, "examples")) {
            flags.examples = true;
        } else if (sv_eq_cstr(arg_sv, "CC")) {
            if (argc == 0) {
                nob_log(ERROR, "Expected compiler name after (e.g. CC clang)");
                exit(1);
            } else {
                config.CC = shift_args(&argc, &argv);
                flags.examples = true;
            }
        }
    }

    return build();
}

// x86_64-w64-mingw32-gcc ./examples/raylib_circle.c -o.build/raylib_circle -fmax-errors=5 -DSOFTWARE_RENDERER -D_REENTRANT -DDEBUG -I. -I./src/include/ -pthread -I./src/vendor/raylib/src -I./src/vendor/ -L./ -l:.build/raylib/mingw/libraylib.a -l:./.build/raylib/mingw/libraylib.a -l:./src/vendor/SDL3/SDL3-x86_64-w64-mingw32/lib/libSDL3.a -lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 -lversion -lsetupapi -lwinmm -luuid
// x86_64-w64-mingw32-gcc ./examples/raylib_circle.c -o.build/raylib_circle -fmax-errors=5 -DSOFTWARE_RENDERER -D_REENTRANT -DDEBUG -I. -I./src/include/ -pthread -I./src/vendor/raylib/src -I./src/vendor/ -L./ -l:./.build/raylib/mingw/libraylib.a  -lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 -lversion -luuid -lsetupapi -lwinmm -l:./src/vendor/SDL3/SDL3-x86_64-w64-mingw32/lib/libSDL3.dll.a
// /bin/x86_64-w64-mingw32-gcc ./examples/raylib_clipboard_image.c -I ./src/vendor/raylib/src -L./ -l:./concat.a -lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 -lversion -lsetupapi -lwinmm -Wl,--allow-shlib-undefined -luuid
// x86_64-w64-mingw32-objcopy --redefine-sym MatrixMultiply=SDLmath_MatrixMultiply concat.a
// ./examples/raylib_clipboard_image.c -I ./src/vendor/raylib/src -L./ -l:./concat.a -lgdi32 -lole32 -lcfgmgr32 -limm32 -loleaut32 -lversion -lsetupapi -lwinmm -Wl,--allow-shlib-undefined -luuid
// SDL3 required libs on windows kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 setupapi shell32)
// mingw-w64-clang-i686-cc mingw-w64-clang-i686-cmake mingw-w64-clang-i686-ninja  mingw-w64-clang-i686-pkg-config mingw-w64-clang-i686-clang-tools-extra
// mingw-w64-x86_64-cc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja  mingw-w64-x86_64-pkg-config mingw-w64-x86_64-tools-extra
// pacman -S mingw-w64-x86_64-pkg-config

