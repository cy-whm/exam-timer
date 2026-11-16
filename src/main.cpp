#include "app.h"
#include "render.h"
#include "ui.h"

#include <windowsx.h>
#include <mmsystem.h>
#include <shobjidl.h>
#include <string.h>
#include <cstdlib>
#include <cwchar>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

#define WM_TRAYICON (WM_APP + 1)
#define TIMER_TICK 1
#define TIMER_SOUND 2
#define IDI_APP 101

static const float kMinW = 380.f;
static const float kMinH = 520.f;

static HINSTANCE g_inst = nullptr;
static HWND g_hwnd = nullptr;
static HICON g_iconBig = nullptr;
static NOTIFYICONDATAW g_nid{};
static Renderer g_rr;
static AppState g_app;
static std::vector<TzItem> g_zones;

static float g_dpiScale = 1.f;
static UINT  g_tickMs = 250;
static bool  g_keepAwake = false;
static int   g_soundRepeats = 0;
static double g_finishAt = -1.0;
static bool  g_exiting = false;
static int   g_winX = INT_MIN;
static int   g_winY = INT_MIN;

// ------------------------------------------------------------- helpers

static float Scale() { return g_dpiScale; }

static void ApplyWindowSize(HWND hwnd, float dipW, float dipH) {
    int w = static_cast<int>(dipW * Scale() + 0.5f);
    int h = static_cast<int>(dipH * Scale() + 0.5f);
    RECT rc{};
    GetWindowRect(hwnd, &rc);
    SetWindowPos(hwnd, nullptr, rc.left, rc.top, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

static void SyncWindowSizeFromPixels(int pxW, int pxH) {
    g_app.winW = static_cast<float>(pxW) / Scale();
    g_app.winH = static_cast<float>(pxH) / Scale();
}

static void UpdateKeepAwake() {
    bool need = g_app.cdRunning || g_app.swRunning || (g_app.alActive && !g_app.alFinished);
    if (need == g_keepAwake) return;
    g_keepAwake = need;
    if (need)
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    else
        SetThreadExecutionState(ES_CONTINUOUS);
}

static void UpdateTickRate() {
    bool timerAnim = g_app.cdRunning || g_app.swRunning ||
                     (g_app.alActive && !g_app.alFinished);
    bool pulse = (g_finishAt >= 0.0) && (SteadyNow() - g_finishAt < 6.0);
    bool aurora = (g_app.backdrop == 1) && IsWindowVisible(g_hwnd);

    UINT want = 250u;
    if (timerAnim || pulse) want = 16u;
    else if (aurora) want = 40u;

    if (want != g_tickMs) {
        g_tickMs = want;
        KillTimer(g_hwnd, TIMER_TICK);
        SetTimer(g_hwnd, TIMER_TICK, g_tickMs, nullptr);
    }
}

static void PlayAlertSound() {
    g_soundRepeats = 2;
    PlaySoundW(L"SystemNotification", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
    SetTimer(g_hwnd, TIMER_SOUND, 900, nullptr);
}

static void ShowBalloon(const std::wstring& title, const std::wstring& body) {
    if (g_nid.hWnd == nullptr) return;
    NOTIFYICONDATAW nid = g_nid;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, body.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

static void OnTimerFinished(const std::wstring& title, const std::wstring& body) {
    g_finishAt = SteadyNow();
    PlayAlertSound();
    ShowBalloon(title, body);
    FLASHWINFO fi{};
    fi.cbSize = sizeof(fi);
    fi.hwnd = g_hwnd;
    fi.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    FlashWindowEx(&fi);
    UpdateKeepAwake();
}

// ------------------------------------------------------------- 外观同步

static void ApplyAppearance() {
    Pal::ApplyTheme(g_app.themeIndex);
    if (g_rr.HasImage() || g_app.backdrop == 2) {
        if (g_app.imagePath.empty()) g_rr.ClearImage();
        else g_rr.LoadImage(g_app.imagePath);
    }
    g_rr.InvalidateTheme();

    if (g_hwnd) {
        BOOL dark = Pal::IsLight(g_app.themeIndex) ? FALSE : TRUE;
        DwmSetWindowAttribute(g_hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
    }
}

static void ApplyTopmost() {
    if (!g_hwnd) return;
    SetWindowPos(g_hwnd, g_app.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void PersistNow() {
    if (!g_hwnd) return;
    RECT rc{};
    GetWindowRect(g_hwnd, &rc);
    SaveSettings(g_app, rc.left, rc.top);
}

// 改动后延迟落盘，避免频繁写文件
static double g_dirtyAt = -1.0;
static double g_runStart = 0.0;

static void MarkDirty() { g_dirtyAt = SteadyNow(); }

// ------------------------------------------------------------- 托盘

static void AddTrayIcon() {
    if (g_nid.hWnd) return;
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_iconBig ? g_iconBig : LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wcsncpy_s(g_nid.szTip, L"计时器", _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void RemoveTrayIcon() {
    if (!g_nid.hWnd) return;
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    g_nid.hWnd = nullptr;
}

static void RestoreFromTray() {
    ShowWindow(g_hwnd, SW_RESTORE);
    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);
    UpdateTickRate();
}

static void ShowTrayMenu() {
    POINT pt{};
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, L"显示窗口");
    AppendMenuW(menu, MF_STRING | (g_app.alwaysOnTop ? MF_CHECKED : MF_UNCHECKED), 3, L"窗口置顶");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 2, L"退出");

    SetForegroundWindow(g_hwnd);
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hwnd, nullptr);
    DestroyMenu(menu);

    if (cmd == 1) RestoreFromTray();
    else if (cmd == 2) {
        g_exiting = true;
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
    }
    else if (cmd == 3) {
        g_app.alwaysOnTop = !g_app.alwaysOnTop;
        ApplyTopmost();
        InvalidateRect(g_hwnd, nullptr, FALSE);
    }
}

// ------------------------------------------------------------- 秒表嘀嗒声

static std::vector<BYTE> g_tickWav;
static std::vector<BYTE> g_tockWav;
static std::vector<BYTE> g_fastWav;
static bool g_tickAlt = false;
static int  g_lastSecond = -1;
static int  g_lastFastPhase = -1;
static const double kFastPeriod = 0.125;   // 快速嗒嗒：每秒 8 声

// 程序化生成一声"嘀嗒"：正弦 + 低频体 + 起音噪声，指数衰减
static std::vector<BYTE> BuildClickWav(double freq, double durSec, double amp) {
    const int rate = 44100;
    const double twoPi = 6.283185307179586;
    int n = static_cast<int>(rate * durSec);

    std::vector<BYTE> out;
    out.reserve(static_cast<size_t>(44 + n * 2));

    auto put32 = [&](DWORD v) { for (int k = 0; k < 4; ++k) out.push_back(static_cast<BYTE>((v >> (8 * k)) & 0xFF)); };
    auto put16 = [&](WORD v) { out.push_back(static_cast<BYTE>(v & 0xFF)); out.push_back(static_cast<BYTE>((v >> 8) & 0xFF)); };
    auto putTag = [&](const char* s) { for (int k = 0; k < 4; ++k) out.push_back(static_cast<BYTE>(s[k])); };

    putTag("RIFF");
    put32(static_cast<DWORD>(36 + n * 2));
    putTag("WAVE");
    putTag("fmt ");
    put32(16);
    put16(1);                              // PCM
    put16(1);                              // 单声道
    put32(static_cast<DWORD>(rate));
    put32(static_cast<DWORD>(rate * 2));   // 字节率
    put16(2);                              // 块对齐
    put16(16);                             // 位深
    putTag("data");
    put32(static_cast<DWORD>(n * 2));

    for (int i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / rate;
        double env = std::exp(-t * 72.0);
        double tone = 0.72 * std::sin(twoPi * freq * t) + 0.28 * std::sin(twoPi * freq * 0.55 * t);
        double noise = ((rand() % 2000) / 1000.0 - 1.0) * std::exp(-t * 320.0) * 0.45;
        double v = (tone * 0.8 + noise) * env * amp;
        v = std::clamp(v, -1.0, 1.0);
        put16(static_cast<WORD>(static_cast<short>(v * 32000.0)));
    }
    return out;
}

static void EnsureTickWavs() {
    if (!g_tickWav.empty()) return;
    g_tickWav = BuildClickWav(1950.0, 0.055, 0.42);
    g_tockWav = BuildClickWav(1340.0, 0.065, 0.38);
    g_fastWav = BuildClickWav(2600.0, 0.028, 0.30);
}

static void PlayTickSound() {
    EnsureTickWavs();
    const std::vector<BYTE>& w = g_tickAlt ? g_tockWav : g_tickWav;
    g_tickAlt = !g_tickAlt;
    PlaySoundW(reinterpret_cast<LPCWSTR>(w.data()), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

static void PlayFastTick() {
    EnsureTickWavs();
    PlaySoundW(reinterpret_cast<LPCWSTR>(g_fastWav.data()), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

static std::wstring g_lastTip = L"计时器";

// 托盘图标悬停提示：显示剩余时间
static void UpdateTrayTip() {
    if (!g_nid.hWnd) return;

    std::wstring tip = L"计时器";
    if (g_app.mode == Mode::Countdown && g_app.cdRunning) {
        tip = L"计时器 · 倒计时 剩余 " + FormatDur(CountdownRemaining(g_app));
    }
    else if (g_app.mode == Mode::Countdown && g_app.cdFinished) {
        tip = L"计时器 · 倒计时已结束";
    }
    else if (g_app.alActive && !g_app.alFinished) {
        tip = L"计时器 · 定时 剩余 " + FormatDur(AlarmRemaining(g_app));
    }
    else if (g_app.swRunning) {
        double e = g_app.swBase + (SteadyNow() - g_app.swStart);
        tip = L"计时器 · 秒表 " + FormatDur(std::floor(e));
    }

    if (tip == g_lastTip) return;
    g_lastTip = tip;

    wcsncpy_s(g_nid.szTip, tip.c_str(), _TRUNCATE);
    NOTIFYICONDATAW nid = g_nid;
    nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// ------------------------------------------------------------- 逻辑

static void StartCountdown() {
    if (g_app.cdTotal <= 0.5) g_app.cdTotal = 3600.0;
    g_app.cdStart = SteadyNow();
    g_app.cdRunning = true;
    g_app.cdFinished = false;
    g_finishAt = -1.0;
    if (g_app.cdBase >= g_app.cdTotal) g_app.cdBase = 0.0;
    UpdateKeepAwake();
}

static void PauseCountdown() {
    if (!g_app.cdRunning) return;
    g_app.cdBase += SteadyNow() - g_app.cdStart;
    g_app.cdRunning = false;
    UpdateKeepAwake();
}

static void ResetCountdown() {
    g_app.cdRunning = false;
    g_app.cdBase = 0.0;
    g_app.cdFinished = false;
    g_app.ring = 1.f;
    g_finishAt = -1.0;
    UpdateKeepAwake();
}

static void SetCountdownTotal(double seconds) {
    g_app.cdTotal = (std::max)(60.0, (std::min)(seconds, 24.0 * 3600.0));
    ResetCountdown();
}

static void ToggleStopwatch() {
    if (g_app.swRunning) {
        g_app.swBase += SteadyNow() - g_app.swStart;
        g_app.swRunning = false;
    }
    else {
        g_app.swStart = SteadyNow();
        g_app.swRunning = true;
        g_finishAt = -1.0;
        g_tickAlt = false;
        g_lastSecond = -1;
        g_lastFastPhase = -1;
    }
    UpdateKeepAwake();
}

static void ResetStopwatch() {
    g_app.swRunning = false;
    g_app.swBase = 0.0;
    g_app.ring = 0.f;
    g_app.swLaps.clear();
    g_app.swLapOpen = false;
    g_app.swLapScroll = 0.f;
    g_tickAlt = false;
    g_lastSecond = -1;
    g_lastFastPhase = -1;
    UpdateKeepAwake();
}

static std::wstring PickImageFile(HWND owner) {
    std::wstring result;
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dlg))) || !dlg)
        return result;

    DWORD opts = 0;
    dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    COMDLG_FILTERSPEC filters[] = {
        { L"图片文件", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp;*.tif;*.tiff" }
    };
    dlg->SetFileTypes(1, filters);
    dlg->SetTitle(L"选择背景图片");

    if (SUCCEEDED(dlg->Show(owner))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item)) && item) {
            PWSTR p = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p)) && p) {
                result = p;
                CoTaskMemFree(p);
            }
            item->Release();
        }
    }
    dlg->Release();
    return result;
}

static void HandleClickInner(int id);

static void HandleClick(int id) {
    HandleClickInner(id);
    if (id != ID_NONE) MarkDirty();
}

static void HandleClickInner(int id) {
    // 设置面板控件（id >= 200）优先判断，避免被时区行的区间吞掉
    if (id >= ID_THEME && id < ID_THEME + kThemeCount) {
        g_app.themeIndex = id - ID_THEME;
        ApplyAppearance();
        PersistNow();
        return;
    }
    if (id >= ID_BACKDROP && id < ID_BACKDROP + 3) {
        g_app.backdrop = id - ID_BACKDROP;
        if (g_app.backdrop == 2 && g_app.imagePath.empty()) {
            std::wstring picked = PickImageFile(g_hwnd);
            if (!picked.empty()) g_app.imagePath = picked;
            else g_app.backdrop = 0;
        }
        ApplyAppearance();
        PersistNow();
        return;
    }
    if (id >= ID_TZ_ROW && id < ID_THEME) {
        int idx = id - ID_TZ_ROW;
        if (idx >= 0 && idx < static_cast<int>(g_zones.size())) {
            g_app.tzIndex = idx;
            g_app.tzOpen = false;
        }
        return;
    }
    if (id >= ID_PRESET && id < ID_PRESET + 6) {
        int i = id - ID_PRESET;
        if (g_app.mode == Mode::Countdown) {
            if (i >= 5) {
                g_app.cdEditing = true;
                g_app.cdRunning = false;
                g_app.cdBase = 0.0;
                g_app.cdFinished = false;
                g_app.cdEditH = static_cast<int>(g_app.cdTotal / 3600.0);
                g_app.cdEditM = static_cast<int>((static_cast<int>(g_app.cdTotal) % 3600) / 60);
                UpdateKeepAwake();
            }
            else {
                g_app.cdEditing = false;
                SetCountdownTotal(kCdPreset[i] * 60.0);
            }
        }
        else if (g_app.mode == Mode::Alarm) {
            if (i >= 4) {
                g_app.alEditing = true;
                int total = static_cast<int>(g_app.alSpan > 0.5 ? g_app.alSpan : 3600.0);
                g_app.alEditH = std::clamp(total / 3600, 0, 23);
                g_app.alEditM = std::clamp((total % 3600) / 60, 0, 59);
            }
            else {
                static const int alPreset[4] = { 15, 30, 60, 120 };
                g_app.alSpan = alPreset[i] * 60.0;
                g_app.alEditing = false;
            }
        }
        return;
    }
    if (id >= ID_EDIT_BASE && id < ID_EDIT_BASE + 4) {
        int k = id - ID_EDIT_BASE;
        if (k == 0) g_app.cdEditH = (g_app.cdEditH + 1) % 24;
        else if (k == 1) g_app.cdEditH = (g_app.cdEditH + 23) % 24;
        else if (k == 2) g_app.cdEditM = (g_app.cdEditM + 1) % 60;
        else g_app.cdEditM = (g_app.cdEditM + 59) % 60;
        return;
    }
    if (id >= ID_ALARM_BASE && id < ID_ALARM_BASE + 4) {
        int k = id - ID_ALARM_BASE;
        if (g_app.alEditing) {
            if (k == 0) g_app.alEditH = (g_app.alEditH + 1) % 24;
            else if (k == 1) g_app.alEditH = (g_app.alEditH + 23) % 24;
            else if (k == 2) g_app.alEditM = (g_app.alEditM + 1) % 60;
            else g_app.alEditM = (g_app.alEditM + 59) % 60;
        }
        else {
            if (k == 0) g_app.alHour = (g_app.alHour + 1) % 24;
            else if (k == 1) g_app.alHour = (g_app.alHour + 23) % 24;
            else if (k == 2) g_app.alMinute = (g_app.alMinute + 1) % 60;
            else g_app.alMinute = (g_app.alMinute + 59) % 60;
        }
        return;
    }
    if (id >= ID_TAB && id < ID_TAB + 4) {
        g_app.mode = static_cast<Mode>(id - ID_TAB);
        g_app.tzOpen = false;
        g_app.cdEditing = false;
        PersistNow();
        return;
    }

    switch (id) {
    case ID_CLOSE_BTN:
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
        break;
    case ID_MIN_BTN:
        ShowWindow(g_hwnd, SW_MINIMIZE);
        UpdateTickRate();
        break;
    case ID_PIN:
        g_app.alwaysOnTop = !g_app.alwaysOnTop;
        ApplyTopmost();
        PersistNow();
        break;
    case ID_GEAR:
        g_app.settingsOpen = !g_app.settingsOpen;
        g_app.tzOpen = false;
        if (g_app.settingsOpen) g_app.hintSeen = true;
        break;

    case ID_SETTINGS_DONE:
        g_app.settingsOpen = false;
        break;
    case ID_TOGGLE_TOP:
        g_app.alwaysOnTop = !g_app.alwaysOnTop;
        ApplyTopmost();
        PersistNow();
        break;
    case ID_TOGGLE_TRAY:
        g_app.closeToTray = !g_app.closeToTray;
        PersistNow();
        break;
    case ID_TOGGLE_SOUND:
        g_app.swSoundMode = (g_app.swSoundMode + 1) % 3;
        g_lastSecond = -1;
        g_lastFastPhase = -1;
        if (g_app.swSoundMode != 0) EnsureTickWavs();
        PersistNow();
        break;
    case ID_PICK_IMAGE: {
        std::wstring picked = PickImageFile(g_hwnd);
        if (!picked.empty()) {
            g_app.imagePath = picked;
            g_app.backdrop = 2;
            ApplyAppearance();
            PersistNow();
        }
        break;
    }
    case ID_THEME + 0: case ID_THEME + 1: case ID_THEME + 2:
    case ID_THEME + 3: case ID_THEME + 4: case ID_THEME + 5:
        break;

    case ID_PRIMARY:
        if (g_app.mode == Mode::Countdown) {
            if (g_app.cdEditing) {
                int totalSec = (g_app.cdEditH * 60 + g_app.cdEditM) * 60;
                if (totalSec < 60) totalSec = 60;
                g_app.cdEditing = false;
                SetCountdownTotal(static_cast<double>(totalSec));
            }
            else if (g_app.cdFinished) { ResetCountdown(); StartCountdown(); }
            else if (g_app.cdRunning) PauseCountdown();
            else StartCountdown();
        }
        else if (g_app.mode == Mode::Stopwatch) {
            ToggleStopwatch();
        }
        else if (g_app.mode == Mode::Alarm) {
            if (g_app.alEditing) {
                int totalSec = (g_app.alEditH * 60 + g_app.alEditM) * 60;
                if (totalSec < 60) totalSec = 60;
                g_app.alSpan = static_cast<double>(totalSec);
                g_app.alEditing = false;
            }
            else {
                if (g_app.alAsTimeOfDay) AlarmSetTimeOfDay(g_app, g_app.alHour, g_app.alMinute);
                else AlarmSetDuration(g_app, g_app.alSpan > 0.5 ? g_app.alSpan : 3600.0);
                g_finishAt = -1.0;
                g_app.ring = 1.f;
                UpdateKeepAwake();
            }
        }
        break;

    case ID_RESET:
        if (g_app.mode == Mode::Countdown) {
            if (g_app.cdEditing) g_app.cdEditing = false;
            else ResetCountdown();
        }
        else if (g_app.mode == Mode::Stopwatch) {
            ResetStopwatch();
        }
        else if (g_app.mode == Mode::Alarm && g_app.alEditing) {
            g_app.alEditing = false;
        }
        break;

    case ID_LAP: {
        double elapsed = g_app.swBase + (g_app.swRunning ? (SteadyNow() - g_app.swStart) : 0.0);
        if (elapsed > 0.005) {
            g_app.swLaps.push_back(elapsed);
            g_app.swLapScroll = 0.f;
        }
        break;
    }
    case ID_LAP_LIST:
        if (!g_app.swLaps.empty()) {
            g_app.swLapOpen = !g_app.swLapOpen;
            if (g_app.swLapOpen) g_app.swLapScroll = 0.f;
        }
        break;

    case ID_CANCEL_ALARM:
        g_app.alActive = false;
        g_app.alFinished = false;
        g_app.ring = 1.f;
        g_finishAt = -1.0;
        UpdateKeepAwake();
        break;

    case ID_ALARM_DUR:
        if (!g_app.alActive) {
            g_app.alAsTimeOfDay = false;
            g_app.alEditing = false;
        }
        break;
    case ID_ALARM_TOD:
        if (!g_app.alActive) {
            g_app.alAsTimeOfDay = true;
            g_app.alEditing = false;
        }
        break;

    case ID_TZ_PREV:
        if (!g_zones.empty()) g_app.tzIndex = (std::max)(0, g_app.tzIndex - 1);
        break;
    case ID_TZ_NEXT:
        if (!g_zones.empty())
            g_app.tzIndex = (std::min)(static_cast<int>(g_zones.size()) - 1, g_app.tzIndex + 1);
        break;
    case ID_TZ_NAME:
        g_app.tzOpen = !g_app.tzOpen;
        if (g_app.tzOpen) {
            float rowH = 32.f;
            float target = g_app.tzIndex * rowH - 96.f;
            g_app.tzScroll = (std::max)(0.f, target);
        }
        break;
    default:
        break;
    }
}

static void TickLogic() {
    float target = g_app.ring;
    if (g_app.mode == Mode::Countdown) {
        if (g_app.cdTotal > 0.01)
            target = static_cast<float>(CountdownRemaining(g_app) / g_app.cdTotal);
        if (g_app.cdFinished) target = 0.f;
    }
    else if (g_app.mode == Mode::Stopwatch) {
        double e = g_app.swBase + (g_app.swRunning ? SteadyNow() - g_app.swStart : 0.0);
        target = static_cast<float>(std::fmod(e, 60.0) / 60.0);
    }
    else if (g_app.mode == Mode::Alarm) {
        if (g_app.alActive && g_app.alSpan > 0.01)
            target = static_cast<float>(AlarmRemaining(g_app) / g_app.alSpan);
        else
            target = 1.f;
        if (g_app.alFinished) target = 0.f;
    }
    target = Clamp01(target);

    bool smooth = g_app.cdRunning || g_app.swRunning || (g_app.alActive && !g_app.alFinished);
    if (smooth) {
        if (std::fabs(target - g_app.ring) > 0.5f) g_app.ring = target;
        else g_app.ring = LerpF(g_app.ring, target, 0.22f);
    }
    else {
        g_app.ring = target;
    }

    // 秒表音效
    if (g_app.swRunning && g_app.swSoundMode != 0) {
        double e = g_app.swBase + (SteadyNow() - g_app.swStart);
        if (g_app.swSoundMode == 2) {
            int phase = static_cast<int>(e / kFastPeriod);
            if (phase != g_lastFastPhase) {
                g_lastFastPhase = phase;
                if (e > 0.02) PlayFastTick();
            }
        }
        else {
            int sec = static_cast<int>(e);
            if (sec != g_lastSecond) {
                g_lastSecond = sec;
                if (sec > 0) PlayTickSound();
            }
        }
    }
    else {
        g_lastSecond = -1;
        g_lastFastPhase = -1;
    }

    if (g_app.cdRunning && CountdownRemaining(g_app) <= 0.0) {
        g_app.cdBase = g_app.cdTotal;
        g_app.cdRunning = false;
        g_app.cdFinished = true;
        g_app.ring = 0.f;
        wchar_t body[128];
        swprintf_s(body, L"设定的 %s 已结束",
                   FormatDur(g_app.cdTotal, g_app.cdTotal >= 3600.0).c_str());
        OnTimerFinished(L"倒计时结束", body);
    }

    if (g_app.alActive && !g_app.alFinished && AlarmRemaining(g_app) <= 0.0) {
        g_app.alFinished = true;
        g_app.ring = 0.f;
        std::wstring body;
        if (g_app.alAsTimeOfDay) {
            wchar_t buf[96];
            swprintf_s(buf, L"已到达设定时刻 %02d:%02d", g_app.alHour, g_app.alMinute);
            body = buf;
        }
        else {
            body = L"设定的定时时长已结束";
        }
        OnTimerFinished(L"定时提醒", body);
    }

    UpdateKeepAwake();
    UpdateTickRate();
    UpdateTrayTip();

    // 延迟落盘
    if (g_dirtyAt > 0.0 && (SteadyNow() - g_dirtyAt) > 0.8) {
        g_dirtyAt = -1.0;
        PersistNow();
    }

    // 首次使用的入口提示，12 秒后自动消失
    if (!g_app.hintSeen && (SteadyNow() - g_runStart) > 12.0) {
        g_app.hintSeen = true;
        MarkDirty();
        InvalidateRect(g_hwnd, nullptr, FALSE);
    }
}

// ------------------------------------------------------------- 绘制

static void Paint() {
    PAINTSTRUCT ps{};
    BeginPaint(g_hwnd, &ps);

    if (g_rr.Begin()) {
        float t = static_cast<float>(SteadyNow());
        g_rr.Backdrop(g_app.winW, g_app.winH, t, g_app.backdrop);
        Layout L = BuildLayout(g_app, g_zones);
        DrawApp(g_rr, g_app, L, g_zones, t);
        g_rr.End();
    }

    EndPaint(g_hwnd, &ps);
}

static void RefreshHover(POINT ptClient) {
    float dx = static_cast<float>(ptClient.x) / Scale();
    float dy = static_cast<float>(ptClient.y) / Scale();
    Layout L = BuildLayout(g_app, g_zones);
    int id = HitTest(L, dx, dy);
    if (id != g_app.hover) {
        g_app.hover = id;
        g_app.hoverSince = SteadyNow();
        InvalidateRect(g_hwnd, nullptr, FALSE);
    }
}

// ------------------------------------------------------------- 窗口过程

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        UINT dpi = GetDpiForWindow(hwnd);
        if (dpi == 0) dpi = 96;
        g_dpiScale = static_cast<float>(dpi) / 96.f;
        g_rr.Init(hwnd, g_dpiScale);
        ApplyWindowSize(hwnd, g_app.winW, g_app.winH);
        ApplyAppearance();
        ApplyTopmost();
        AddTrayIcon();
        SetTimer(hwnd, TIMER_TICK, g_tickMs, nullptr);
        return 0;
    }

    case WM_NCCALCSIZE:
        if (wp == TRUE) return 0;
        break;

    case WM_NCHITTEST: {
        POINT pt{ static_cast<LONG>(GET_X_LPARAM(lp)), static_cast<LONG>(GET_Y_LPARAM(lp)) };
        POINT c = pt;
        ScreenToClient(hwnd, &c);

        RECT rc{};
        GetClientRect(hwnd, &rc);
        const int grip = 6;
        bool left = c.x < grip;
        bool right = c.x >= rc.right - grip;
        bool top = c.y < grip;
        bool bottom = c.y >= rc.bottom - grip;

        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;

        float dx = static_cast<float>(c.x) / Scale();
        float dy = static_cast<float>(c.y) / Scale();
        Layout L = BuildLayout(g_app, g_zones);
        int id = HitTest(L, dx, dy);
        if (id == ID_CLOSE_BTN || id == ID_MIN_BTN || id == ID_PIN || id == ID_GEAR) return HTCLIENT;
        if (id >= ID_TAB && id < ID_TAB + 4) return HTCLIENT;
        if (L.titleBar.Contains(dx, dy)) return HTCAPTION;
        return HTCLIENT;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lp) == HTCLIENT) {
            POINT p{};
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int grip = 6;
            bool left = p.x < grip;
            bool right = p.x >= rc.right - grip;
            bool top = p.y < grip;
            bool bottom = p.y >= rc.bottom - grip;
            LPCWSTR cur = IDC_ARROW;
            if ((top && left) || (bottom && right)) cur = IDC_SIZENWSE;
            else if ((top && right) || (bottom && left)) cur = IDC_SIZENESW;
            else if (left || right) cur = IDC_SIZEWE;
            else if (top || bottom) cur = IDC_SIZENS;
            SetCursor(LoadCursorW(nullptr, cur));
            return TRUE;
        }
        break;
    }

    case WM_GETMINMAXINFO: {
        auto mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize.x = static_cast<LONG>(kMinW * Scale());
        mmi->ptMinTrackSize.y = static_cast<LONG>(kMinH * Scale());
        return 0;
    }

    case WM_PAINT:
        Paint();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_TIMER:
        if (wp == TIMER_TICK) {
            TickLogic();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wp == TIMER_SOUND) {
            if (g_soundRepeats > 0) {
                PlaySoundW(L"SystemNotification", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
                --g_soundRepeats;
            }
            else {
                KillTimer(hwnd, TIMER_SOUND);
            }
        }
        return 0;

    case WM_MOUSEMOVE: {
        POINT p{ static_cast<LONG>(GET_X_LPARAM(lp)), static_cast<LONG>(GET_Y_LPARAM(lp)) };
        RefreshHover(p);
        return 0;
    }

    case WM_MOUSELEAVE:
        if (g_app.hover != 0) {
            g_app.hover = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        POINT p{ static_cast<LONG>(GET_X_LPARAM(lp)), static_cast<LONG>(GET_Y_LPARAM(lp)) };
        Layout L = BuildLayout(g_app, g_zones);
        int id = HitTest(L, p.x / Scale(), p.y / Scale());
        g_app.press = id;
        SetCapture(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP: {
        POINT p{ static_cast<LONG>(GET_X_LPARAM(lp)), static_cast<LONG>(GET_Y_LPARAM(lp)) };
        Layout L = BuildLayout(g_app, g_zones);
        int id = HitTest(L, p.x / Scale(), p.y / Scale());
        int pressed = g_app.press;
        g_app.press = 0;
        if (GetCapture() == hwnd) ReleaseCapture();
        if (id == pressed && id != 0) HandleClick(id);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        POINT p{ static_cast<LONG>(GET_X_LPARAM(lp)), static_cast<LONG>(GET_Y_LPARAM(lp)) };
        ScreenToClient(hwnd, &p);
        float mx = p.x / Scale();
        float my = p.y / Scale();
        int delta = GET_WHEEL_DELTA_WPARAM(wp);

        if (g_app.settingsOpen) break;
        Layout L = BuildLayout(g_app, g_zones);

        if (g_app.mode == Mode::Clock && g_app.tzOpen && L.tzPanel.Contains(mx, my)) {
            g_app.tzScroll -= delta / 120.f * L.tzRowH * 2.f;
            float maxScroll = (std::max)(0.f, static_cast<float>(g_zones.size()) * L.tzRowH - L.tzPanel.H());
            g_app.tzScroll = std::clamp(g_app.tzScroll, 0.f, maxScroll);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (g_app.mode == Mode::Stopwatch && g_app.swLapOpen && L.lapPanel.Contains(mx, my)) {
            g_app.swLapScroll -= delta / 120.f * L.lapRowH * 2.f;
            float bodyH = L.lapPanel.H() - 38.f;
            float maxScroll = (std::max)(0.f, static_cast<float>(g_app.swLaps.size()) * L.lapRowH - bodyH);
            g_app.swLapScroll = std::clamp(g_app.swLapScroll, 0.f, maxScroll);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_KEYDOWN: {
        if (wp == VK_SPACE) {
            if (g_app.settingsOpen) return 0;
            if (g_app.mode == Mode::Alarm && g_app.alActive) HandleClick(ID_CANCEL_ALARM);
            else if (g_app.mode != Mode::Clock) HandleClick(ID_PRIMARY);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wp == 'L' && !g_app.settingsOpen && g_app.mode == Mode::Stopwatch) {
            HandleClick(ID_LAP);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wp == VK_ESCAPE) {
            if (g_app.settingsOpen) g_app.settingsOpen = false;
            else if (g_app.swLapOpen) g_app.swLapOpen = false;
            else if (g_app.tzOpen) g_app.tzOpen = false;
            else if (g_app.cdEditing) g_app.cdEditing = false;
            else if (g_app.alEditing) g_app.alEditing = false;
            else ShowWindow(hwnd, SW_MINIMIZE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_TRAYICON:
        if (lp == WM_LBUTTONDBLCLK || lp == WM_LBUTTONUP) RestoreFromTray();
        else if (lp == WM_RBUTTONUP) ShowTrayMenu();
        return 0;

    case WM_DPICHANGED: {
        UINT dpi = HIWORD(wp);
        g_dpiScale = static_cast<float>(dpi) / 96.f;
        g_rr.Release();
        g_rr.Init(hwnd, g_dpiScale);
        ApplyAppearance();
        ApplyWindowSize(hwnd, g_app.winW, g_app.winH);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_ACTIVATE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) {
            int pxW = LOWORD(lp), pxH = HIWORD(lp);
            if (pxW > 0 && pxH > 0) {
                SyncWindowSizeFromPixels(pxW, pxH);
                g_rr.Resize(static_cast<UINT>(pxW), static_cast<UINT>(pxH));
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        UpdateTickRate();
        return 0;

    case WM_EXITSIZEMOVE: {
        RECT rc{};
        GetWindowRect(hwnd, &rc);
        SaveSettings(g_app, rc.left, rc.top);
        return 0;
    }

    case WM_CLOSE: {
        if (!g_exiting && g_app.closeToTray) {
            PersistNow();
            ShowWindow(hwnd, SW_HIDE);
            ShowBalloon(L"仍在后台计时", L"窗口已收起到托盘，双击托盘图标可恢复");
            UpdateTickRate();
            return 0;
        }

        RECT rc{};
        GetWindowRect(hwnd, &rc);
        SaveSettings(g_app, rc.left, rc.top);
        KillTimer(hwnd, TIMER_TICK);
        KillTimer(hwnd, TIMER_SOUND);
        SetThreadExecutionState(ES_CONTINUOUS);
        RemoveTrayIcon();
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ------------------------------------------------------------- 入口

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    g_inst = hInst;

    // 单实例：已经有一个在跑（可能在托盘里），就把它唤到前台
    HANDLE single = CreateMutexW(nullptr, TRUE, L"Local\\ExamTimer.SingleInstance");
    if (single && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"ExamTimerWindow", nullptr);
        if (existing) {
            ShowWindow(existing, SW_RESTORE);
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
            PostMessageW(existing, WM_ACTIVATE, WA_ACTIVE, 0);
        }
        CloseHandle(single);
        return 0;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    timeBeginPeriod(1);
    g_runStart = SteadyNow();

    LoadSettings(g_app, g_winX, g_winY);
    LoadTimeZones(g_zones);
    if (g_app.tzIndex >= static_cast<int>(g_zones.size())) g_app.tzIndex = 0;

    {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv) {
            for (int i = 1; i < argc; ++i) {
                auto next = [&](double def) -> double {
                    return (i + 1 < argc) ? _wtof(argv[i + 1]) : def;
                };
                auto nextStr = [&](const wchar_t* def) -> std::wstring {
                    return (i + 1 < argc) ? argv[i + 1] : def;
                };

                if (_wcsicmp(argv[i], L"--seconds") == 0) {
                    double s = next(0);
                    if (s >= 1.0 && s <= 86400.0) {
                        g_app.mode = Mode::Countdown;
                        g_app.cdTotal = s;
                        g_app.cdBase = 0.0;
                        g_app.cdFinished = false;
                        g_app.cdStart = SteadyNow();
                        g_app.cdRunning = true;
                    }
                    ++i;
                }
                else if (_wcsicmp(argv[i], L"--theme") == 0) {
                    g_app.themeIndex = std::clamp(static_cast<int>(next(0)), 0, kThemeCount - 1);
                    ++i;
                }
                else if (_wcsicmp(argv[i], L"--backdrop") == 0) {
                    g_app.backdrop = std::clamp(static_cast<int>(next(0)), 0, 2);
                    ++i;
                }
                else if (_wcsicmp(argv[i], L"--image") == 0) {
                    g_app.imagePath = nextStr(L"");
                    ++i;
                }
                else if (_wcsicmp(argv[i], L"--dumptick") == 0) {
                    // 开发用：把生成的嘀嗒声导出为 wav，便于检查
                    std::wstring dir = nextStr(L".");
                    ++i;
                    EnsureTickWavs();
                    auto dump = [&](const std::wstring& name, const std::vector<BYTE>& data) {
                        std::wstring p = dir + L"\\" + name;
                        HANDLE f = CreateFileW(p.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                               FILE_ATTRIBUTE_NORMAL, nullptr);
                        if (f == INVALID_HANDLE_VALUE) return;
                        DWORD written = 0;
                        WriteFile(f, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
                        CloseHandle(f);
                    };
                    dump(L"tick.wav", g_tickWav);
                    dump(L"tock.wav", g_tockWav);
                    dump(L"fast.wav", g_fastWav);
                }
                else if (_wcsicmp(argv[i], L"--size") == 0) {
                    std::wstring spec = nextStr(L"");
                    size_t xp = spec.find_first_of(L"xX");
                    if (xp != std::wstring::npos) {
                        g_app.winW = std::clamp(static_cast<float>(_wtof(spec.substr(0, xp).c_str())), kMinW, 1600.f);
                        g_app.winH = std::clamp(static_cast<float>(_wtof(spec.substr(xp + 1).c_str())), kMinH, 1600.f);
                    }
                    ++i;
                }
            }
            LocalFree(argv);
        }
    }

    Pal::ApplyTheme(g_app.themeIndex);
    g_app.ring = 1.f;

    HICON icon = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                             0, 0, LR_DEFAULTSIZE));
    g_iconBig = icon;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = icon;
    wc.hIconSm = icon;
    wc.lpszClassName = L"ExamTimerWindow";
    if (!RegisterClassExW(&wc)) return 1;

    int w = static_cast<int>(g_app.winW);
    int h = static_cast<int>(g_app.winH);

    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (g_winX != INT_MIN && g_winY != INT_MIN) {
        x = g_winX;
        y = g_winY;
    }
    else {
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        x = (work.right - work.left - w) / 2 + work.left;
        y = (work.bottom - work.top - h) / 2 + work.top;
    }

    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"计时器",
                                WS_POPUP | WS_MINIMIZEBOX,
                                x, y, w, h, nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return 1;

    DWORD corner = 2;  // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetForegroundWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_rr.Release();
    CoUninitialize();
    timeEndPeriod(1);
    return static_cast<int>(msg.wParam);
}
