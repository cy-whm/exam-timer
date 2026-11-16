// 打印计时器窗口的实际屏幕矩形（含 DWM 扩展边框），供截图脚本使用
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

int wmain(int argc, wchar_t** argv) {
    const wchar_t* cls = (argc > 1) ? argv[1] : L"ExamTimerWindow";
    HWND h = FindWindowW(cls, nullptr);
    if (!h) {
        wprintf(L"NOTFOUND\n");
        return 1;
    }
    RECT r{};
    HRESULT hr = DwmGetWindowAttribute(h, 9 /*DWMWA_EXTENDED_FRAME_BOUNDS*/, &r, sizeof(r));
    if (FAILED(hr)) GetWindowRect(h, &r);
    wprintf(L"%ld %ld %ld %ld\n", r.left, r.top, r.right - r.left, r.bottom - r.top);
    return 0;
}
