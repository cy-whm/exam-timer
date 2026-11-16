#pragma once

#include "common.h"

enum TextFmt {
    TF_BODY = 0,
    TF_SMALL,
    TF_TINY,
    TF_LABEL,
    TF_BUTTON,
    TF_TAB,
    TF_TITLE,
    TF_MED,
    TF_COUNT
};

enum Align { AL_CENTER = 0, AL_LEFT = 1, AL_RIGHT = 2 };

class Renderer {
public:
    ~Renderer() { Release(); }

    bool Init(HWND hwnd, float dpiScale);
    void Release();

    bool Resize(UINT pxWidth, UINT pxHeight);

    bool Begin();
    void End();

    // ---- 主题 / 背景 ----
    void InvalidateTheme();
    bool LoadImage(const std::wstring& path);
    void ClearImage();
    bool HasImage() const { return image_ != nullptr; }
    void Backdrop(float w, float h, float timeSec, int mode);

    // ---- 基元 ----
    void FillRect(const RectF& r, const Color& c);
    void FillRoundRect(const RectF& r, float radius, const Color& c);
    void StrokeRoundRect(const RectF& r, float radius, const Color& c, float width);
    void FillCircle(float cx, float cy, float radius, const Color& c);
    void StrokeCircle(float cx, float cy, float radius, float width, const Color& c);
    void RadialGlow(float cx, float cy, float radius, const Color& c);

    // 角度：0 度在正上方，顺时针为正
    void Arc(float cx, float cy, float radius, float fromDeg, float sweepDeg, float width, const Color& c);

    void Text(const std::wstring& s, const RectF& box, int fmt, const Color& c,
              int align = AL_CENTER, bool vcenter = true);
    float Measure(const std::wstring& s, int fmt) const;

    // 固定字号（UI 小字）
    void Digits(const std::wstring& s, float cx, float cy, int fmt, const Color& c, float tracking = 1.0f);

    // 自适应字号（大号计时数字）：size 为字号，thin 使用细体
    void BigDigits(const std::wstring& s, float cx, float cy, float size, const Color& c,
                   float tracking = 1.0f, bool thin = false);
    float BigDigitsWidth(const std::wstring& s, float size, float tracking, bool thin) const;

    void PushClip(const RectF& r);
    void PopClip();

private:
    struct DynFmt {
        float size = 0.f;
        bool thin = false;
        IDWriteTextFormat* fmt = nullptr;
        float cell = 0.f;
    };

    bool CreateDeviceResources();
    void DiscardDeviceResources();
    bool EnsureBackdropBrush(float w, float h);

    DynFmt* FormatFor(float size, bool thin);
    float MeasureWith(IDWriteTextFormat* fmt, const std::wstring& s) const;
    void DrawDigitsWith(IDWriteTextFormat* fmt, float cell, const std::wstring& s,
                        float cx, float cy, const Color& c, float tracking);

    HWND hwnd_ = nullptr;
    float dpiScale_ = 1.f;

    ID2D1Factory* fac_ = nullptr;
    IDWriteFactory* dw_ = nullptr;
    ID2D1HwndRenderTarget* rt_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    ID2D1StrokeStyle* roundCap_ = nullptr;
    IDWriteTextFormat* fmts_[TF_COUNT] = {};

    ID2D1LinearGradientBrush* bg_ = nullptr;
    ID2D1Bitmap* image_ = nullptr;
    std::wstring imagePath_;
    float bgW_ = 0.f, bgH_ = 0.f;
    bool bgDirty_ = true;

    std::vector<DynFmt> dyn_;
    mutable float digitW_[TF_COUNT] = {};
};
