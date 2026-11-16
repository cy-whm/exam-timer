// 测试辅助：向计时器窗口投递点击（坐标按 DIP 传入，内部换算为物理像素）
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

#pragma comment(lib, "user32.lib")

static HWND FindTimer() { return FindWindowW(L"ExamTimerWindow", nullptr); }

int wmain(int argc, wchar_t** argv) {
    HWND h = FindTimer();
    if (!h) {
        wprintf(L"NOTFOUND\n");
        return 1;
    }

    if (argc >= 4 && _wcsicmp(argv[1], L"hover") == 0) {
        double dx = _wtof(argv[2]);
        double dy = _wtof(argv[3]);
        UINT dpi = GetDpiForWindow(h);
        if (dpi == 0) dpi = 96;
        double s = dpi / 96.0;
        int px = static_cast<int>(dx * s + 0.5);
        int py = static_cast<int>(dy * s + 0.5);
        PostMessageW(h, WM_MOUSEMOVE, 0, MAKELPARAM(px, py));
        wprintf(L"hover dip(%.1f,%.1f)\n", dx, dy);
        return 0;
    }

    if (argc >= 4 && _wcsicmp(argv[1], L"click") == 0) {
        double dx = _wtof(argv[2]);
        double dy = _wtof(argv[3]);
        UINT dpi = GetDpiForWindow(h);
        if (dpi == 0) dpi = 96;
        double s = dpi / 96.0;
        int px = static_cast<int>(dx * s + 0.5);
        int py = static_cast<int>(dy * s + 0.5);

        PostMessageW(h, WM_MOUSEMOVE, 0, MAKELPARAM(px, py));
        Sleep(40);
        PostMessageW(h, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(px, py));
        Sleep(40);
        PostMessageW(h, WM_LBUTTONUP, 0, MAKELPARAM(px, py));
        Sleep(60);
        wprintf(L"click dip(%.1f,%.1f) -> px(%d,%d) dpi=%u\n", dx, dy, px, py, dpi);
        return 0;
    }

    if (argc >= 3 && _wcsicmp(argv[1], L"key") == 0) {
        UINT vk = (wcscmp(argv[2], L"space") == 0) ? VK_SPACE : static_cast<UINT>(_wtoi(argv[2]));
        PostMessageW(h, WM_KEYDOWN, vk, 0);
        Sleep(60);
        PostMessageW(h, WM_KEYUP, vk, 0);
        wprintf(L"key vk=%u sent\n", vk);
        return 0;
    }

    if (argc >= 2 && _wcsicmp(argv[1], L"min") == 0) {
        ShowWindow(h, SW_MINIMIZE);
        wprintf(L"minimized\n");
        return 0;
    }

    if (argc >= 2 && _wcsicmp(argv[1], L"restore") == 0) {
        ShowWindow(h, SW_RESTORE);
        ShowWindow(h, SW_SHOW);
        SetForegroundWindow(h);
        wprintf(L"restored\n");
        return 0;
    }

    if (argc >= 2 && _wcsicmp(argv[1], L"top") == 0) {
        SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(h);
        wprintf(L"topmost\n");
        return 0;
    }

    if (argc >= 2 && _wcsicmp(argv[1], L"close") == 0) {
        PostMessageW(h, WM_CLOSE, 0, 0);
        return 0;
    }

    wprintf(L"usage: uictl click <dipX> <dipY> | close\n");
    return 2;
}
