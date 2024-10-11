# Start

```bash
    gcc nob.c -o nob && ./nob # Just compile once then you can just ./nob
```

# Random Odin package definition
Package, filename, folder structure Odin advice
A package is a directory of Odin code files (in the same folder), all of which have the same and unique package declaration package foo at the top of those files in that folder;
The package is only linked to the directory name (folder) and nothing else;
The package foo header (declaration at the top of Odin files) is only used by the ABI as a prefix for symbols in the final binary, it does not change the import name or path;
As stated in (1), the Odin files in the same package (i.e, same folder) must all have the same package foo header with the same name foo, but that name does NOT have to be the same as the last element of the package directory name in (2) above...although by convention, the package name is usually set the same as the last element in the import path;
The default name for the import name (in the calling code file) will be determined by the last element in the import path, n.b., a different import name can be used over the default package name, e.g., import "core:fmt" could be changed to import bar "core:fmt";
Sub-packages do not exist, so even nested directories are standalone packages and also do not infer dependence;
You cannot have cyclical imports - ever;
In Odin you should put as much in one package as possible and do not use packages for namespaces since it will not work (see using in the Overview);
Do not use packages to organize your files, rather use separate packages only to group reusable (stand alone) libraries;
To get some form of grouping of code, one can organize files within a package directory by their filenames (ala Java) and not sub-directories for all the reasons above; e.g., use snake case for the file names; e.g. math_app.odin, parse_app.odin, but these files are STILL within the same package directory/folder and all have the same package foo declaration so it is still a flat structure.


# MingW
Only works on mingw-gcc
    pacman -S mingw-w64-x86_64-SDL2
    pacman -S mingw-w64-x86_64-mpv
    pacman -S mingw-w64-x86_64-glfw
    pacman -S mingw-w64-x86_64-raylib

    pacman -Sy mingw-w64-x86_64-SDL2  mingw-w64-x86_64-mpv  mingw-w64-x86_64-glfw  mingw-w64-x86_64-raylib
