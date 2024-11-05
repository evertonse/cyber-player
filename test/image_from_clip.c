#include "raylib.h"
#include <windows.h>

// Function to convert Windows HBITMAP to raylib Image
Image GetImageFromHBITMAP(HBITMAP hBitmap) {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);
    
    // Get image data
    HDC hdcMem = CreateCompatibleDC(NULL);
    SelectObject(hdcMem, hBitmap);
    
    // Create buffer for raw image data
    int size = bmp.bmWidth * bmp.bmHeight * 4; // 4 bytes per pixel (RGBA)
    unsigned char* buffer = (unsigned char*)malloc(size);
    
    // Setup bitmap info
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight; // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // Get the actual bitmap data
    GetDIBits(hdcMem, hBitmap, 0, bmp.bmHeight, buffer,
              &bmi, DIB_RGB_COLORS);
    
    // Convert BGR to RGB and add alpha channel
    for(int i = 0; i < size; i += 4) {
        unsigned char temp = buffer[i];
        buffer[i] = buffer[i + 2];
        buffer[i + 2] = temp;
        buffer[i + 3] = 255;  // Set alpha to 255
    }
    
    // Create raylib Image
    Image image = {
        .data = buffer,
        .width = bmp.bmWidth,
        .height = bmp.bmHeight,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    
    // Cleanup
    DeleteDC(hdcMem);
    
    return image;
}

Image GetClipboardImage(void) {
    Image result = {0};
    
    // Open clipboard
    if (!OpenClipboard(NULL)) {
        return result;
    }
    
    // Check if clipboard contains a bitmap
    HANDLE hClipboard = GetClipboardData(CF_BITMAP);
    if (hClipboard) {
        // Convert clipboard HBITMAP to raylib Image
        result = GetImageFromHBITMAP((HBITMAP)hClipboard);
    }
    
    // Close clipboard
    CloseClipboard();
    
    return result;
}

// Example usage
void ExampleUsage(void) {
    // Get image from clipboard
    Image clipboardImage = GetClipboardImage();
    
    if (clipboardImage.data) {
        // Convert to texture if you want to draw it
        Texture2D texture = LoadTextureFromImage(clipboardImage);
        
        // Draw the texture
        DrawTexture(texture, 0, 0, WHITE);
        
        // Cleanup when done
        UnloadTexture(texture);
        UnloadImage(clipboardImage);
    }
}
