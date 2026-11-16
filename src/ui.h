#pragma once

#include "app.h"
#include "common.h"
#include "render.h"

enum {
    ID_NONE = 0,
    ID_TAB = 10,          // +0..3
    ID_GEAR = 20,
    ID_PIN = 21,
    ID_MIN_BTN = 22,
    ID_CLOSE_BTN = 23,
    ID_PRIMARY = 30,
    ID_RESET = 31,
    ID_PRESET = 40,       // +0..5   (5 = 自定义)
    ID_EDIT_BASE = 48,    // +0..3   (小时上/下, 分钟上/下)
    ID_ALARM_BASE = 62,   // +0..3   (定时时刻编辑)
    ID_ALARM_DUR = 70,
    ID_ALARM_TOD = 71,
    ID_CANCEL_ALARM = 72,
    ID_TZ_PREV = 80,
    ID_TZ_NAME = 81,
    ID_TZ_NEXT = 82,
    ID_TZ_ROW = 100,      // +index
    ID_THEME = 200,       // +0..5
    ID_BACKDROP = 210,    // +0..2
    ID_PICK_IMAGE = 220,
    ID_TOGGLE_TOP = 230,
    ID_TOGGLE_TRAY = 231,
    ID_TOGGLE_SOUND = 232,
    ID_LAP = 233,
    ID_LAP_LIST = 234,
    ID_SETTINGS_DONE = 240,
};

struct Hit {
    int id;
    RectF r;
};

struct Layout {
    float W = 440.f;
    float H = 600.f;

    RectF titleBar;
    RectF btnGear, btnPin, btnMin, btnClose;
    RectF tabs[4];

    RectF ringBounds;
    float ringCx = 220.f;
    float ringCy = 268.f;
    float ringRadius = 120.f;
    float ringStroke = 11.f;
    float bigFont = 74.f;
    float centerCy = 262.f;

    RectF primary, reset;
    RectF soundBtn;          // 秒表音效按钮（仅秒表模式有）
    RectF lapBtn, lapListBtn, lapPanel;
    int   lapVisibleRows = 0;
    float lapRowH = 28.f;
    RectF presets[6];
    int   presetCount = 0;

    // 时间编辑（倒计时自定义 / 定时指定时刻共用几何）
    RectF hourUp, hourDown, minUp, minDown;
    float editHCx = 0.f, editMCx = 0.f;

    RectF alarmDur, alarmTod;

    RectF clockTime, clockDate, tzPill, tzPrev, tzName, tzNext, tzPanel;
    int   tzVisibleRows = 0;
    float tzRowH = 32.f;

    // 设置面板
    RectF settingsPanel, settingsDone, settingsTitle;
    RectF themeSwatch[kThemeCount];
    RectF themeName;
    RectF backdropChip[3];
    RectF pickImage, imageName;
    RectF toggleTop, toggleTray;

    RectF footer;

    std::vector<Hit> hits;
};

extern const int kCdPreset[5];   // 分钟

Layout BuildLayout(const AppState& a, const std::vector<TzItem>& zones);
void   DrawApp(Renderer& rr, const AppState& a, const Layout& L,
               const std::vector<TzItem>& zones, float timeSec);
int    HitTest(const Layout& L, float x, float y);
