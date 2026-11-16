#include "render.h"

#include <cstdio>

static const float kPi = 3.14159265358979f;

static DWRITE_FONT_WEIGHT g_weight[TF_COUNT] = {
    DWRITE_FONT_WEIGHT_NORMAL,      // TF_BODY
    DWRITE_FONT_WEIGHT_NORMAL,      // TF_SMALL
    DWRITE_FONT_WEIGHT_NORMAL,      // TF_TINY
    DWRITE_FONT_WEIGHT_SEMI_BOLD,   // TF_LABEL
    DWRITE_FONT_WEIGHT_SEMI_BOLD,   // TF_BUTTON
    DWRITE_FONT_WEIGHT_SEMI_BOLD,   // TF_TAB
    DWRITE_FONT_WEIGHT_SEMI_BOLD,   // TF_TITLE
    DWRITE_FONT_WEIGHT_SEMI_BOLD,   // TF_MED
};

static float g_size[TF_COUNT] = {
    13.0f,   // TF_BODY
    11.5f,   // TF_SMALL
    10.0f,   // TF_TINY
    12.0f,   // TF_LABEL
    14.0f,   // TF_BUTTON
    13.0f,   // TF_TAB
    15.0f,   // TF_TITLE
    38.0f,   // TF_MED
};

bool Renderer::Init(HWND hwnd, float dpiScale) {
    hwnd_ = hwnd;
    dpiScale_ = dpiScale;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &fac_)))
        return false;

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(&dw_))))
        return false;

    for (int i = 0; i < TF_COUNT; ++i) {
        HRESULT hr = dw_->CreateTextFormat(
            L"Segoe UI", nullptr, g_weight[i], DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, g_size[i], L"zh-CN", &fmts_[i]);
        if (FAILED(hr)) return false;
        fmts_[i]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }

    D2D1_STROKE_STYLE_PROPERTIES sp = D2D1::StrokeStyleProperties(
        D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
        D2D1_LINE_JOIN_ROUND, 10.f);
    fac_->CreateStrokeStyle(sp, nullptr, 0, &roundCap_);

    return true;
}

void Renderer::Release() {
    DiscardDeviceResources();
    for (auto& d : dyn_) {
        if (d.fmt) d.fmt->Release();
    }
    dyn_.clear();

    if (roundCap_) { roundCap_->Release(); roundCap_ = nullptr; }
    for (int i = 0; i < TF_COUNT; ++i) {
        if (fmts_[i]) { fmts_[i]->Release(); fmts_[i] = nullptr; }
    }
    if (dw_) { dw_->Release(); dw_ = nullptr; }
    if (fac_) { fac_->Release(); fac_ = nullptr; }
}

void Renderer::DiscardDeviceResources() {
    if (bg_) { bg_->Release(); bg_ = nullptr; }
    if (image_) { image_->Release(); image_ = nullptr; }
    if (brush_) { brush_->Release(); brush_ = nullptr; }
    if (rt_) { rt_->Release(); rt_ = nullptr; }
    bgW_ = bgH_ = 0.f;
    bgDirty_ = true;
    for (int i = 0; i < TF_COUNT; ++i) digitW_[i] = 0.f;
}

void Renderer::InvalidateTheme() {
    if (bg_) { bg_->Release(); bg_ = nullptr; }
    bgW_ = bgH_ = 0.f;
    bgDirty_ = true;
}

bool Renderer::LoadImage(const std::wstring& path) {
    ClearImage();
    imagePath_ = path;
    if (path.empty() || !rt_) return false;

    IWICImagingFactory* wic = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&wic))) || !wic)
        return false;

    IWICBitmapDecoder* dec = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;
    bool ok = false;

    if (SUCCEEDED(wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &dec)) && dec &&
        SUCCEEDED(dec->GetFrame(0, &frame)) && frame &&
        SUCCEEDED(wic->CreateFormatConverter(&conv)) && conv &&
        SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                                   nullptr, 0.f, WICBitmapPaletteTypeCustom))) {
        ok = SUCCEEDED(rt_->CreateBitmapFromWicBitmap(conv, nullptr, &image_)) && image_;
    }

    if (conv) conv->Release();
    if (frame) frame->Release();
    if (dec) dec->Release();
    wic->Release();
    return ok;
}

void Renderer::ClearImage() {
    if (image_) { image_->Release(); image_ = nullptr; }
    imagePath_.clear();
}

bool Renderer::CreateDeviceResources() {
    if (rt_) return true;

    auto props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f * dpiScale_, 96.f * dpiScale_);

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    UINT w = static_cast<UINT>(rc.right - rc.left);
    UINT h = static_cast<UINT>(rc.bottom - rc.top);
    if (w == 0 || h == 0) return false;

    auto hwndProps = D2D1::HwndRenderTargetProperties(hwnd_, D2D1::SizeU(w, h));
    if (FAILED(fac_->CreateHwndRenderTarget(props, hwndProps, &rt_)))
        return false;

    rt_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    rt_->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &brush_);

    bgDirty_ = true;
    if (!imagePath_.empty()) {
        std::wstring keep = imagePath_;
        LoadImage(keep);
    }
    return true;
}

bool Renderer::Resize(UINT pxWidth, UINT pxHeight) {
    if (!CreateDeviceResources()) return false;
    rt_->Resize(D2D1::SizeU(pxWidth, pxHeight));
    bgDirty_ = true;
    return true;
}

bool Renderer::Begin() {
    if (!CreateDeviceResources()) return false;
    rt_->BeginDraw();
    rt_->SetTransform(D2D1::Matrix3x2F::Identity());

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    brush_->SetColor(D2(Pal::BgTop));
    rt_->FillRectangle(
        D2D1::RectF(0.f, 0.f, static_cast<float>(rc.right), static_cast<float>(rc.bottom)),
        brush_);
    return true;
}

void Renderer::End() {
    if (!rt_) return;
    HRESULT hr = rt_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) DiscardDeviceResources();
}

// ------------------------------------------------------------ backdrop

bool Renderer::EnsureBackdropBrush(float w, float h) {
    if (bg_ && !bgDirty_ && bgW_ == w && bgH_ == h) return true;
    if (bg_) { bg_->Release(); bg_ = nullptr; }

    D2D1_GRADIENT_STOP stops[3] = {
        { 0.00f, D2(Pal::BgTop) },
        { 0.45f, D2(LerpColor(Pal::BgTop, Pal::BgBottom, 0.5f)) },
        { 1.00f, D2(Pal::BgBottom) },
    };
    ID2D1GradientStopCollection* coll = nullptr;
    if (FAILED(rt_->CreateGradientStopCollection(stops, 3, &coll)) || !coll) return false;

    auto lp = D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0.f, h));
    HRESULT hr = rt_->CreateLinearGradientBrush(lp, coll, &bg_);
    coll->Release();

    bgW_ = w;
    bgH_ = h;
    bgDirty_ = false;
    return SUCCEEDED(hr) && bg_;
}

void Renderer::Backdrop(float w, float h, float timeSec, int mode) {
    if (!rt_) return;

    if (mode == 2 && image_) {
        // 自定义图片：cover 裁剪 + 暗色蒙版
        D2D1_SIZE_F sz = image_->GetSize();
        if (sz.width > 1.f && sz.height > 1.f) {
            float scale = (std::max)(w / sz.width, h / sz.height);
            float srcW = w / scale;
            float srcH = h / scale;
            D2D1_RECT_F src = D2D1::RectF((sz.width - srcW) * 0.5f, (sz.height - srcH) * 0.5f,
                                          (sz.width + srcW) * 0.5f, (sz.height + srcH) * 0.5f);
            D2D1_RECT_F dst = D2D1::RectF(0.f, 0.f, w, h);
            rt_->DrawBitmap(image_, &dst, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &src);

            D2D1_GRADIENT_STOP stops[2] = {
                { 0.00f, D2(WithAlpha(Pal::BgTop, 0.66f)) },
                { 1.00f, D2(WithAlpha(Pal::BgBottom, 0.84f)) },
            };
            ID2D1GradientStopCollection* coll = nullptr;
            if (SUCCEEDED(rt_->CreateGradientStopCollection(stops, 2, &coll)) && coll) {
                auto lp = D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0.f, h));
                ID2D1LinearGradientBrush* br = nullptr;
                if (SUCCEEDED(rt_->CreateLinearGradientBrush(lp, coll, &br)) && br) {
                    rt_->FillRectangle(D2D1::RectF(0.f, 0.f, w, h), br);
                    br->Release();
                }
                coll->Release();
            }
        }
        return;
    }

    if (!EnsureBackdropBrush(w, h)) return;
    rt_->FillRectangle(D2D1::RectF(0.f, 0.f, w, h), bg_);

    if (mode == 1) {
        // 极光：三团缓慢漂移的光晕
        float base = (std::min)(w, h);
        for (int i = 0; i < 3; ++i) {
            float ph = i * 2.1f;
            float cx = w * (0.5f + 0.36f * sinf(timeSec * 0.10f + ph));
            float cy = h * (0.42f + 0.26f * cosf(timeSec * 0.13f + ph * 1.7f));
            float rad = base * (0.72f + 0.10f * sinf(timeSec * 0.07f + ph));
            Color col = (i == 1) ? Pal::AccentDeep : Pal::Accent;
            RadialGlow(cx, cy, rad, WithAlpha(col, 0.34f - i * 0.08f));
        }
    }
}

// ------------------------------------------------------------ primitives

void Renderer::FillRect(const RectF& r, const Color& c) {
    if (!rt_) return;
    brush_->SetColor(D2(c));
    rt_->FillRectangle(r.D2R(), brush_);
}

void Renderer::FillRoundRect(const RectF& r, float radius, const Color& c) {
    if (!rt_) return;
    brush_->SetColor(D2(c));
    rt_->FillRoundedRectangle(D2D1::RoundedRect(r.D2R(), radius, radius), brush_);
}

void Renderer::StrokeRoundRect(const RectF& r, float radius, const Color& c, float width) {
    if (!rt_) return;
    brush_->SetColor(D2(c));
    rt_->DrawRoundedRectangle(D2D1::RoundedRect(r.D2R(), radius, radius), brush_, width);
}

void Renderer::FillCircle(float cx, float cy, float radius, const Color& c) {
    if (!rt_) return;
    brush_->SetColor(D2(c));
    rt_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), brush_);
}

void Renderer::StrokeCircle(float cx, float cy, float radius, float width, const Color& c) {
    if (!rt_) return;
    brush_->SetColor(D2(c));
    rt_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), brush_, width, roundCap_);
}

void Renderer::RadialGlow(float cx, float cy, float radius, const Color& c) {
    if (!rt_) return;
    D2D1_GRADIENT_STOP stops[3] = {
        { 0.00f, D2(WithAlpha(c, c.a)) },
        { 0.55f, D2(WithAlpha(c, c.a * 0.35f)) },
        { 1.00f, D2(WithAlpha(c, 0.f)) },
    };
    ID2D1GradientStopCollection* coll = nullptr;
    if (FAILED(rt_->CreateGradientStopCollection(stops, 3, &coll)) || !coll) return;

    ID2D1RadialGradientBrush* rg = nullptr;
    auto rp = D2D1::RadialGradientBrushProperties(
        D2D1::Point2F(cx, cy), D2D1::Point2F(0.f, 0.f), radius, radius);
    if (SUCCEEDED(rt_->CreateRadialGradientBrush(rp, coll, &rg)) && rg) {
        rt_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), rg);
        rg->Release();
    }
    coll->Release();
}

void Renderer::Arc(float cx, float cy, float radius, float fromDeg, float sweepDeg,
                   float width, const Color& c) {
    if (!rt_ || radius <= 0.f) return;
    if (sweepDeg <= 0.15f) return;

    brush_->SetColor(D2(c));

    if (sweepDeg >= 359.8f) {
        rt_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), brush_, width,
                         roundCap_);
        return;
    }

    auto pointAt = [&](float deg) {
        float rad = (deg - 90.f) * kPi / 180.f;
        return D2D1::Point2F(cx + radius * cosf(rad), cy + radius * sinf(rad));
    };

    ID2D1PathGeometry* geo = nullptr;
    if (FAILED(fac_->CreatePathGeometry(&geo)) || !geo) return;

    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(geo->Open(&sink)) || !sink) { geo->Release(); return; }

    sink->BeginFigure(pointAt(fromDeg), D2D1_FIGURE_BEGIN_HOLLOW);
    D2D1_ARC_SEGMENT seg{};
    seg.point = pointAt(fromDeg + sweepDeg);
    seg.size = D2D1::SizeF(radius, radius);
    seg.rotationAngle = 0.f;
    seg.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
    seg.arcSize = (sweepDeg > 180.f) ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
    sink->AddArc(seg);
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();
    sink->Release();

    rt_->DrawGeometry(geo, brush_, width, roundCap_);
    geo->Release();
}

// ------------------------------------------------------------ text

float Renderer::MeasureWith(IDWriteTextFormat* fmt, const std::wstring& s) const {
    if (!dw_ || !fmt || s.empty()) return 0.f;
    IDWriteTextLayout* layout = nullptr;
    float w = 0.f;
    if (SUCCEEDED(dw_->CreateTextLayout(s.c_str(), static_cast<UINT32>(s.size()), fmt,
                                        8192.f, 8192.f, &layout)) && layout) {
        DWRITE_TEXT_METRICS m{};
        if (SUCCEEDED(layout->GetMetrics(&m))) w = m.widthIncludingTrailingWhitespace;
        layout->Release();
    }
    return w;
}

float Renderer::Measure(const std::wstring& s, int fmt) const {
    if (fmt < 0 || fmt >= TF_COUNT) return 0.f;
    return MeasureWith(fmts_[fmt], s);
}

Renderer::DynFmt* Renderer::FormatFor(float size, bool thin) {
    size = std::clamp(size, 8.f, 240.f);
    for (auto& d : dyn_) {
        if (d.thin == thin && std::fabs(d.size - size) < 0.35f) return &d;
    }

    DynFmt d;
    d.size = size;
    d.thin = thin;
    HRESULT hr = dw_->CreateTextFormat(
        L"Segoe UI", nullptr,
        thin ? DWRITE_FONT_WEIGHT_LIGHT : DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"zh-CN", &d.fmt);
    if (FAILED(hr) || !d.fmt) return nullptr;
    d.fmt->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    float mx = 0.f;
    for (wchar_t ch = L'0'; ch <= L'9'; ++ch) {
        mx = (std::max)(mx, MeasureWith(d.fmt, std::wstring(1, ch)));
    }
    d.cell = mx;

    dyn_.push_back(d);
    return &dyn_.back();
}

static float CharCell(wchar_t ch, float digitCell) {
    if (ch >= L'0' && ch <= L'9') return digitCell;
    if (ch == L':') return digitCell * 0.46f;
    if (ch == L'.') return digitCell * 0.34f;
    return digitCell * 0.52f;
}

void Renderer::DrawDigitsWith(IDWriteTextFormat* fmt, float cell, const std::wstring& s,
                              float cx, float cy, const Color& c, float tracking) {
    if (!fmt || s.empty() || cell <= 0.f) return;

    size_t n = s.size();
    std::vector<float> widths(n);
    float total = 0.f;
    for (size_t i = 0; i < n; ++i) {
        widths[i] = CharCell(s[i], cell) + tracking;
        total += widths[i];
    }
    total -= tracking;

    float x = cx - total * 0.5f;
    float size = fmt->GetFontSize();
    float boxH = size * 1.55f;

    fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    brush_->SetColor(D2(c));

    for (size_t i = 0; i < n; ++i) {
        wchar_t ch = s[i];
        if (ch == L' ') { x += widths[i]; continue; }
        D2D1_RECT_F box = D2D1::RectF(x, cy - boxH * 0.5f, x + widths[i], cy + boxH * 0.5f);
        rt_->DrawTextW(&ch, 1, fmt, box, brush_, D2D1_DRAW_TEXT_OPTIONS_NONE,
                       DWRITE_MEASURING_MODE_NATURAL);
        x += widths[i];
    }
}

void Renderer::Digits(const std::wstring& s, float cx, float cy, int fmt, const Color& c,
                      float tracking) {
    if (!rt_ || fmt < 0 || fmt >= TF_COUNT || !fmts_[fmt] || s.empty()) return;
    if (digitW_[fmt] <= 0.f) {
        float mx = 0.f;
        for (wchar_t ch = L'0'; ch <= L'9'; ++ch)
            mx = (std::max)(mx, MeasureWith(fmts_[fmt], std::wstring(1, ch)));
        digitW_[fmt] = mx;
    }
    DrawDigitsWith(fmts_[fmt], digitW_[fmt], s, cx, cy, c, tracking);
}

void Renderer::BigDigits(const std::wstring& s, float cx, float cy, float size, const Color& c,
                         float tracking, bool thin) {
    if (!rt_ || s.empty()) return;
    DynFmt* d = FormatFor(size, thin);
    if (!d) return;
    DrawDigitsWith(d->fmt, d->cell, s, cx, cy, c, tracking);
}

float Renderer::BigDigitsWidth(const std::wstring& s, float size, float tracking, bool thin) const {
    if (s.empty()) return 0.f;
    float cell = size * 0.56f;
    for (const auto& d : dyn_) {
        if (d.thin == thin && std::fabs(d.size - std::clamp(size, 8.f, 240.f)) < 0.35f) {
            cell = d.cell;
            break;
        }
    }
    float total = 0.f;
    for (size_t i = 0; i < s.size(); ++i) total += CharCell(s[i], cell) + tracking;
    return total > 0.f ? total - tracking : 0.f;
}

void Renderer::Text(const std::wstring& s, const RectF& box, int fmt, const Color& c,
                    int align, bool vcenter) {
    if (!rt_ || fmt < 0 || fmt >= TF_COUNT || !fmts_[fmt] || s.empty()) return;

    fmts_[fmt]->SetTextAlignment(align == AL_CENTER ? DWRITE_TEXT_ALIGNMENT_CENTER
                                : align == AL_RIGHT ? DWRITE_TEXT_ALIGNMENT_TRAILING
                                                    : DWRITE_TEXT_ALIGNMENT_LEADING);
    fmts_[fmt]->SetParagraphAlignment(vcenter ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER
                                              : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    brush_->SetColor(D2(c));
    rt_->DrawTextW(s.c_str(), static_cast<UINT32>(s.size()), fmts_[fmt], box.D2R(), brush_,
                   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
}

void Renderer::PushClip(const RectF& r) {
    if (!rt_) return;
    rt_->PushAxisAlignedClip(r.D2R(), D2D1_ANTIALIAS_MODE_ALIASED);
}

void Renderer::PopClip() {
    if (!rt_) return;
    rt_->PopAxisAlignedClip();
}
