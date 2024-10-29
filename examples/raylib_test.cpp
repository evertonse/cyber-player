#include <iostream>
#include <fstream>
#include <windows.h>

// https://stackoverflow.com/questions/30552255/how-to-read-a-bitmap-from-the-windows-clipboard

int main()
{
    std::cout<<"Format Bitmap: "<<IsClipboardFormatAvailable(CF_BITMAP)<<"\n";
    std::cout<<"Format DIB: "<<IsClipboardFormatAvailable(CF_DIB)<<"\n";
    std::cout<<"Format DIBv5: "<<IsClipboardFormatAvailable(CF_DIBV5)<<"\n";

    if (IsClipboardFormatAvailable(CF_DIB))
    {
        if (OpenClipboard(NULL))
        {
            HANDLE hClipboard = GetClipboardData(CF_DIB);
            if (hClipboard != NULL && hClipboard != INVALID_HANDLE_VALUE)
            {
                void* dib = GlobalLock(hClipboard);

                if (dib)
                {
                    BITMAPINFOHEADER* info = reinterpret_cast<BITMAPINFOHEADER*>(dib);
                    BITMAPFILEHEADER fileHeader = {0};
                    fileHeader.bfType = 0x4D42;
                    fileHeader.bfOffBits = 54;
                    fileHeader.bfSize = (((info->bmiHeader.biWidth * info->bmiHeader.biBitCount + 31) & ~31) / 8
                              * info->bmiHeader.biHeight) + fileHeader.bfOffBits;

                    std::cout<<"Type: "<<std::hex<<fileHeader.bfType<<std::dec<<"\n";
                    std::cout<<"bfSize: "<<fileHeader.bfSize<<"\n";
                    std::cout<<"Reserved: "<<fileHeader.bfReserved1<<"\n";
                    std::cout<<"Reserved2: "<<fileHeader.bfReserved2<<"\n";
                    std::cout<<"Offset: "<<fileHeader.bfOffBits<<"\n";
                    std::cout<<"biSize: "<<info->bmiHeader.biSize<<"\n";
                    std::cout<<"Width: "<<info->bmiHeader.biWidth<<"\n";
                    std::cout<<"Height: "<<info->bmiHeader.biHeight<<"\n";
                    std::cout<<"Planes: "<<info->bmiHeader.biPlanes<<"\n";
                    std::cout<<"Bits: "<<info->bmiHeader.biBitCount<<"\n";
                    std::cout<<"Compression: "<<info->bmiHeader.biCompression<<"\n";
                    std::cout<<"Size: "<<info->bmiHeader.biSizeImage<<"\n";
                    std::cout<<"X-res: "<<info->bmiHeader.biXPelsPerMeter<<"\n";
                    std::cout<<"Y-res: "<<info->bmiHeader.biYPelsPerMeter<<"\n";
                    std::cout<<"ClrUsed: "<<info->bmiHeader.biClrUsed<<"\n";
                    std::cout<<"ClrImportant: "<<info->bmiHeader.biClrImportant<<"\n";

                    // std::ofstream file("C:/Users/Brandon/Desktop/Test.bmp", std::ios::out | std::ios::binary);
                    std::ofstream file("test.bmp", std::ios::out | std::ios::binary);
                    if (file)
                    {
                        file.write(reinterpret_cast<char*>(&fileHeader), sizeof(BITMAPFILEHEADER));
                        file.write(reinterpret_cast<char*>(info), sizeof(BITMAPINFOHEADER));
                        file.write(reinterpret_cast<char*>(++info), bmp.dib.biSizeImage);
                    }

                    GlobalUnlock(dib);
                }
            }

            CloseClipboard();
        }
    }

    return 0;
}
