#include "app.h"

#include <cstdio>
#include <ctime>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

// ------------------------------------------------------------ themes

const ThemeColors kThemes[kThemeCount] = {
    // 0 暗夜青
    { L"暗夜青", false,
      { 0.055f, 0.070f, 0.090f, 1.f }, { 0.086f, 0.104f, 0.133f, 1.f },
      { 0.110f, 0.133f, 0.169f, 1.f }, { 0.145f, 0.176f, 0.224f, 1.f },
      { 0.169f, 0.200f, 0.243f, 1.f }, { 0.239f, 0.286f, 0.353f, 1.f },
      { 0.906f, 0.933f, 0.965f, 1.f }, { 0.478f, 0.529f, 0.596f, 1.f }, { 0.353f, 0.400f, 0.463f, 1.f },
      { 0.310f, 0.765f, 0.969f, 1.f }, { 0.176f, 0.529f, 0.808f, 1.f }, { 0.043f, 0.070f, 0.094f, 1.f },
      { 1.000f, 0.420f, 0.420f, 1.f }, { 1.000f, 0.722f, 0.302f, 1.f }, { 0.137f, 0.165f, 0.208f, 1.f } },

    // 1 石墨
    { L"石墨", false,
      { 0.070f, 0.072f, 0.075f, 1.f }, { 0.105f, 0.108f, 0.113f, 1.f },
      { 0.130f, 0.134f, 0.140f, 1.f }, { 0.170f, 0.175f, 0.183f, 1.f },
      { 0.200f, 0.206f, 0.215f, 1.f }, { 0.280f, 0.288f, 0.300f, 1.f },
      { 0.930f, 0.935f, 0.940f, 1.f }, { 0.520f, 0.530f, 0.550f, 1.f }, { 0.380f, 0.390f, 0.410f, 1.f },
      { 0.800f, 0.830f, 0.860f, 1.f }, { 0.550f, 0.580f, 0.620f, 1.f }, { 0.080f, 0.085f, 0.090f, 1.f },
      { 0.950f, 0.420f, 0.420f, 1.f }, { 0.950f, 0.720f, 0.320f, 1.f }, { 0.155f, 0.160f, 0.170f, 1.f } },

    // 2 暖阳
    { L"暖阳", false,
      { 0.086f, 0.065f, 0.043f, 1.f }, { 0.129f, 0.094f, 0.059f, 1.f },
      { 0.160f, 0.118f, 0.075f, 1.f }, { 0.215f, 0.160f, 0.100f, 1.f },
      { 0.250f, 0.190f, 0.120f, 1.f }, { 0.340f, 0.260f, 0.170f, 1.f },
      { 0.980f, 0.940f, 0.890f, 1.f }, { 0.660f, 0.580f, 0.480f, 1.f }, { 0.480f, 0.410f, 0.330f, 1.f },
      { 1.000f, 0.702f, 0.278f, 1.f }, { 0.850f, 0.550f, 0.180f, 1.f }, { 0.100f, 0.060f, 0.020f, 1.f },
      { 1.000f, 0.450f, 0.420f, 1.f }, { 1.000f, 0.850f, 0.350f, 1.f }, { 0.200f, 0.150f, 0.100f, 1.f } },

    // 3 樱粉
    { L"樱粉", false,
      { 0.082f, 0.055f, 0.075f, 1.f }, { 0.122f, 0.082f, 0.110f, 1.f },
      { 0.155f, 0.105f, 0.140f, 1.f }, { 0.210f, 0.145f, 0.190f, 1.f },
      { 0.250f, 0.170f, 0.225f, 1.f }, { 0.340f, 0.240f, 0.310f, 1.f },
      { 0.980f, 0.930f, 0.955f, 1.f }, { 0.660f, 0.560f, 0.620f, 1.f }, { 0.480f, 0.400f, 0.450f, 1.f },
      { 1.000f, 0.561f, 0.694f, 1.f }, { 0.800f, 0.360f, 0.490f, 1.f }, { 0.100f, 0.030f, 0.060f, 1.f },
      { 1.000f, 0.420f, 0.450f, 1.f }, { 1.000f, 0.760f, 0.480f, 1.f }, { 0.200f, 0.140f, 0.180f, 1.f } },

    // 4 森野
    { L"森野", false,
      { 0.043f, 0.075f, 0.058f, 1.f }, { 0.062f, 0.110f, 0.086f, 1.f },
      { 0.080f, 0.140f, 0.108f, 1.f }, { 0.110f, 0.190f, 0.148f, 1.f },
      { 0.140f, 0.240f, 0.190f, 1.f }, { 0.200f, 0.330f, 0.260f, 1.f },
      { 0.910f, 0.970f, 0.930f, 1.f }, { 0.500f, 0.640f, 0.550f, 1.f }, { 0.360f, 0.470f, 0.400f, 1.f },
      { 0.435f, 0.816f, 0.549f, 1.f }, { 0.260f, 0.630f, 0.400f, 1.f }, { 0.030f, 0.090f, 0.050f, 1.f },
      { 1.000f, 0.480f, 0.440f, 1.f }, { 0.980f, 0.800f, 0.340f, 1.f }, { 0.100f, 0.170f, 0.130f, 1.f } },

    // 5 晨光（浅色）
    { L"晨光", true,
      { 0.965f, 0.970f, 0.980f, 1.f }, { 0.900f, 0.915f, 0.936f, 1.f },
      { 1.000f, 1.000f, 1.000f, 1.f }, { 0.925f, 0.936f, 0.951f, 1.f },
      { 0.840f, 0.855f, 0.878f, 1.f }, { 0.720f, 0.740f, 0.780f, 1.f },
      { 0.100f, 0.125f, 0.153f, 1.f }, { 0.415f, 0.455f, 0.510f, 1.f }, { 0.580f, 0.620f, 0.670f, 1.f },
      { 0.176f, 0.529f, 0.808f, 1.f }, { 0.130f, 0.420f, 0.660f, 1.f }, { 1.000f, 1.000f, 1.000f, 1.f },
      { 0.860f, 0.240f, 0.260f, 1.f }, { 0.880f, 0.550f, 0.060f, 1.f }, { 0.860f, 0.880f, 0.900f, 1.f } },
};

namespace Pal {
    Color BgTop, BgBottom, Surface, SurfaceHi, Border, BorderHi;
    Color Text, TextDim, TextMute;
    Color Accent, AccentDeep, OnAccent;
    Color Danger, Warn, Track;

    void ApplyTheme(int index) {
        index = std::clamp(index, 0, kThemeCount - 1);
        const ThemeColors& t = kThemes[index];
        BgTop = t.bgTop; BgBottom = t.bgBottom;
        Surface = t.surface; SurfaceHi = t.surfaceHi;
        Border = t.border; BorderHi = t.borderHi;
        Text = t.text; TextDim = t.textDim; TextMute = t.textMute;
        Accent = t.accent; AccentDeep = t.accentDeep; OnAccent = t.onAccent;
        Danger = t.danger; Warn = t.warn; Track = t.track;
    }

    bool IsLight(int index) { return kThemes[std::clamp(index, 0, kThemeCount - 1)].light; }

    Color AccentOf(int index) { return kThemes[std::clamp(index, 0, kThemeCount - 1)].accent; }
}

// ------------------------------------------------------------ 时间

double SteadyNow() {
    static const auto origin = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - origin).count();
}

double SystemSeconds() {
    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u{};
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return static_cast<double>(u.QuadPart) / 10000000.0;
}

std::wstring FormatDur(double sec, bool alwaysHours) {
    if (sec < 0) sec = 0;
    long long total = static_cast<long long>(sec + 0.5);
    long long h = total / 3600;
    long long m = (total % 3600) / 60;
    long long s = total % 60;

    wchar_t buf[64];
    if (h > 0 || alwaysHours)
        swprintf_s(buf, L"%lld:%02lld:%02lld", h, m, s);
    else
        swprintf_s(buf, L"%02lld:%02lld", m, s);
    return buf;
}

std::wstring FormatHMS(const SYSTEMTIME& st) {
    wchar_t buf[32];
    swprintf_s(buf, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    return buf;
}

std::wstring FormatDateCN(const SYSTEMTIME& st) {
    static const wchar_t* kWeek[] = {
        L"星期日", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"
    };
    int wd = st.wDayOfWeek;
    if (wd < 0 || wd > 6) wd = 0;
    wchar_t buf[96];
    swprintf_s(buf, L"%d年%d月%d日  %s", st.wYear, st.wMonth, st.wDay, kWeek[wd]);
    return buf;
}

void LoadTimeZones(std::vector<TzItem>& out) {
    out.clear();

    TzItem local;
    local.key = L"";
    local.name = L"跟随系统";
    local.offsetMin = 0;
    TIME_ZONE_INFORMATION tzi{};
    GetTimeZoneInformation(&tzi);
    local.offsetMin = -(tzi.Bias + tzi.StandardBias);
    out.push_back(local);

    for (DWORD i = 0;; ++i) {
        DYNAMIC_TIME_ZONE_INFORMATION dtzi{};
        DWORD r = EnumDynamicTimeZoneInformation(i, &dtzi);
        if (r == ERROR_NO_MORE_ITEMS || r != ERROR_SUCCESS) break;

        TzItem it;
        it.key = dtzi.TimeZoneKeyName;
        it.name = dtzi.StandardName;
        it.offsetMin = -(dtzi.Bias + dtzi.StandardBias);
        it.dtzi = dtzi;
        out.push_back(std::move(it));

        if (i > 2000) break;
    }

    std::stable_sort(out.begin() + 1, out.end(), [](const TzItem& a, const TzItem& b) {
        if (a.offsetMin != b.offsetMin) return a.offsetMin < b.offsetMin;
        return a.name < b.name;
    });
}

bool ZoneLocalTime(const std::vector<TzItem>& zones, int index, SYSTEMTIME& out) {
    if (index < 0 || index >= static_cast<int>(zones.size()) || zones[index].key.empty()) {
        GetLocalTime(&out);
        return true;
    }
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    DYNAMIC_TIME_ZONE_INFORMATION dtzi = zones[index].dtzi;
    return SystemTimeToTzSpecificLocalTimeEx(&dtzi, &utc, &out) != FALSE;
}

SYSTEMTIME SystemLocalNow() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    return st;
}

double CountdownRemaining(const AppState& a) {
    double elapsed = a.cdBase + (a.cdRunning ? (SteadyNow() - a.cdStart) : 0.0);
    double rem = a.cdTotal - elapsed;
    return rem > 0 ? rem : 0.0;
}

double AlarmRemaining(const AppState& a) {
    if (!a.alActive) return 0.0;
    double rem = a.alDeadline - SteadyNow();
    return rem > 0 ? rem : 0.0;
}

void AlarmSetDuration(AppState& a, double seconds) {
    a.alAsTimeOfDay = false;
    a.alSpan = seconds;
    a.alDeadline = SteadyNow() + seconds;
    a.alActive = true;
    a.alFinished = false;
}

void AlarmSetTimeOfDay(AppState& a, int hour, int minute) {
    a.alAsTimeOfDay = true;
    SYSTEMTIME now{};
    GetLocalTime(&now);

    SYSTEMTIME target = now;
    target.wHour = static_cast<WORD>(hour);
    target.wMinute = static_cast<WORD>(minute);
    target.wSecond = 0;
    target.wMilliseconds = 0;

    FILETIME fNow{}, fTgt{};
    SystemTimeToFileTime(&now, &fNow);
    SystemTimeToFileTime(&target, &fTgt);

    ULARGE_INTEGER uNow{}, uTgt{};
    uNow.LowPart = fNow.dwLowDateTime;   uNow.HighPart = fNow.dwHighDateTime;
    uTgt.LowPart = fTgt.dwLowDateTime;   uTgt.HighPart = fTgt.dwHighDateTime;

    double delta = (static_cast<double>(uTgt.QuadPart) - static_cast<double>(uNow.QuadPart)) / 10000000.0;
    if (delta <= 1.0) delta += 86400.0;

    a.alSpan = delta;
    a.alDeadline = SteadyNow() + delta;
    a.alActive = true;
    a.alFinished = false;
}

// ------------------------------------------------------------ settings

// 配置优先级：环境变量 > exe 同目录（便携） > %APPDATA%\ExamTimer
static bool PortableIni(wchar_t* buf, size_t n) {
    wchar_t exe[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, exe, MAX_PATH) == 0) return false;
    wchar_t* slash = wcsrchr(exe, L'\\');
    if (!slash) return false;
    *(slash + 1) = 0;

    wchar_t candidate[MAX_PATH]{};
    swprintf_s(candidate, L"%ssettings.ini", exe);

    // 目录可写才用它
    HANDLE h = CreateFileW(candidate, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    CloseHandle(h);

    wcsncpy_s(buf, n, candidate, _TRUNCATE);
    return true;
}

std::wstring SettingsFile() {
    static std::wstring cached;
    if (!cached.empty()) return cached;

    wchar_t env[MAX_PATH]{};
    DWORD got = GetEnvironmentVariableW(L"EXAMTIMER_INI", env, MAX_PATH);
    if (got > 0 && got < MAX_PATH) {
        cached = env;
        return cached;
    }

    wchar_t portable[MAX_PATH]{};
    if (PortableIni(portable, MAX_PATH)) {
        cached = portable;
        return cached;
    }

    wchar_t appdata[MAX_PATH]{};
    DWORD got2 = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
    if (got2 == 0 || got2 >= MAX_PATH) GetTempPathW(MAX_PATH, appdata);
    wchar_t dir[MAX_PATH]{};
    swprintf_s(dir, L"%s\\ExamTimer", appdata);
    CreateDirectoryW(dir, nullptr);
    wchar_t full[MAX_PATH]{};
    swprintf_s(full, L"%s\\settings.ini", dir);
    cached = full;
    return cached;
}

void LoadSettings(AppState& a, int& winX, int& winY) {
    std::wstring pathW = SettingsFile();
    a.settingsFile = pathW;
    const wchar_t* path = pathW.c_str();

    a.mode = static_cast<Mode>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"app", L"mode", 0, path)), 0, 3));

    int total = static_cast<int>(GetPrivateProfileIntW(L"timer", L"cdTotal", 3600, path));
    if (total < 60) total = 60;
    if (total > 24 * 3600) total = 24 * 3600;
    a.cdTotal = static_cast<double>(total);
    a.cdBase = 0.0;

    a.alHour = std::clamp(static_cast<int>(GetPrivateProfileIntW(L"alarm", L"hour", 2, path)), 0, 23);
    a.alMinute = std::clamp(static_cast<int>(GetPrivateProfileIntW(L"alarm", L"minute", 0, path)), 0, 59);
    a.alAsTimeOfDay = GetPrivateProfileIntW(L"alarm", L"asTimeOfDay", 0, path) != 0;

    a.tzIndex = static_cast<int>(GetPrivateProfileIntW(L"clock", L"tzIndex", 0, path));
    if (a.tzIndex < 0) a.tzIndex = 0;

    a.swSoundMode = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"stopwatch", L"sound", 0, path)), 0, 2);

    // 外观
    a.themeIndex = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"look", L"theme", 0, path)), 0, kThemeCount - 1);
    a.backdrop = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"look", L"backdrop", 0, path)), 0, 2);
    a.alwaysOnTop = GetPrivateProfileIntW(L"look", L"topmost", 0, path) != 0;
    a.closeToTray = GetPrivateProfileIntW(L"look", L"closeToTray", 1, path) != 0;

    wchar_t img[MAX_PATH]{};
    GetPrivateProfileStringW(L"look", L"image", L"", img, MAX_PATH, path);
    a.imagePath = img;

    a.hintSeen = GetPrivateProfileIntW(L"look", L"hint", 0, path) != 0;

    // 窗口
    int ww = static_cast<int>(GetPrivateProfileIntW(L"window", L"w", 440, path));
    int wh = static_cast<int>(GetPrivateProfileIntW(L"window", L"h", 600, path));
    a.winW = static_cast<float>(std::clamp(ww, 380, 1600));
    a.winH = static_cast<float>(std::clamp(wh, 520, 1600));

    winX = static_cast<int>(GetPrivateProfileIntW(L"window", L"x", INT_MIN, path));
    winY = static_cast<int>(GetPrivateProfileIntW(L"window", L"y", INT_MIN, path));
}

void SaveSettings(const AppState& a, int winX, int winY) {
    std::wstring pathW = SettingsFile();
    const wchar_t* path = pathW.c_str();

    wchar_t buf[64];
    swprintf_s(buf, L"%d", static_cast<int>(a.mode));
    WritePrivateProfileStringW(L"app", L"mode", buf, path);

    swprintf_s(buf, L"%d", static_cast<int>(a.cdTotal + 0.5));
    WritePrivateProfileStringW(L"timer", L"cdTotal", buf, path);

    swprintf_s(buf, L"%d", a.alHour);
    WritePrivateProfileStringW(L"alarm", L"hour", buf, path);
    swprintf_s(buf, L"%d", a.alMinute);
    WritePrivateProfileStringW(L"alarm", L"minute", buf, path);
    WritePrivateProfileStringW(L"alarm", L"asTimeOfDay", a.alAsTimeOfDay ? L"1" : L"0", path);

    swprintf_s(buf, L"%d", a.tzIndex);
    WritePrivateProfileStringW(L"clock", L"tzIndex", buf, path);

    swprintf_s(buf, L"%d", a.swSoundMode);
    WritePrivateProfileStringW(L"stopwatch", L"sound", buf, path);

    swprintf_s(buf, L"%d", a.themeIndex);
    WritePrivateProfileStringW(L"look", L"theme", buf, path);
    swprintf_s(buf, L"%d", a.backdrop);
    WritePrivateProfileStringW(L"look", L"backdrop", buf, path);
    WritePrivateProfileStringW(L"look", L"topmost", a.alwaysOnTop ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"look", L"closeToTray", a.closeToTray ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"look", L"image", a.imagePath.c_str(), path);
    WritePrivateProfileStringW(L"look", L"hint", a.hintSeen ? L"1" : L"0", path);

    swprintf_s(buf, L"%d", static_cast<int>(a.winW + 0.5f));
    WritePrivateProfileStringW(L"window", L"w", buf, path);
    swprintf_s(buf, L"%d", static_cast<int>(a.winH + 0.5f));
    WritePrivateProfileStringW(L"window", L"h", buf, path);

    swprintf_s(buf, L"%d", winX);
    WritePrivateProfileStringW(L"window", L"x", buf, path);
    swprintf_s(buf, L"%d", winY);
    WritePrivateProfileStringW(L"window", L"y", buf, path);
}
