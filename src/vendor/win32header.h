
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

