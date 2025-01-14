// original: https://stackoverflow.com/questions/30552255/how-to-read-a-bitmap-from-the-windows-clipboard
// https://en.wikipedia.org/wiki/BMP_file_format
// https://learn.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclipboarddata
// https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-operations


#if defined (_WIN32)
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#if DEMO_SFML
#include <SFML/Graphics.hpp>
#endif


static BOOL OpenClipboard_ButTryABitHarder(HWND ClipboardOwner);
static INT  GetPixelDataOffsetForPackedDIB(const BITMAPINFOHEADER *BitmapInfoHeader);
static void PutBitmapInClipboard_AsDIB(HBITMAP hBitmap);
static void PutBitmapInClipboard_From32bppTopDownRGBAData(INT Width, INT Height, const void *Data32bppRGBA);

static BYTE* GetImageDataFromHBITMAP(HBITMAP hBitmap, int* width, int* height);


#if defined(TEST_CLIPBOARD)
int main(int argc, wchar_t *argv[])
{
    if (!OpenClipboard_ButTryABitHarder(NULL))
    {
        // Could not open clipboard. This usually indicates that another application is permanently blocking it.
        return 1;
    }

    HGLOBAL ClipboardDataHandle = (HGLOBAL)GetClipboardData(CF_DIB);
    if (!ClipboardDataHandle)
    {
        // Clipboard object is not a DIB, and is not auto-convertible to DIB
        CloseClipboard();
        return 0;
    }

    BITMAPINFOHEADER *BitmapInfoHeader = (BITMAPINFOHEADER *)GlobalLock(ClipboardDataHandle);
    assert(BitmapInfoHeader); // This can theoretically fail if mapping the HGLOBAL into local address space fails. Very pathological, just act as if it wasn't a bitmap in the clipboard.

    SIZE_T ClipboardDataSize = GlobalSize(ClipboardDataHandle);
    assert(ClipboardDataSize >= sizeof(BITMAPINFOHEADER)); // Malformed data. While older DIB formats exist (e.g. BITMAPCOREHEADER), they are not valid data for CF_DIB; it mandates a BITMAPINFO struct. If this fails, just act as if it wasn't a bitmap in the clipboard.

    INT PixelDataOffset = GetPixelDataOffsetForPackedDIB(BitmapInfoHeader);


    // ============================================================================================================
    // ============================================================================================================
    //
    // Example 1: Write it to a .bmp file
    //


    // The clipboard contains a packed DIB, whose start address coincides with BitmapInfoHeader, and whose total size is ClipboardDataSize.
    // By definition, we can jam the whole DIB memory into a BMP file as-is, except that we need to prepend a BITMAPFILEHEADER struct.
    // The tricky part is that for BITMAPFILEHEADER.bfOffBits, which must be calculated using the information in BITMAPINFOHEADER.

    // The BMP file layout:
    // @offset 0:                              BITMAPFILEHEADER
    // @offset 14 (sizeof(BITMAPFILEHEADER)):  BITMAPINFOHEADER
    // @offset 14 + BitmapInfoHeader->biSize:  Optional bit masks and color table
    // @offset 14 + DIBPixelDataOffset:        pixel bits
    // @offset 14 + ClipboardDataSize:         EOF
    size_t TotalBitmapFileSize = sizeof(BITMAPFILEHEADER) + ClipboardDataSize;
    wprintf(L"BITMAPINFOHEADER size:          %u\r\n", BitmapInfoHeader->biSize);
    wprintf(L"Format:                         %hubpp, Compression %u\r\n", BitmapInfoHeader->biBitCount, BitmapInfoHeader->biCompression);
    wprintf(L"Pixel data offset within DIB:   %u\r\n", PixelDataOffset);
    wprintf(L"Total DIB size:                 %zu\r\n", ClipboardDataSize);
    wprintf(L"Total bitmap file size:         %zu\r\n", TotalBitmapFileSize);

    BITMAPFILEHEADER BitmapFileHeader = {};
    BitmapFileHeader.bfType = 0x4D42;
    BitmapFileHeader.bfSize = (DWORD)TotalBitmapFileSize; // Will fail if bitmap size is nonstandard >4GB
    BitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + PixelDataOffset;

    HANDLE FileHandle = CreateFileW(L"test.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD dummy = 0;
        BOOL Success = true;
        Success &= WriteFile(FileHandle, &BitmapFileHeader, sizeof(BITMAPFILEHEADER), &dummy, NULL);
        Success &= WriteFile(FileHandle, BitmapInfoHeader, (DWORD)ClipboardDataSize, &dummy, NULL);
        Success &= CloseHandle(FileHandle);
        if (Success)
        {
            wprintf(L"File saved.\r\n");
        }
    }

#if DEMO_SFML

    // ============================================================================================================
    // ============================================================================================================
    //
    // Example 2: Load it from memory in SFML
    //


    // SFML expects a whole bitmap file, including its BITMAPFILEHEADER, in memory.
    // So this is similar to Example 1, except in memory.
    BYTE *BitmapFileContents = (BYTE *)malloc(TotalBitmapFileSize);
    assert(BitmapFileContents);
    memcpy(BitmapFileContents, &BitmapFileHeader, sizeof(BITMAPFILEHEADER));
    // Append DIB
    memcpy(BitmapFileContents + sizeof(BITMAPFILEHEADER), BitmapInfoHeader, ClipboardDataSize);
    sf::Image image;
    image.loadFromMemory(BitmapFileContents, TotalBitmapFileSize);
    // The memory can be freed once the image has been loaded in SFML.
    free(BitmapFileContents);

    // Manipulate it:
    image.flipHorizontally();
                
    // Put it back in the clipboard:
    PutBitmapInClipboard_From32bppTopDownRGBAData(image.getSize().x, image.getSize().y, image.getPixelsPtr());

#else

    // ============================================================================================================
    // ============================================================================================================
    //
    // Example 3: Convert to HBITMAP for GDI
    //


    BYTE *PixelDataFromClipboard = (BYTE *)BitmapInfoHeader + PixelDataOffset;
    // This will only work if the DIB format is supported by GDI. Not all formats are supported.
    BYTE *PixelDataNew;
    HBITMAP hBitmap = CreateDIBSection(NULL, (BITMAPINFO *)BitmapInfoHeader, DIB_RGB_COLORS, (void **)&PixelDataNew, NULL, 0);
    
    assert(hBitmap);

    // Need to copy the data from the clipboard to the new DIBSection.
    BITMAP BitmapDesc = {};
    GetObjectW(hBitmap, sizeof(BitmapDesc), &BitmapDesc);
    SIZE_T PixelDataBytesToCopy = (SIZE_T)BitmapDesc.bmHeight * BitmapDesc.bmWidthBytes;
    SIZE_T PixelDataBytesAvailable = ClipboardDataSize - PixelDataOffset;
    if (PixelDataBytesAvailable < PixelDataBytesToCopy)
    {
        // Malformed data; doesn't contain enough pixels.
        PixelDataBytesToCopy = PixelDataBytesAvailable;
    }
    memcpy(PixelDataNew, PixelDataFromClipboard, PixelDataBytesToCopy);
    // NOTE: While it is possible to create a DIB section without copying the pixel data, in general you'd want to
    //       copy it anyway because the clipboard needs to be closed asap.

    // Draw something on it.
    PixelDataNew[7] = 0;
    PixelDataNew[11] = 100;
    HDC hdc = CreateCompatibleDC(NULL);
    assert(hdc);
    SelectObject(hdc, hBitmap);
    RECT rc = { 0, 0, BitmapDesc.bmWidth / 2, BitmapDesc.bmHeight / 2 };
    HBRUSH brush = CreateSolidBrush(RGB(250, 100, 0));
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);
    DeleteDC(hdc);


    // ============================================================================================================
    // ============================================================================================================
    //
    // Copy it back to the clipboard.
    //


    PutBitmapInClipboard_AsDIB(hBitmap);
#endif // DEMO_SFML

    GlobalUnlock(ClipboardDataHandle);

    CloseClipboard();

    return 0;
}
#endif


static BOOL OpenClipboard_ButTryABitHarder(HWND hWnd)
{
    for (int i = 0; i < 20; ++i)
    {
        // This can fail if the clipboard is currently being accessed by another application.
        if (OpenClipboard(hWnd)) return true;
        Sleep(10);
    }
    return false;
}


// Returns the offset, in bytes, from the start of the BITMAPINFO, to the start of the pixel data array, for a packed DIB.
static INT GetPixelDataOffsetForPackedDIB(const BITMAPINFOHEADER *BitmapInfoHeader)
{
    INT OffsetExtra = 0;

    if (BitmapInfoHeader->biSize == sizeof(BITMAPINFOHEADER) /* 40 */)
    {
        // This is the common BITMAPINFOHEADER type. In this case, there may be bit masks following the BITMAPINFOHEADER
        // and before the actual pixel bits (does not apply if bitmap has <= 8 bpp)
        if (BitmapInfoHeader->biBitCount > 8)
        {
            if (BitmapInfoHeader->biCompression == BI_BITFIELDS)
            {
                OffsetExtra += 3 * sizeof(RGBQUAD);
            }
            else if (BitmapInfoHeader->biCompression == 6 /* BI_ALPHABITFIELDS */)
            {
                // Not widely supported, but valid.
                OffsetExtra += 4 * sizeof(RGBQUAD);
            }
        }
    }

    if (BitmapInfoHeader->biClrUsed > 0)
    {
        // We have no choice but to trust this value.
        OffsetExtra += BitmapInfoHeader->biClrUsed * sizeof(RGBQUAD);
    }
    else
    {
        // In this case, the color table contains the maximum number for the current bit count (0 if > 8bpp)
        if (BitmapInfoHeader->biBitCount <= 8)
        {
            // 1bpp: 2
            // 4bpp: 16
            // 8bpp: 256
            OffsetExtra += sizeof(RGBQUAD) << BitmapInfoHeader->biBitCount;
        }
    }

    return BitmapInfoHeader->biSize + OffsetExtra;
}


// Helper function for interaction with libraries like stb_image.
// Data will be copied, so you can do what you want with it after this function returns.
static void PutBitmapInClipboard_From32bppTopDownRGBAData(INT Width, INT Height, const void *Data32bppRGBA)
{
    // Nomenclature: Data at offset 0 is R top left corner, offset 1 is G top left corner, etc.
    //               This is pretty much the opposite of what a HBITMAP normally does.
    assert(Width > 0);
    assert(Height > 0);
    assert(Data32bppRGBA);

    // GDI won't help us here if we want to preserve the alpha channel. It doesn't support BI_ALPHABITFIELDS, and
    // we can't use BI_RGB directly because BI_RGB actually means BGRA in reality.
    // That means, unfortunately it's not going to be a simple memcpy :(
    
    DWORD PixelDataSize = 4/*32bpp*/ * Width * Height;
    // We need BI_BITFIELDS for RGB color masks here.
    size_t TotalSize = sizeof(BITMAPINFOHEADER) + PixelDataSize;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, TotalSize);
    assert(hGlobal);
    void *mem = GlobalLock(hGlobal);
    assert(mem);

    BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)mem;
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = Width;
    bih->biHeight = -Height; // Negative height means top-down bitmap
    bih->biPlanes = 1;
    bih->biBitCount = 32;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = PixelDataSize;

    BYTE *PixelData = (BYTE *)mem + sizeof(BITMAPINFOHEADER);
    DWORD NumPixels = Width * Height;
    for (DWORD i = 0; i < NumPixels; ++i)
    {
        // Convert RGBA to BGRA
        DWORD tmp = ((DWORD *)Data32bppRGBA)[i];
        DWORD tmp2 = tmp & 0xff00ff00; // assumes LE
        tmp2 |= (tmp >> 16) & 0xff;
        tmp2 |= (tmp & 0xff) << 16;
        ((DWORD *)PixelData)[i] = tmp2;
    }

    GlobalUnlock(hGlobal);

    EmptyClipboard();
    SetClipboardData(CF_DIB, hGlobal);

    // The hGlobal now belongs to the clipboard. Do not free it.
}


// Bitmap will be copied, so you can do what you want with it after this function returns.
static void PutBitmapInClipboard_AsDIB(HBITMAP hBitmap)
{
    // Need this to get the bitmap dimensions.
    BITMAP desc = {};
    int tmp = GetObjectW(hBitmap, sizeof(desc), &desc);
    assert(tmp != 0);

    // We need to build this structure in a GMEM_MOVEABLE global memory block:
    //   BITMAPINFOHEADER (40 bytes)
    //   PixelData (4 * Width * Height bytes)
    // We're enforcing 32bpp BI_RGB, so no bitmasks and no color table.
    // NOTE: SetClipboardData(CF_DIB) insists on the size 40 version of BITMAPINFOHEADER, otherwise it will misinterpret the data.

    DWORD PixelDataSize = 4/*32bpp*/ * desc.bmWidth * desc.bmHeight; // Correct alignment happens implicitly.
    assert(desc.bmWidth > 0);
    assert(desc.bmHeight > 0);
    size_t TotalSize = sizeof(BITMAPINFOHEADER) + PixelDataSize;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, TotalSize);
    assert(hGlobal);
    void *mem = GlobalLock(hGlobal);
    assert(mem);

    BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)mem;
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = desc.bmWidth;
    bih->biHeight = desc.bmHeight;
    bih->biPlanes = 1;
    bih->biBitCount = 32;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = PixelDataSize;
    HDC hdc = CreateCompatibleDC(NULL);
    assert(hdc);
    HGDIOBJ old = SelectObject(hdc, hBitmap);
    assert(old != NULL); // This can fail if the hBitmap is still selected into a different DC.
    void *PixelData = (BYTE *)mem + sizeof(BITMAPINFOHEADER);
    // Pathologial "bug": If the bitmap is a DDB that originally belonged to a device with a different palette, that palette is lost. The caller would need to give us the correct HDC, but this is already insane enough as it is.
    tmp = GetDIBits(hdc, hBitmap, 0, desc.bmHeight, PixelData, (BITMAPINFO *)bih, DIB_RGB_COLORS);
    assert(tmp != 0);
    // NOTE: This will correctly preserve the alpha channel if possible, but it's up to the receiving application to handle it.
    DeleteDC(hdc);

    GlobalUnlock(hGlobal);

    EmptyClipboard();
    SetClipboardData(CF_DIB, hGlobal);

    // The hGlobal now belongs to the clipboard. Do not free it.
}


char* GetClipboardImage(int* width, int* height, size_t *data_size) {

    if (!OpenClipboard_ButTryABitHarder(NULL)) {
        // Could not open clipboard. This usually indicates that another application is permanently blocking it.
        return NULL;
    }

    HGLOBAL clip_handle = (HGLOBAL)GetClipboardData(CF_DIB);

    if (!clip_handle) {
        // Clipboard object is not a DIB, and is not auto-convertible to DIB
        CloseClipboard();
        return NULL;
    }

    BITMAPINFOHEADER *bmp_info_header = (BITMAPINFOHEADER *)GlobalLock(clip_handle);
    assert(bmp_info_header); // This can theoretically fail if mapping the HGLOBAL into local address space fails. Very pathological, just act as if it wasn't a bitmap in the clipboard.

    *width = bmp_info_header->biWidth;
    *height = bmp_info_header->biHeight;

    SIZE_T clip_data_size = GlobalSize(clip_handle);
    assert(clip_data_size >= sizeof(bmp_info_header[0])); // Malformed data. While older DIB formats exist (e.g. BITMAPCOREHEADER), they are not valid data for CF_DIB; it mandates a BITMAPINFO struct. If this fails, just act as if it wasn't a bitmap in the clipboard.
    INT pixel_offset = GetPixelDataOffsetForPackedDIB(bmp_info_header);


    BITMAPFILEHEADER bmp_file_header = {};
    size_t bmp_file_size = sizeof(bmp_file_header) + clip_data_size;
    bmp_file_header.bfType = 0x4D42;
    bmp_file_header.bfSize = (DWORD)bmp_file_size; // Will fail if bitmap size is nonstandard >4GB
    bmp_file_header.bfOffBits = sizeof(bmp_file_header) + pixel_offset;
    // if (compression == 1) image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    // else if (compression == 2) image.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
    // else if (compression == 3) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    // else if (compression == 4) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    BYTE* bmp_data = malloc(sizeof(bmp_file_header) + clip_data_size);

    memcpy(bmp_data, &bmp_file_header, sizeof(bmp_file_header));
    memcpy(bmp_data + sizeof(bmp_file_header), bmp_info_header, clip_data_size);
    *data_size = sizeof(bmp_file_header) + clip_data_size;

    GlobalUnlock(clip_handle);
    CloseClipboard();
    return bmp_data;
}

static BYTE* GetImageDataFromHBITMAP(HBITMAP hBitmap, int* width, int* height) {
    // Need this to get the bitmap dimensions.
    BITMAP desc = {};
    int tmp = GetObjectW(hBitmap, sizeof(desc), &desc);
    assert(tmp != 0);

    // We need to build this structure in a GMEM_MOVEABLE global memory block:
    //   BITMAPINFOHEADER (40 bytes)
    //   PixelData (4 * Width * Height bytes)
    // We're enforcing 32bpp BI_RGB, so no bitmasks and no color table.
    // NOTE: SetClipboardData(CF_DIB) insists on the size 40 version of BITMAPINFOHEADER, otherwise it will misinterpret the data.

    DWORD PixelDataSize = 4/*32bpp*/ * desc.bmWidth * desc.bmHeight; // Correct alignment happens implicitly.
    assert(desc.bmWidth > 0);
    assert(desc.bmHeight > 0);
    size_t TotalSize = sizeof(BITMAPINFOHEADER) + PixelDataSize;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, TotalSize);
    assert(hGlobal);
    void *mem = GlobalLock(hGlobal);
    assert(mem);
    *width = desc.bmWidth;
    *height = desc.bmHeight;

    BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)mem;
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = desc.bmWidth;
    bih->biHeight = desc.bmHeight;
    bih->biPlanes = 1;
    bih->biBitCount = 32;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = PixelDataSize;
    
    void *PixelData = (BYTE *)mem + sizeof(BITMAPINFOHEADER);
    GlobalUnlock(hGlobal);
    return PixelData;
}
#else
#    error "GetClipboardImage is not supported on anything other than windows"
#endif

