#pragma once

#include "common.h"

enum class Mode { Countdown = 0, Stopwatch = 1, Alarm = 2, Clock = 3 };
enum class Backdrop { Gradient = 0, Aurora = 1, Image = 2 };

struct TzItem {
    std::wstring key;
    std::wstring name;
    int offsetMin = 0;
    DYNAMIC_TIME_ZONE_INFORMATION dtzi{};
};

struct AppState {
    Mode mode = Mode::Countdown;

    // ---- countdown (单调时钟驱动) ----
    double cdTotal = 3600.0;
    double cdBase = 0.0;
    bool   cdRunning = false;
    bool   cdFinished = false;
    double cdStart = 0.0;

    // 自定义时长编辑
    bool   cdEditing = false;
    int    cdEditH = 1;
    int    cdEditM = 0;

    // ---- stopwatch ----
    double swBase = 0.0;
    bool   swRunning = false;
    double swStart = 0.0;
    bool   swSound = false;          // 已废弃，保留兼容
    int    swSoundMode = 0;          // 0=关 1=嘀嗒(tick-tock) 2=快速嗒嗒
    std::vector<double> swLaps;      // 每次计次时的累计秒数
    bool   swLapOpen = false;        // 计次记录面板
    float  swLapScroll = 0.f;

    // ---- alarm ----
    bool   alActive = false;
    bool   alFinished = false;
    double alDeadline = 0.0;
    double alSpan = 0.0;
    int    alHour = 2;
    int    alMinute = 0;
    bool   alAsTimeOfDay = false;
    bool   alEditing = false;        // 自定义时长编辑
    int    alEditH = 1;
    int    alEditM = 0;

    // ---- clock ----
    int    tzIndex = 0;
    bool   tzOpen = false;
    float  tzScroll = 0.f;

    // ---- 外观 / 行为 ----
    int    themeIndex = 0;
    int    backdrop = 0;              // Backdrop
    std::wstring imagePath;
    bool   alwaysOnTop = false;
    bool   closeToTray = true;

    // ---- 窗口 ----
    float  winW = 440.f;
    float  winH = 600.f;

    // ---- ui ----
    bool   settingsOpen = false;
    bool   hintSeen = false;          // 是否已看过"设置入口"提示
    int    hover = 0;
    int    press = 0;
    double hoverSince = 0.0;
    float  ring = 0.f;
    float  pulse = 0.f;
    std::wstring settingsFile;        // 实际使用的配置文件路径
};

// 单调时钟（不受系统校时 / 夏令时影响）
double SteadyNow();

// 系统墙钟时间，秒（自 1601-01-01 UTC）
double SystemSeconds();

std::wstring FormatDur(double sec, bool alwaysHours = false);
std::wstring FormatHMS(const SYSTEMTIME& st);
std::wstring FormatDateCN(const SYSTEMTIME& st);

void LoadTimeZones(std::vector<TzItem>& out);
bool ZoneLocalTime(const std::vector<TzItem>& zones, int index, SYSTEMTIME& out);
SYSTEMTIME SystemLocalNow();

double CountdownRemaining(const AppState& a);
double AlarmRemaining(const AppState& a);

void AlarmSetDuration(AppState& a, double seconds);
void AlarmSetTimeOfDay(AppState& a, int hour, int minute);

// 配置持久化
std::wstring SettingsFile();
void LoadSettings(AppState& a, int& winX, int& winY);
void SaveSettings(const AppState& a, int winX, int winY);
