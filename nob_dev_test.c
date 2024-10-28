#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"



void test_new_funtions() {
    #ifdef _WIN32
    const char *test_paths[] = {
        "file.txt",
        "path\\to\\file.tar.gz",
        ".hidden",
        ".hidden.txt",
        ".hidden.txt.tar",
        "no_extension",
        "multiple.dots.in.name",
        ".\\current\\dir\\file.h",
        ".",
        "..",
        "",
        "folder\\",
        "C:\\folder\\file.txt",
        "C:\\folder\\file.txt.zip",

    };
    #else
    const char *test_paths[] = {
        "file.txt",                   // -> "file"
        "path/to/file.tar.gz",        // -> "file.tar" (last=false), "file" (last=true)
        ".hidden",                    // -> ".hidden"
        ".hidden.txt",                // -> ".hidden"
        ".hidden.txt.tar",            // -> ".hidden.txt" (last=false), ".hidden" (last=true)
        "no_extension",               // -> "no_extension"
        "multiple.dots.in.name",      // -> "multiple" (last=false), "multiple.dots.in" (last=true)
        "./current/dir/file.h",       // -> "file"
        ".",                          // -> "."
        "..",                         // -> ".."
        "",                           // -> ""
        "folder/",                    // -> "folder"
        "/home/user/porn.tar",       // -> "file"
        NULL
    };
    #endif

    printf("Testing name_without_ext:\n");
    printf("%-30s | %-20s | %-20s\n", "Path", "First Ext", "Last Ext");
    printf("-------------------------------+----------------------+----------------------\n");

    for (int i = 0; test_paths[i] != NULL; i++) {
        printf("%-30s | %-20s | %-20s\n",
               test_paths[i],
               nob_path_without_ext(test_paths[i], false),
               nob_path_without_ext(test_paths[i], true));
    }

    printf("\n-------------------------------+----------------------+----------------------\n");

    printf("%-30s | %-20s | %-20s\n", "Path", "Dir Of", "Base Name");
    printf("-------------------------------+----------------------+----------------------\n");
    for (int i = 0; test_paths[i] != NULL; i++) {
        printf("%-30s | %-20s | %-20s\n",
               test_paths[i],
               nob_dir_of(test_paths[i]),
               nob_base_name(test_paths[i]));
    }

    printf("-------------------------------+----------------------+----------------------\n");
    printf("%-30s | %-20s | %-20s\n", "Path", "RealPath of /home", "RealPath");
    printf("%-30s | %-20s | %-20s\n",
           __FILE__,
           nob_absolute_path("/home"),
           nob_absolute_path(nob_base_name(__FILE__))
    );
}
void test_filter(void) {

    File_Paths c_files = {0};

    read_entire_dir_filtered(
        "./examples/raylib/text/", &c_files,
        true, nob_filter_by_extension, ".c"
    );
    read_entire_dir_filtered(
        "./examples/raylib/shapes/", &c_files,
        true, nob_filter_by_extension, ".c"
    );

    read_entire_dir_filtered(
        "./examples", &c_files,
        true, nob_filter_base_name_starts_with,
        "raylib_"
    );
    for (int idx = 0; idx < c_files.count; idx += 1) {
        printf("c_files.items[idx]=%s\n", c_files.items[idx]);
        
    }
}


int main() {
    test_new_funtions();
    test_filter();
    return 0;
}
