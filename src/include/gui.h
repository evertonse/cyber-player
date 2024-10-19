#ifndef RAYGUI_MO_H
#define RAYGUI_MO_H

#include <math.h>
#include <stdint.h>
#include <float.h>
#include "raymath.h"
#include "raylib.h"


#define NOB_STRIP_PREFIX
#include "nob.h"

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

// int GuiMoListView(Rectangle bounds, const char **text, int count, Vector2 *scrollIndex, int *active, int *focus);


#if defined(__cplusplus)
}            // Prevents name mangling of functions
#endif

#endif // RAYGUI_MO_H


#if defined(GUI_IMPLEMENTATION)

// #define USE_CUSTOM_LISTVIEW_FILEINFO
#define GUI_FILE_DIALOG_IMPLEMENTATION
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 25
#include "raygui_custom_dialog.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>               // Required for: roundf() [GuiColorPicker()]


// return if it's done
bool GuiMoPopup(Rectangle bounds, const char *message, double *time) {
    if (time == NULL) {
        time = malloc(1*sizeof(time));
    }
    if (*time > 10000.0)  {
        *time = 0;
        return 0;
    }
    *time += GetFrameTime();
    GuiMessageBox(bounds, "popup", message, "");
    // GuiDrawText(const char *text, Rectangle textBounds, int alignment, Color tint)
}

// Maybe considere SDF
// https://www.raylib.com/examples/text/loader.html?name=text_font_sdf
int GuiMoListView(Rectangle bounds, const char **text, int count, Vector2 *scrollPercentage, int *active, int *focus, bool react)
{
    int ret = 0;
    assert(bounds.height > 0 && bounds.width > 0 && "check if this is the correct behaviour");

    Vector2 mouse_position = GetMousePosition();
    Vector2 mouse_wheel = GetMouseWheelMoveV();

    float whell_speed = 0.02;
    if (IsKeyDown(KEY_LEFT_CONTROL)){
        whell_speed = 0.8;
    }
    // TODO: Fix to visible items stuff
    assert(scrollPercentage);
    scrollPercentage->y -= ((mouse_wheel.y*whell_speed)/bounds.y);
    scrollPercentage->y  = Clamp(scrollPercentage->y, 0, 1.0);

    scrollPercentage->x -= ((mouse_wheel.x*whell_speed)/bounds.x);
    scrollPercentage->x  = Clamp(scrollPercentage->x, 0, 1.0);



    const int horizontal_spacing = -1;
    const int vertical_spacing   = -4;

    static bool font_loaded = false;

    static Font  font;
    const  float font_size   = 24;
    static float item_height = 0;


    bool useVerticalScrollBar = false;
    bool useHorizontalScrollBar = false;

    if (font_loaded == false) {
        font_loaded = true;

        font = LoadFontEx("./assets/fonts/Alegreya-Regular.ttf", font_size, NULL, 0);
        
        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
        item_height = vertical_spacing + MeasureTextEx(font, "_", font_size, horizontal_spacing).y;
        assert(
            MeasureTextEx(font, "_", font_size, horizontal_spacing).y == font_size
            && "Expected to measure the same thing but might not be because of Line spacing which might be set with SetTextLineSpacing()"
        );
    }
    // const Font font = jetbrains;
    Color fontColor         = GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_NORMAL));
    Color fontColorFocused = GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_FOCUSED));
    Color fontColorPressed = GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_PRESSED));
    // font_color = MAROON;

    // TODO: Move this calculation to the caller?
    float biggestTextWidth = 0;
    float smallestTextWidth = FLT_MAX;
    for (int i = 0; i < count; ++i) {
        float width = MeasureTextEx(font, text[i], font_size, horizontal_spacing).x;
        biggestTextWidth  = Max(biggestTextWidth, width);
        smallestTextWidth = Min(smallestTextWidth, width);
    }



    static const Color transparent =  BLANK;

    const int   borderWidth        = GuiGetStyle(DEFAULT, BORDER_WIDTH);
    const Color borderColor        = GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_NORMAL));
    const Color borderColorFocused = GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_FOCUSED));
    const Color borderColorPressed = GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_PRESSED));

    const Color item_focused_color = GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_FOCUSED));

    const Color background        = GetColor(GuiGetStyle(DEFAULT,  BACKGROUND_COLOR));
    // Color background        = GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_NORMAL));
    const Color backgroundFocused = GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_FOCUSED));
    const Color backgroundPressed = GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_PRESSED));

    const int scrollBarWidth = 12;
    GuiDrawRectangle(bounds, borderWidth, borderColor, background);


    Rectangle scrollBarBounds = {
        bounds.width + bounds.x - scrollBarWidth - borderWidth,
        bounds.y + borderWidth,
        scrollBarWidth,
        bounds.height - 2 * borderWidth
    };

    Rectangle scrollBarBoundsHorz = {
        bounds.x + borderWidth,
        bounds.y + bounds.height - borderWidth - scrollBarWidth,
        bounds.width - 2 * borderWidth - scrollBarWidth,
        scrollBarWidth
    };


    // left, top , right, bottom
    const Vector4 padding = {0.0 + borderWidth, 2.0 + borderWidth, 0.0 + borderWidth, 2.0 + borderWidth};
    Rectangle boundsWithPadding = {
        bounds.x + padding.x,
        bounds.y + padding.y,
        bounds.width - (padding.z + padding.x) - scrollBarWidth,
        bounds.height - (padding.w + padding.y) - scrollBarWidth
    };
    bounds = boundsWithPadding;


    Rectangle virtualBounds  = {
        bounds.x, bounds.y,
        biggestTextWidth,
        count*item_height,
    };


    if ((virtualBounds.height - bounds.height) > 0) {
        useVerticalScrollBar = true;
    } else {
        bounds.width += scrollBarWidth;
    }

    if ((virtualBounds.width - bounds.width) > 0) {
        useHorizontalScrollBar = true;
    } else {
        bounds.height += scrollBarWidth;
    }

    if (useVerticalScrollBar) {
        virtualBounds.y -= Remap(scrollPercentage->y, 0, 1.0, 0, ((float)(virtualBounds.height - bounds.height)));
    }

    const float textPaddingLeft = 3;
    if (useHorizontalScrollBar) {
        virtualBounds.x -= Remap(scrollPercentage->x, 0, 1.0, 0, ((float)((virtualBounds.width + textPaddingLeft) - bounds.width)));
    }

    BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);




    // We might need to add one because we might be
    // seeing half from above and another half from below
    // Also remember spacing
    int visibleItemsCount = (int) Min(
        // We take the floor because we wanna make
        // sure that we count ONLY FULLY visible items
        // If we see half of up or half down we do not count that
        // that is considering correctly paned scroll, that is, default 
        // position (when scrollPercentage.y is 0.0)
        ceil(bounds.height/item_height) + 1.0,
        (float)count
    );



    int startIndex = Remap(
        Remap(scrollPercentage->y, 0, 1.0, 0, virtualBounds.height - bounds.height - item_height),
        0, virtualBounds.height,
        0, count-1
    );
    startIndex = (int)Clamp(startIndex, 0, count-1);

    int endIndex = startIndex + visibleItemsCount + 1;
    endIndex = (int)Clamp(endIndex, 0, count-1);



    bool foundActive = false;

    // for (int i = 0; i <= count-1; ++i) {
    for (int i = startIndex; i <= endIndex; ++i) {
        const char *file_path = text[i];

        Vector2 text_pos = {
            virtualBounds.x + textPaddingLeft,
            virtualBounds.y + (i * item_height)
        };

        // Vector2 text_size = MeasureTextEx(font, file_path, font_size, horizontal_spacing);
        // Rectangle text_rect = {text_pos.x, text_pos.y, text_size.x, text_size.y};
        // Rectangle itemBounds = {bounds.x, text_pos.y, bounds.width, item_height};
        Rectangle itemBounds = {virtualBounds.x, text_pos.y, virtualBounds.width+textPaddingLeft-borderWidth, item_height};
        

        Color usedTextColor = fontColor;
        Color usedBackgroundColor = transparent;
        Color usedBorderColor = transparent;
        if ((foundActive == false)
            && CheckCollisionPointRec(mouse_position, bounds)
            && CheckCollisionPointRec(mouse_position, itemBounds)
        ) {
            foundActive = true;

            usedTextColor       = fontColorFocused;
            usedBackgroundColor = backgroundFocused;
            usedBorderColor     = borderColorFocused;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                *active = i;
                usedTextColor       = fontColorPressed;
                usedBackgroundColor = backgroundPressed;
                usedBorderColor     = borderColorPressed;
            }
        } else {

            if (i == *active) {
                usedTextColor       = fontColorFocused;
                usedBackgroundColor = backgroundFocused;
                usedBorderColor     = borderColorFocused;
            }
        }

        GuiDrawRectangle(itemBounds, borderWidth, usedBorderColor, usedBackgroundColor);
        DrawTextEx(font, file_path, text_pos, font_size, horizontal_spacing, usedTextColor);
    }

    EndScissorMode();

    const int scrollScale = INT32_MAX/2;
    // const int scrollScale = count-visibleItemsCount;
    // const int scrollScale = count;
    // const int scrollScale = bounds.height;
    // const int scrollScale = scrollBarBounds.height;
    int scrollResult = 0;
    if (useVerticalScrollBar) {
        scrollResult = GuiScrollBar(scrollBarBounds, scrollPercentage->y * (float)scrollScale, 0, scrollScale);
        if (mouse_wheel.y == 0) {
            scrollPercentage->y = (float)scrollResult/scrollScale;
        }
    } else {
        scrollBarBoundsHorz.width += scrollBarWidth;
    }

    if (useHorizontalScrollBar) {
        scrollResult = GuiScrollBar(scrollBarBoundsHorz, scrollPercentage->x * (float)scrollScale, 0, scrollScale);
        if (mouse_wheel.y == 0) {
            scrollPercentage->x = (float)scrollResult/scrollScale;
        }
    }

    #ifdef DEBUG
    DrawTextEx(
        GetFontDefault(),
        tprintf(
                "bounds = {%.1f, %.1f, %.1f, %.1f}\n"
                "visibleItemsCount = %d \n"
                "scrollPercentage = {%.1f, %.1f}\n "
                "mouse_wheel = %f : %f \n"
                "startIndex = %d\n"
                "endIndex = %d\n"
                "found_active = %d\n"
                "item_height = %f\n"
                "scrollResult = %d\n"
                "biggestTextWidth = %.1f\n"
                "smallestTextWidth = %.1f\n"
                "text[count-1] = %s\n"
                "ceil(bounds.height/item_height) = %f\n",
                bounds.x, bounds.y, bounds.width, bounds.height,
                visibleItemsCount,
                mouse_wheel.x, mouse_wheel.y,
                scrollPercentage->x, scrollPercentage->y,
                startIndex,
                endIndex,
                foundActive,
                item_height,
                scrollResult,
                biggestTextWidth,
                smallestTextWidth,
                text[count-1],
                ceilf(bounds.height/item_height)
        ),
        (CLITERAL(Vector2){280, 100}), GetFontDefault().baseSize*2.5, 0, WHITE);
    #endif

    return ret;
}

#endif      // GUI_IMPLEMENTATION