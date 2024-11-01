/**********************************************************************************************
**********************************************************************************************/

#include <stdio.h>
#if !defined(_WIN32)
#   error "This module is only made for Windows OS"
#else

// To avoid conflicting windows.h symbols with raylib, some flags are defined
// WARNING: Those flags avoid inclusion of some Win32 headers that could be required
// by user at some point and won't be included...
//-------------------------------------------------------------------------------------

// // If defined, the following flags inhibit definition of the indicated items.
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
#define NOGDI             // All GDI defines and routines
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
#include <windows.h>
// #include <winuser.h>
// #include <minwindef.h>
// #include <minwinbase.h>

// typedef HANDLE HWND;
WINUSERAPI WINBOOL WINAPI OpenClipboard(HWND hWndNewOwner);
WINUSERAPI WINBOOL WINAPI CloseClipboard(VOID);
WINUSERAPI DWORD   WINAPI GetClipboardSequenceNumber(VOID);
WINUSERAPI HWND    WINAPI GetClipboardOwner(VOID);
WINUSERAPI HWND    WINAPI SetClipboardViewer(HWND hWndNewViewer);
WINUSERAPI HWND    WINAPI GetClipboardViewer(VOID);
WINUSERAPI WINBOOL WINAPI ChangeClipboardChain(HWND hWndRemove,            HWND   hWndNewNext);
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

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;


typedef struct tagRGBQUAD {
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;


// https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
#define CF_DIB 8



//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// typedef struct {
//     // TODO: add data to realloc clipboard stuff
// } PlatformData;


//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

// static PlatformData platform = { 0 };   // Platform specific data

//----------------------------------------------------------------------------------
// Local Variables Definition
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------

static BOOL OpenClipboardRetrying(HWND winHandle); // Open clipboard with a number of retries
static INT GetPixelDataOffsetForPackedDIB(const BITMAPINFOHEADER *BitmapInfoHeader);

//----------------------------------------------------------------------------------
// Module Functions Definition: Clipboard Image
//----------------------------------------------------------------------------------

char* GetClipboardImage(int* width, int* height, unsigned int *data_size) {

    HWND win = NULL; // Get from somewhere but is doesnt seem to matter
    const char* errorString = "";
    if (!OpenClipboardRetrying(win)) {
        errorString = "Couldn't open clipboard";
        return NULL;
    }

    HGLOBAL clipHandle = (HGLOBAL)GetClipboardData(CF_DIB);

    if (!clipHandle) {
        errorString = "Clipboard data is not an Image";
        printf("error STRING=%s\n", errorString);
        CloseClipboard();
        return NULL;
    }

    BITMAPINFOHEADER *bmpInfoHeader = (BITMAPINFOHEADER *)GlobalLock(clipHandle);

    if (!bmpInfoHeader) {
        // Mapping from HGLOBAL to our local *address space* failed
        errorString = "Clipboard data failed to be locked";
        printf("error STRING=%s\n", errorString);
        CloseClipboard();
        return NULL;
    }

    *width = bmpInfoHeader->biWidth;
    *height = bmpInfoHeader->biHeight;

    SIZE_T clipDataSize = GlobalSize(clipHandle);
    if (clipDataSize < sizeof(BITMAPINFOHEADER)) {
        // Format CF_DIB needs space for BITMAPINFOHEADER struct. 
        errorString = "Clipboard has Malformed data";
        printf("error STRING=%s\n", errorString);
        CloseClipboard();
        return NULL;
    }

    // Denotes where the pixel data starts from the bmpInfoHeader pointer
    INT pixelOffset = GetPixelDataOffsetForPackedDIB(bmpInfoHeader);

    printf("pixelOffset=%d\n", pixelOffset);
    //--------------------------------------------------------------------------------//
    //
    // The rest of the section is about create the bytes for a correct BMP file
    // Then we copy the data and to a pointer
    // 
    //--------------------------------------------------------------------------------//

    BITMAPFILEHEADER bmpFileHeader = {0};
    SIZE_T bmpFileSize = sizeof(bmpFileHeader) + clipDataSize;
    *data_size = bmpFileSize;

    bmpFileHeader.bfType = 0x4D42; //https://stackoverflow.com/questions/601430/multibyte-character-constants-and-bitmap-file-header-type-constants#601536

    bmpFileHeader.bfSize = (DWORD)bmpFileSize; // Up to 4GB works fine, i don't 
    bmpFileHeader.bfOffBits = sizeof(bmpFileHeader) + pixelOffset;
    // if (compression == 1) image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    // else if (compression == 2) image.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
    // else if (compression == 3) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    // else if (compression == 4) image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    // Each process has a default heap provided by the system
    // Memory objects allocated by GlobalAlloc and LocalAlloc are in private, 
    // committed pages with read/write access that cannot be accessed by other processes.
    //
    // This may be wrong since we might be allocating in a DLL and freeing from another module, the main application
    // that may cause heap corruption. We could create a FreeImage function
    //
    BYTE* bmpData = malloc(sizeof(bmpFileHeader) + clipDataSize);
    // First we add the header for a bmp file
    memcpy(bmpData, &bmpFileHeader, sizeof(bmpFileHeader));
    // Then we add the header for the bmp itself + the pixel data
    memcpy(bmpData + sizeof(bmpFileHeader), bmpInfoHeader, clipDataSize);

    printf("%s\n", "success");
    GlobalUnlock(clipHandle);
    CloseClipboard();
    return bmpData;
}

static BOOL OpenClipboardRetrying(HWND hWnd)
{
    static const int maxTries = 20;
    static const int sleepTime = 6;
    for (int _ = 0; _ < maxTries; ++_)
    {
        // Might be being hold by another process
        // Or yourself forgot to CloseClipboard
        if (OpenClipboard(hWnd)) {
            return true;
        }
        Sleep(sleepTime);
    }
    return false;
}

#define BI_RGB __MSABI_LONG(0)
#define BI_RLE8 __MSABI_LONG(1)
#define BI_RLE4 __MSABI_LONG(2)
#define BI_BITFIELDS __MSABI_LONG(3)
#define BI_JPEG __MSABI_LONG(4)
#define BI_PNG __MSABI_LONG(5)

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



#endif
// EOF
