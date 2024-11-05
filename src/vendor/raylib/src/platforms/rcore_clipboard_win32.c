#include "utils.h"
#if !defined(_WIN32)
#   error "This module is only made for Windows OS"
#else
// Needs both `Image` and `LoadImageFromMemory` from `rtexture` >:C

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <minwindef.h>
// #include <sdkddkver.h>
// #include <winuser.h>
// #include <minwinbase.h>
// #include <windows.h>

#ifndef WINAPI
#if defined(_ARM_)
#define WINAPI
#else
#define WINAPI __stdcall
#endif
#endif

#ifndef WINAPI
#if defined(_ARM_)
#define WINAPI
#else
#define WINAPI __stdcall
#endif
#endif

#ifndef WINBASEAPI
#ifndef _KERNEL32_
#define WINBASEAPI DECLSPEC_IMPORT
#else
#define WINBASEAPI
#endif
#endif

#ifndef WINUSERAPI
#ifndef _USER32_
#define WINUSERAPI __declspec (dllimport)
#else
#define WINUSERAPI
#endif
#endif

typedef int WINBOOL;



// typedef HANDLE HGLOBAL;

#ifndef HWND
#define HWND void*
#endif

#ifndef HDC
#define HDC void*
#endif

#ifndef HGDIOBJ
#define HGDIOBJ void*
#endif

#ifndef HBITMAP
#define HBITMAP void*
#endif

#ifndef HGLOBAL
#define HGLOBAL void*
#endif


#if !defined(_WINUSER_) || !defined(WINUSER_ALREADY_INCLUDED)
WINUSERAPI WINBOOL WINAPI OpenClipboard(HWND hWndNewOwner);
WINUSERAPI WINBOOL WINAPI CloseClipboard(VOID);
WINUSERAPI DWORD   WINAPI GetClipboardSequenceNumber(VOID);
WINUSERAPI HWND    WINAPI GetClipboardOwner(VOID);
WINUSERAPI HWND    WINAPI SetClipboardViewer(HWND hWndNewViewer);
WINUSERAPI HWND    WINAPI GetClipboardViewer(VOID);
WINUSERAPI WINBOOL WINAPI ChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext);
WINUSERAPI HANDLE  WINAPI SetClipboardData(UINT uFormat, HANDLE hMem);
WINUSERAPI HANDLE  WINAPI GetClipboardData(UINT uFormat);
WINUSERAPI UINT    WINAPI RegisterClipboardFormatA(LPCSTR  lpszFormat);
WINUSERAPI UINT    WINAPI RegisterClipboardFormatW(LPCWSTR lpszFormat);
WINUSERAPI int     WINAPI CountClipboardFormats(VOID);
WINUSERAPI UINT    WINAPI EnumClipboardFormats(UINT format);
WINUSERAPI int     WINAPI GetClipboardFormatNameA(UINT format, LPSTR  lpszFormatName, int cchMaxCount);
WINUSERAPI int     WINAPI GetClipboardFormatNameW(UINT format, LPWSTR lpszFormatName, int cchMaxCount);
WINUSERAPI WINBOOL WINAPI EmptyClipboard(VOID);
WINUSERAPI WINBOOL WINAPI IsClipboardFormatAvailable(UINT format);
WINUSERAPI int     WINAPI GetPriorityClipboardFormat(UINT *paFormatPriorityList, int cFormats);
WINUSERAPI HWND    WINAPI GetOpenClipboardWindow(VOID);
#endif


#if !defined(_WINBASE_) || !defined(WINBASE_ALREADY_INCLUDED)
WINBASEAPI SIZE_T  WINAPI GlobalSize (HGLOBAL hMem);
WINBASEAPI LPVOID  WINAPI GlobalLock (HGLOBAL hMem);
WINBASEAPI WINBOOL WINAPI GlobalUnlock (HGLOBAL hMem);

WINBASEAPI HGLOBAL WINAPI GlobalAlloc (UINT uFlags, SIZE_T dwBytes);
WINBASEAPI HGLOBAL WINAPI GlobalReAlloc (HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags);
WINBASEAPI HGLOBAL WINAPI GlobalFree (HGLOBAL hMem);
WINBASEAPI HLOCAL  WINAPI LocalReAlloc (HLOCAL hMem, SIZE_T uBytes, UINT uFlags);


#define GMEM_FIXED 0x0
#define GMEM_MOVEABLE 0x2
#define GMEM_NOCOMPACT 0x10
#define GMEM_NODISCARD 0x20
#define GMEM_ZEROINIT 0x40
#define GMEM_MODIFY 0x80
#define GMEM_DISCARDABLE 0x100
#define GMEM_NOT_BANKED 0x1000
#define GMEM_SHARE 0x2000
#define GMEM_DDESHARE 0x2000
#define GMEM_NOTIFY 0x4000
#define GMEM_LOWER GMEM_NOT_BANKED
#define GMEM_VALID_FLAGS 0x7f72
#define GMEM_INVALID_HANDLE 0x8000

#endif // _WINBASE_



#if !defined(_WINGDI_) || !defined(WINGDI_ALREADY_INCLUDED)
#ifdef _GDI32_
    #define WINGDIAPI
#else
    #define WINGDIAPI DECLSPEC_IMPORT
#endif

#ifndef BITMAPINFOHEADER_ALREADY_DEFINED
#define BITMAPINFOHEADER_ALREADY_DEFINED
// Does this header need to be packed ? by the windowps header it doesnt seem to be
#pragma pack(push, 1)
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;
#pragma pack(pop)
#endif

#ifndef BITMAPFILEHEADER_ALREADY_DEFINED
#define BITMAPFILEHEADER_ALREADY_DEFINED
#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
#pragma pack(pop)
#endif

#ifndef RGBQUAD_ALREADY_DEFINED
#define RGBQUAD_ALREADY_DEFINED
typedef struct tagRGBQUAD {
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;
#endif

#ifndef BITMAP_ALREADY_DEFINED
#define BITMAP_ALREADY_DEFINED
  typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
  } BITMAP,*PBITMAP,*NPBITMAP,*LPBITMAP;

#endif



// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-wmf/4e588f70-bd92-4a6f-b77f-35d0feaf7a57
#define BI_RGB       0x0000
#define BI_RLE8      0x0001
#define BI_RLE4      0x0002
#define BI_BITFIELDS 0x0003
#define BI_JPEG      0x0004
#define BI_PNG       0x0005
#define BI_CMYK      0x000B
#define BI_CMYKRLE8  0x000C
#define BI_CMYKRLE4  0x000D



#ifndef BITMAPINFO_ALREADY_DEFINED
#define BITMAPINFO_ALREADY_DEFINED
  typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
  } BITMAPINFO,*LPBITMAPINFO,*PBITMAPINFO;
#endif

#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1

WINGDIAPI HBITMAP WINAPI CreateBitmap(int nWidth,int nHeight,UINT nPlanes,UINT nBitCount, CONST VOID *lpBits);
WINGDIAPI HBITMAP WINAPI CreateBitmapIndirect(CONST BITMAP *pbm);
WINGDIAPI int WINAPI GetObjectA(HANDLE h,int c,LPVOID pv);
WINGDIAPI int WINAPI GetObjectW(HANDLE h,int c,LPVOID pv);
WINGDIAPI HGDIOBJ WINAPI SelectObject(HDC hdc,HGDIOBJ h);
WINGDIAPI HDC WINAPI CreateCompatibleDC(HDC hdc);
WINGDIAPI int WINAPI GetDIBits(HDC hdc,HBITMAP hbm,UINT start,UINT cLines,LPVOID lpvBits,LPBITMAPINFO lpbmi,UINT usage);
WINGDIAPI WINBOOL WINAPI DeleteDC(HDC hdc);
WINGDIAPI WINBOOL WINAPI DeleteMetaFile(HMETAFILE hmf);
WINGDIAPI WINBOOL WINAPI DeleteObject(HGDIOBJ ho);


#endif // WINDGI

// https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
#define CF_DIB    8
#define CF_BITMAP 2

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setsystemcursor
// #define OCR_NORMAL      32512 // Normal     select
// #define OCR_IBEAM       32513 // Text       select
// #define OCR_WAIT        32514 // Busy
// #define OCR_CROSS       32515 // Precision  select
// #define OCR_UP          32516 // Alternate  select
// #define OCR_SIZENWSE    32642 // Diagonal   resize 1
// #define OCR_SIZENESW    32643 // Diagonal   resize 2
// #define OCR_SIZEWE      32644 // Horizontal resize
// #define OCR_SIZENS      32645 // Vertical   resize
// #define OCR_SIZEALL     32646 // Move
// #define OCR_NO          32648 // Unavailable
// #define OCR_HAND        32649 // Link       select
// #define OCR_APPSTARTING 32650 //


//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------


static BOOL           OpenClipboardRetrying(HWND handle); // Open clipboard with a number of retries
static int            GetPixelDataOffset(BITMAPINFOHEADER bih);
static unsigned char* GetClipboardImageData(int* width, int* height, unsigned int *dataSize);
static void           PutBitmapInClipboardAsDIB(HBITMAP hBitmap);
static void           PutBitmapInClipboardFrom32bppTopDownRGBAData(INT Width, INT Height, const void *Data32bppRGBA);
//----------------------------------------------------------------------------------
// Module Functions Definition: Clipboard Image
//----------------------------------------------------------------------------------

Image GetClipboardImage(void)
{
    int width = 0;
    int height = 0;
    unsigned int dataSize = 0;
    unsigned char* fileData = GetClipboardImageData(&width, &height, &dataSize);
    Image image = LoadImageFromMemory(".bmp", fileData, dataSize);
    return image;
}

void SetClipboardImage(Image image)
{
    if (image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        TRACELOG(LOG_INFO, "We only support pixelformat uncompressed-R8G8B8A8 \n");
        image = ImageCopy(image);
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }
    PutBitmapInClipboardFrom32bppTopDownRGBAData(image.width, image.height, image.data);
}



static unsigned char* GetClipboardImageData(int* width, int* height, unsigned int *dataSize)
{
    HWND win = NULL; // Get from somewhere but is doesnt seem to matter
    const char* msgString = "";
    int severity = LOG_INFO;
    BYTE* bmpData = NULL;
    if (!OpenClipboardRetrying(win)) {
        severity = LOG_ERROR;
        msgString = "Couldn't open clipboard";
        goto end;
    }

    HGLOBAL clipHandle = (HGLOBAL)GetClipboardData(CF_DIB);
    if (!clipHandle) {
        severity = LOG_ERROR;
        msgString = "Clipboard data is not an Image";
        goto close;
    }

    BITMAPINFOHEADER *bmpInfoHeader = (BITMAPINFOHEADER *)GlobalLock(clipHandle);
    if (!bmpInfoHeader) {
        // Mapping from HGLOBAL to our local *address space* failed
        severity = LOG_ERROR;
        msgString = "Clipboard data failed to be locked";
        goto unlock;
    }

    *width = bmpInfoHeader->biWidth;
    *height = bmpInfoHeader->biHeight;

    SIZE_T clipDataSize = GlobalSize(clipHandle);
    if (clipDataSize < sizeof(BITMAPINFOHEADER)) {
        // Format CF_DIB needs space for BITMAPINFOHEADER struct.
        msgString = "Clipboard has Malformed data";
        severity = LOG_ERROR;
        goto unlock;
    }

    // Denotes where the pixel data starts from the bmpInfoHeader pointer
    int pixelOffset = GetPixelDataOffset(*bmpInfoHeader);

    //--------------------------------------------------------------------------------//
    //
    // The rest of the section is about create the bytes for a correct BMP file
    // Then we copy the data and to a pointer
    //
    //--------------------------------------------------------------------------------//

    BITMAPFILEHEADER bmpFileHeader = {0};
    SIZE_T bmpFileSize = sizeof(bmpFileHeader) + clipDataSize;
    *dataSize = bmpFileSize;

    bmpFileHeader.bfType = 0x4D42; //https://stackoverflow.com/questions/601430/multibyte-character-constants-and-bitmap-file-header-type-constants#601536

    bmpFileHeader.bfSize = (DWORD)bmpFileSize; // Up to 4GB works fine
    bmpFileHeader.bfOffBits = sizeof(bmpFileHeader) + pixelOffset;

    //
    // Each process has a default heap provided by the system
    // Memory objects allocated by GlobalAlloc and LocalAlloc are in private,
    // committed pages with read/write access that cannot be accessed by other processes.
    //
    // This may be wrong since we might be allocating in a DLL and freeing from another module, the main application
    // that may cause heap corruption. We could create a FreeImage function
    //
    bmpData = malloc(sizeof(bmpFileHeader) + clipDataSize);
    // First we add the header for a bmp file
    memcpy(bmpData, &bmpFileHeader, sizeof(bmpFileHeader));
    // Then we add the header for the bmp itself + the pixel data
    memcpy(bmpData + sizeof(bmpFileHeader), bmpInfoHeader, clipDataSize);
    msgString = "Clipboad image acquired successfully";


unlock:
    GlobalUnlock(clipHandle);
close:
    CloseClipboard();
end:

    TRACELOG(severity, msgString);
    return bmpData;
}

static BOOL OpenClipboardRetrying(HWND hWnd)
{
    static const int maxTries = 20;
    static const int sleepTimeMS = 60;
    for (int _ = 0; _ < maxTries; ++_)
    {
        // Might be being hold by another process
        // Or yourself forgot to CloseClipboard
        if (OpenClipboard(hWnd)) {
            return true;
        }
        Sleep(sleepTimeMS);
    }
    return false;
}

// Based off of researching microsoft docs and reponses from this question https://stackoverflow.com/questions/30552255/how-to-read-a-bitmap-from-the-windows-clipboard#30552856
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
// Get the byte offset where does the pixels data start (from a packed DIB)
static int GetPixelDataOffset(BITMAPINFOHEADER bih)
{
    int offset = 0;
    const unsigned int rgbaSize = sizeof(RGBQUAD);

    // biSize Specifies the number of bytes required by the structure
    // We expect to always be 40 because it should be packed
    if (40 == bih.biSize && 40 == sizeof(BITMAPINFOHEADER))
    {
        //
        // biBitCount Specifies the number of bits per pixel.
        // Might exist some bit masks *after* the header and *before* the pixel offset
        // we're looking, but only if we have more than
        // 8 bits per pixel, so we need to ajust for that
        //
        if (bih.biBitCount > 8)
        {
            // if bih.biCompression is RBG we should NOT offset more

            if (bih.biCompression == BI_BITFIELDS)
            {
                offset += 3 * rgbaSize;
            } else if (bih.biCompression == 6 /* BI_ALPHABITFIELDS */)
            {
                // Not widely supported, but valid.
                offset += 4 * rgbaSize;
            }
        }
    }

    //
    // biClrUsed Specifies the number of color indices in the color table that are actually used by the bitmap.
    // If this value is zero, the bitmap uses the maximum number of colors
    // corresponding to the value of the biBitCount member for the compression mode specified by biCompression.
    // If biClrUsed is nonzero and the biBitCount member is less than 16
    // the biClrUsed member specifies the actual number of colors
    //
    if (bih.biClrUsed > 0) {
        offset += bih.biClrUsed * rgbaSize;
    } else {
        if (bih.biBitCount < 16)
        {
            offset = offset + (rgbaSize << bih.biBitCount);
        }
    }

    return bih.biSize + offset;
}

static void PutBitmapInClipboardAsDIB(HBITMAP hBitmap)
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

// Helper function for interaction with libraries like stb_image.
// Data will be copied, so you can do what you want with it after this function returns.
static void PutBitmapInClipboardFrom32bppTopDownRGBAData(INT Width, INT Height, const void *Data32bppRGBA)
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

void SetClipboardAsBitmap(int width, int height, unsigned char *data) 
{
    HBITMAP bmpHandle = NULL;
    bmpHandle = CreateBitmap(width, height, 1, 32, data);

    if (!OpenClipboardRetrying(NULL)) {
        TRACELOG(LOG_WARNING, "Clipboard image: couldn't open clibboard");
        return;
    }
    EmptyClipboard();

    SetClipboardData(CF_BITMAP, bmpHandle);

    CloseClipboard();
    DeleteObject(bmpHandle);
}

#endif
// EOF

