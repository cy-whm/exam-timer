#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shellapi.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <wincodec.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>

// ---------------------------------------------------------------- color

struct Color {
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;
};

inline constexpr Color C(float r, float g, float b, float a = 1.f) { return Color{ r, g, b, a }; }
inline D2D1_COLOR_F D2(const Color& c) { return D2D1::ColorF(c.r, c.g, c.b, c.a); }
inline Color WithAlpha(const Color& c, float a) { return Color{ c.r, c.g, c.b, a }; }

inline Color LerpColor(const Color& x, const Color& y, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return Color{
        x.r + (y.r - x.r) * t,
        x.g + (y.g - x.g) * t,
        x.b + (y.b - x.b) * t,
        x.a + (y.a - x.a) * t };
}

// ---------------------------------------------------------------- geometry

struct RectF {
    float l = 0.f, t = 0.f, r = 0.f, b = 0.f;

    float W() const { return r - l; }
    float H() const { return b - t; }
    float cx() const { return (l + r) * 0.5f; }
    float cy() const { return (t + b) * 0.5f; }

    bool Contains(float x, float y) const { return x >= l && x < r && y >= t && y < b; }

    RectF Inset(float d) const { return RectF{ l + d, t + d, r - d, b - d }; }
    RectF Offset(float dx, float dy) const { return RectF{ l + dx, t + dy, r + dx, b + dy }; }

    D2D1_RECT_F D2R() const { return D2D1::RectF(l, t, r, b); }
};

inline RectF R(float left, float top, float w, float h) {
    return RectF{ left, top, left + w, top + h };
}

inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
inline float LerpF(float a, float b, float t) { return a + (b - a) * Clamp01(t); }

// ---------------------------------------------------------------- theme

struct ThemeColors {
    const wchar_t* name;
    bool light;
    Color bgTop, bgBottom;
    Color surface, surfaceHi;
    Color border, borderHi;
    Color text, textDim, textMute;
    Color accent, accentDeep, onAccent;
    Color danger, warn, track;
};

constexpr int kThemeCount = 6;
extern const ThemeColors kThemes[kThemeCount];

// 运行期调色板：随主题切换
namespace Pal {
    extern Color BgTop, BgBottom;
    extern Color Surface, SurfaceHi;
    extern Color Border, BorderHi;
    extern Color Text, TextDim, TextMute;
    extern Color Accent, AccentDeep, OnAccent;
    extern Color Danger, Warn, Track;

    void ApplyTheme(int index);
    bool IsLight(int index);
    Color AccentOf(int index);
}
