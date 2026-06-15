void _glfwSetClipboardBitmapWin32(unsigned char * data, int width, int height) {
    HBITMAP fillBm = NULL;
    fillBm = CreateBitmap(width, height, 1, 32, data);

    if (!OpenClipboard(_glfw.win32.helperWindowHandle)) {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR, "Win32: Failed to open clipboard");
        return;
    }
    EmptyClipboard();

    // place handle to clipboard
    SetClipboardData(CF_BITMAP, fillBm);

    // Close the clipboard. 
    CloseClipboard();
    DeleteObject(fillBm);
}
