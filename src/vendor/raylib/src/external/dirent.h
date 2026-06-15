
//
// minirent.h UTF-8 dirent for Windows (no <windows.h> required)
//   x86_64-w64-mingw32-gcc -D_WIN32 -DMINIRENT_TEST_MAIN -x c dirent.h -o dirent.exe && ./dirent.exe//
//
#define MINIRENT_IMPLEMENTATION

#ifdef MINIRENT_TEST_MAIN
#ifndef MINIRENT_IMPLEMENTATION
#define MINIRENT_IMPLEMENTATION
#endif
#endif

#ifndef _WIN32
#include <dirent.h>
#else // _WIN32

#ifndef MINIRENT_H_
#define MINIRENT_H_

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Minimal Win32 types
#ifndef MINIRENT_WIN32_TYPES
#define MINIRENT_WIN32_TYPES

#define MINIRENT_MAX_PATH 260
#define MINIRENT_CP_UTF8 65001U
#define MINIRENT_ERROR_NO_MORE 18UL
#define MINIRENT_FILE_ATTRIBUTE_DIRECTORY 0x10UL

typedef void *MINIRENT_HANDLE;
typedef unsigned long MINIRENT_DWORD;
typedef int MINIRENT_BOOL;
typedef unsigned short MINIRENT_WCHAR;

#define MINIRENT_INVALID_HANDLE ((MINIRENT_HANDLE)(~(size_t)0))

typedef struct {
   MINIRENT_DWORD lo, hi;
} MINIRENT_FILETIME;

typedef struct {
    MINIRENT_DWORD    dwFileAttributes;
    MINIRENT_FILETIME ftCreationTime;
    MINIRENT_FILETIME ftLastAccessTime;
    MINIRENT_FILETIME ftLastWriteTime;
    MINIRENT_DWORD    nFileSizeHigh;
    MINIRENT_DWORD    nFileSizeLow;
    MINIRENT_DWORD    dwReserved0;
    MINIRENT_DWORD    dwReserved1;
    MINIRENT_WCHAR    cFileName[MINIRENT_MAX_PATH];
    MINIRENT_WCHAR    cAlternateFileName[14];
}   MINIRENT_FIND_DATAW;

//                    Direct          kernel32  imports                        —                   no CRT                 wrapper involvement
__declspec(dllimport) MINIRENT_HANDLE __stdcall FindFirstFileW(const           MINIRENT_WCHAR      *, MINIRENT_FIND_DATAW *);
__declspec(dllimport) MINIRENT_BOOL   __stdcall FindNextFileW(MINIRENT_HANDLE, MINIRENT_FIND_DATAW *);
__declspec(dllimport) MINIRENT_BOOL   __stdcall FindClose(MINIRENT_HANDLE);
__declspec(dllimport) MINIRENT_DWORD  __stdcall GetLastError(void);

__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int CodePage, MINIRENT_DWORD dwFlags, const char *lpMultiByteStr, int cbMultiByte, MINIRENT_WCHAR *lpWideCharStr, int cchWideChar);

#ifndef RAYLIB_H // Raylib already defines these

__declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int CodePage, MINIRENT_DWORD dwFlags, const MINIRENT_WCHAR *lpWideCharStr, int cchWideChar, char *lpMultiByteStr, int cbMultiByte, const char *lpDefaultChar, MINIRENT_BOOL *lpUsedDefaultChar);
#endif

#endif // MINIRENT_WIN32_TYPES

// UTF-8 d_name: up to 3 UTF-8 bytes per UTF-16 code unit, plus null byte
#define MINIRENT_MAX_UTF8 (MINIRENT_MAX_PATH * 3 + 1)

struct dirent {
   char d_name[MINIRENT_MAX_UTF8];
   MINIRENT_DWORD d_attrib; // raw WIN32_FIND_DATAW.dwFileAttributes might be useful?
};

typedef struct DIR DIR;

DIR *opendir(const char *dirpath);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif // MINIRENT_H_

#ifdef MINIRENT_IMPLEMENTATION

struct DIR {
   MINIRENT_HANDLE     hFind;
   MINIRENT_FIND_DATAW data;
   struct dirent      *dirent;
};

static int minirent__multibyte_to_wide(const char *src, int srclen, MINIRENT_WCHAR *dst, int dstsz) {
    return MultiByteToWideChar(MINIRENT_CP_UTF8, 0, (void *)src, srclen, (void *)dst, dstsz);
}

static int minirent__wide_to_multibyte(const MINIRENT_WCHAR *src, char *dst, int dstsz) {
    return WideCharToMultiByte(MINIRENT_CP_UTF8, 0, (void *)src, -1, (void *)dst, dstsz, (void *)0, (void *)0);
}

// Normalise slashes to backslash, collapse runs, strip trailing backslash
// (but keep drive roots like "C:\").
// Returns written length (without null byte).
static size_t minirent__clean_path(const char *src, char *dst, size_t dstsz) {
   if (!src || dstsz == 0)
      return 0;

   size_t wi = 0, si = 0;

   while (src[si] != '\0' && wi < dstsz - 1) {
      char c = src[si++];
      if (c == '/' || c == '\\') {
         // Preserve leading "\\" for UNC paths
         int is_unc = (wi == 0 && (src[si] == '/' || src[si] == '\\'));
         dst[wi++] = '\\';
         if (is_unc && wi < dstsz - 1)
            dst[wi++] = '\\';
         while (src[si] == '/' || src[si] == '\\')
            si++;
      } else {
         dst[wi++] = c;
      }
   }

   // Strip trailing backslashes unless it's a drive root ("C:\")
   while (wi > 1 && dst[wi - 1] == '\\') {
      if (wi >= 3 && dst[wi - 2] == ':')
         break;
      wi--;
   }

   dst[wi] = '\0';
   return wi;
}

// Build a wide-char "path\*" glob string for FindFirstFileW.
// Caller must free() the returned buffer.
static MINIRENT_WCHAR *minirent__make_glob(const char *utf8) {
   char clean[MINIRENT_MAX_PATH * 4];
   size_t clen = minirent__clean_path(utf8, clean, sizeof(clean));
   if (clen == 0)
      return NULL;

   // Query required wide-char buffer size
   int wlen = minirent__multibyte_to_wide(clean, (int)clen, NULL, 0);
   if (wlen <= 0)
      return NULL;

   // Allocate wide chars + backslash + asterisk + null byte
   MINIRENT_WCHAR *buf = (MINIRENT_WCHAR *)calloc((size_t)(wlen + 3), sizeof(MINIRENT_WCHAR));
   if (!buf)
      return NULL;

   minirent__multibyte_to_wide(clean, (int)clen, buf, wlen);
   buf[wlen] = (MINIRENT_WCHAR)'\\';
   buf[wlen + 1] = (MINIRENT_WCHAR)'*';
   buf[wlen + 2] = (MINIRENT_WCHAR)'\0';
   return buf;
}

DIR *opendir(const char *dirpath) {
   assert(dirpath);

   MINIRENT_WCHAR *glob = minirent__make_glob(dirpath);
   if (!glob) {
      errno = ENOMEM;
      return NULL;
   }

   DIR *dir = (DIR *)calloc(1, sizeof(DIR));
   if (!dir) {
      free(glob);
      errno = ENOMEM;
      return NULL;
   }

   dir->hFind = FindFirstFileW(glob, &dir->data);
   free(glob);

   if (dir->hFind == MINIRENT_INVALID_HANDLE) {
      errno = ENOENT;
      free(dir);
      return NULL;
   }
   return dir;
}

struct dirent *readdir(DIR *dirp) {
   assert(dirp);

   if (dirp->dirent == NULL) {
      dirp->dirent = (struct dirent *)calloc(1, sizeof(struct dirent));
      if (!dirp->dirent) {
         errno = ENOMEM;
         return NULL;
      }
   } else {
      if (!FindNextFileW(dirp->hFind, &dirp->data)) {
         if (GetLastError() != MINIRENT_ERROR_NO_MORE)
            errno = ENOSYS;
         return NULL;
      }
   }

   memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));
   if (!minirent__wide_to_multibyte(dirp->data.cFileName, dirp->dirent->d_name, (int)sizeof(dirp->dirent->d_name))) {
      errno = EILSEQ;
      return NULL;
   }

   dirp->dirent->d_attrib = dirp->data.dwFileAttributes;
   return dirp->dirent;
}

int closedir(DIR *dirp) {
   assert(dirp);
   int rc = FindClose(dirp->hFind) ? 0 : (errno = ENOSYS, -1);
   if (dirp->dirent)
      free(dirp->dirent);
   free(dirp);
   return rc;
}

#endif // MINIRENT_IMPLEMENTATION

#ifdef MINIRENT_TEST_MAIN

#include <stdio.h>

__declspec(dllimport) MINIRENT_BOOL __stdcall SetConsoleOutputCP(unsigned int wCodePageID);
__declspec(dllimport) MINIRENT_BOOL __stdcall SetConsoleCP(unsigned int wCodePageID);
int main(int argc, char **argv) {
   const char *default_directory = "G:\\Media\\memes";
   const char *path = (argc > 1) ? argv[1] : default_directory;

   // NOTE: To actually see utf8 on the console we need this apparently
   SetConsoleOutputCP(MINIRENT_CP_UTF8);
   SetConsoleCP(MINIRENT_CP_UTF8);

   printf("opendir(\"%s\")\n", path);
   printf("------------------------------------\n");

   DIR *dir = opendir(path);
   if (!dir) {
      perror("opendir");
      return 1;
   }

   struct dirent *ent;
   int n_dirs = 0, n_files = 0;

   while ((ent = readdir(dir)) != NULL) {
      int is_dir = (ent->d_attrib & MINIRENT_FILE_ATTRIBUTE_DIRECTORY) != 0;
      printf("  [%s] %s\n", is_dir ? "DIR " : "FILE", ent->d_name);
      if (is_dir)
         n_dirs++;
      else
         n_files++;
   }

   closedir(dir);

   printf("------------------------------------\n");
   printf("dirs=%d  files=%d\n", n_dirs, n_files);
   return 0;
}

#endif // MINIRENT_TEST_MAIN

#endif // _WIN32
