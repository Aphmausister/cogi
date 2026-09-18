#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>

namespace {

void DrawDemo(HWND hwnd, HDC target, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(
        target, &bmi, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!bitmap) {
        return;
    }

    HDC buffer = CreateCompatibleDC(target);
    if (!buffer) {
        DeleteObject(bitmap);
        return;
    }

    HGDIOBJ previousBitmap = SelectObject(buffer, bitmap);

    // Paint only the off-screen buffer. The desktop is never modified.
    PatBlt(buffer, 0, 0, width, height, BLACKNESS);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            BYTE red = static_cast<BYTE>((255LL * x) / std::max(1, width - 1));
            BYTE green = static_cast<BYTE>((255LL * y) / std::max(1, height - 1));
            BYTE blue = 160;
            SetPixel(buffer, x, y, RGB(red, green, blue));
        }
    }

    // Exercise the permitted raster operations on the application window only.
    StretchBlt(target, 0, 0, width, height,
               buffer, 0, 0, width, height, SRCCOPY);
    BitBlt(target, 20, 20, width / 4, height / 4,
           buffer, 0, 0, SRCCOPY);

    SelectObject(buffer, previousBitmap);
    DeleteDC(buffer);
    DeleteObject(bitmap);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC hdc = BeginPaint(hwnd, &paint);
        RECT client{};
        GetClientRect(hwnd, &client);
        DrawDemo(hwnd, hdc, client.right - client.left,
                 client.bottom - client.top);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    constexpr wchar_t className[] = L"SafeGdiDemo";

    WNDCLASS windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

    if (!RegisterClass(&windowClass)) {
        return 1;
    }

    HWND window = CreateWindowEx(
        0, className, L"Safe GDI raster demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
        nullptr, nullptr, instance, nullptr);
    if (!window) {
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessage(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return static_cast<int>(message.wParam);
}
