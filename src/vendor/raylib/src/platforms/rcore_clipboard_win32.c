#if !defined(_WIN32)
#   error "This module is only made for Windows OS"
#else
// Needs both `Image` and `LoadImageFromMemory` from `rtexture` compleling >:C

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define DrawText DrawText_win32
#define Rectangle rectangle_win32
#define CloseWindow CloseWindow_win32
#define ShowCursor __imp_ShowCursor

// To avoid conflicting windows.h symbols with raylib, some flags are defined
// WARNING: Those flags avoid inclusion of some Win32 headers that could be required
// by user at some point and won't be included...
//-------------------------------------------------------------------------------------

#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
// #define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

// Type required before windows.h inclusion
typedef struct tagMSG *LPMSG;

#define WIN32_LEAN_AND_MEAN
// #include <sdkddkver.h>
// #include <windows.h>
// #include <winuser.h>
#include <minwindef.h>
// #include <minwinbase.h>

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

#ifndef HGLOBAL
#define HGLOBAL void*
#endif

#if !defined(_WINBASE_) || !defined(WINBASE_ALREADY_INCLUDED)
WINBASEAPI SIZE_T  WINAPI GlobalSize (HGLOBAL hMem);
WINBASEAPI LPVOID  WINAPI GlobalLock (HGLOBAL hMem);
WINBASEAPI WINBOOL WINAPI GlobalUnlock (HGLOBAL hMem);
#endif


#if !defined(_WINGDI_) || !defined(WINGDI_ALREADY_INCLUDED)
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

#endif

// https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
#define CF_DIB 8

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
//----------------------------------------------------------------------------------
// Module Functions Definition: Clipboard Image
//----------------------------------------------------------------------------------

Image GetClipboardImage(void)
{
    int width, height;
    unsigned int dataSize;
    unsigned char* fileData = GetClipboardImageData(&width, &height, &dataSize);
    Image image = LoadImageFromMemory(".bmp", fileData, dataSize);
    return image;
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
            // Should never happened RBG with more the 8bpp!
            assert(bih.biCompression != BI_RGB);

            if (bih.biCompression == BI_BITFIELDS)
            {
                offset += 3 * rgbaSize;
            }
            else if (bih.biCompression == 6 /* BI_ALPHABITFIELDS */)
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
            offset = offset + rgbaSize << bih.biBitCount;
        }
    }

    return bih.biSize + offset;
}

#undef NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#undef NOVIRTUALKEYCODES // VK_*
#undef NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#undef NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#undef NOSYSMETRICS      // SM_*
#undef NOMENUS           // MF_*
#undef NOICONS           // IDI_*
#undef NOKEYSTATES       // MK_*
#undef NOSYSCOMMANDS     // SC_*
#undef NORASTEROPS       // Binary and Tertiary raster ops
#undef NOSHOWWINDOW      // SW_*
#undef OEMRESOURCE       // OEM Resource values
#undef NOATOM            // Atom Manager routines
#undef NOCLIPBOARD       // Clipboard routines
#undef NOCOLOR           // Screen colors
#undef NOCTLMGR          // Control and Dialog routines
#undef NODRAWTEXT        // DrawText() and DT_*
#undef NOGDI             // All GDI defines and routines
#undef NOKERNEL          // All KERNEL defines and routines
#undef NOUSER            // All USER defines and routines
#undef NONLS             // All NLS defines and routines
#undef NOMB              // MB_* and MessageBox()
#undef NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#undef NOMETAFILE        // typedef METAFILEPICT
#undef NOMINMAX          // Macros min(a,b) and max(a,b)
#undef NOMSG             // typedef MSG and associated routines
#undef NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#undef NOSCROLL          // SB_* and scrolling routines
#undef NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#undef NOSOUND           // Sound driver routines
#undef NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#undef NOWH              // SetWindowsHook and WH_*
#undef NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#undef NOCOMM            // COMM driver routines
#undef NOKANJI           // Kanji support stuff.
#undef NOHELP            // Help engine interface.
#undef NOPROFILER        // Profiler interface.
#undef NODEFERWINDOWPOS  // DeferWindowPos routines
#undef NOMCX             // Modem Configuration Extensions

#undef DrawText
#undef ShowCursor
#undef CloseWindow
#undef Rectangle
#undef HWND
#endif
// EOF