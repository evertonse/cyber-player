## [rcore] Clipboard Image 

### Overview
I've implemented a small start for Clipboard Image functionality for raylib. The implementation is only for desktop Windows OS and it's only `GetClipboardImage`. I want to implement `SetClipboardImage`, but I need to know a few things first regarding whether my approach is proper for the project.



https://github.com/user-attachments/assets/6e9a4864-39df-4b8b-adc7-1c970fb093cd


You'll see I created `rcore_clipboard_win32` that implements this from scracth using winapi. 
We can question "_Why not use already implemented clipboard functions from platform abstractions such as GLFW, SDL, and RGFW and adpated to our needs?_" Because they either did not implement it, or it is only available on the bleeding edge (SDL3).

- From what I've seen, SDL has clipboard image support but only since SDL 3.1.3 [SDL_clipboard.h from SDL3](https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_clipboard.h#L240C1-L240C54). In that case, I check for the major and minor version and implement using SDL functions; otherwise, it just returns `NULL` (I could add a warning or implement using `rcore_clipboard_win32`). Also, SDL3 has a migration section, so I'm not even sure that the current raylib SDL platform is suited for SDL3. Just to double-check, there's no `SDL_GetClipboardData` in SDL2 [SDL_clipboard.h from SDL2](https://github.com/libsdl-org/SDL/blob/SDL2/include/SDL_clipboard.h).

- RGFW only supports clipboard Unicode Text from what I could see: [RGFW.h - Line 643](https://github.com/ColleagueRiley/RGFW/blob/main/RGFW.h#L6437).

- GLFW is still in the process of implementing clipboard image functionality: [GLFW Pull Request #2385](https://github.com/glfw/glfw/pull/2385/files).

### How it is structured
In this PR, works like this: if we detect that the platform is either GLFW or RGFW and we're on Windows, then we include `rcore_clipboard_win32.c`. The file `rcore_clipboard_win32.c` went the route of defining `windgi.h` structs that are needed, taking care of any name collisions.

### Takeaway
What I'm thinking is that when clipboard image manipulation becomes a stable feature for GLFW, we can easily add it to `rcore_desktop_glfw`. But for platforms that don't support it (such as SDL2, RGFW, and current GLFW) we could have "utils" platform functions such as `GetClipboardImage` (next would be `SetClipboardImage`) implemented seperatly such as in this PR.

## Concerns
Another thing to add is that now it couples `rcore` and `rtexture` by the `Image` struct and the `LoadImageFromMemory` function. Furthermore, it is required to have `SUPPORT_FILEFORMAT_BMP` enabled.

Finally, the whole implementation is behind the flag `SUPPORT_CLIPBOARD_IMAGE`, which in this branch I've enabled by default in `config.h`.

I've added `core_clipboard_image` example to make it easy to check it out (it doesn't have to be permanent, also it hasn't updated its related files besides the examples `Makefile`)

This is kind of a draft. Sorry for the wall of text, I just tought it was important to share this to aid your decision and allow further development.

--- // https://github.com/libsdl-org/SDL/pull/11409
