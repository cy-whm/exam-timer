#include "ui.h"

#include <cwchar>

const int kCdPreset[5] = { 25, 45, 60, 90, 120 };   // 分钟
static const int kAlPreset[4] = { 15, 30, 60, 120 }; // 分钟

// ------------------------------------------------------------------ 工具

static void AddHit(Layout& L, int id, const RectF& r) {
    L.hits.push_back(Hit{ id, r });
}

static float EditorFont(const Layout& L) {
    return std::clamp(L.ringRadius * 0.62f, 30.f, 110.f);
}

static float FitFont(const Layout& L, size_t chars) {
    float availW = 2.f * (L.ringRadius - L.ringStroke) * 0.86f;
    float s = availW / (static_cast<float>((std::max)(size_t(1), chars)) * 0.56f);
    s = (std::min)(s, L.ringRadius * 1.15f);
    return std::clamp(s, 16.f, 200.f);
}

static void LayoutTimeEditor(Layout& L, float cx, float cy, float fs) {
    const float aw = 44.f, ah = 26.f;
    L.editHCx = cx - fs * 0.78f;
    L.editMCx = cx + fs * 0.78f;
    float upY = cy - fs * 0.66f - ah * 0.5f;
    float downY = cy + fs * 0.66f - ah * 0.5f;
    L.hourUp = R(L.editHCx - aw * 0.5f, upY, aw, ah);
    L.hourDown = R(L.editHCx - aw * 0.5f, downY, aw, ah);
    L.minUp = R(L.editMCx - aw * 0.5f, upY, aw, ah);
    L.minDown = R(L.editMCx - aw * 0.5f, downY, aw, ah);
}

// ------------------------------------------------------------------ 布局

Layout BuildLayout(const AppState& a, const std::vector<TzItem>& zones) {
    Layout L;
    L.W = a.winW;
    L.H = a.winH;

    const float pad = 22.f;
    const float btnS = 26.f, btnY = 10.f, btnGap = 34.f;
    float bx = L.W - pad - btnS;
    L.btnClose = R(bx, btnY, btnS, btnS); bx -= btnGap;
    L.btnMin = R(bx, btnY, btnS, btnS); bx -= btnGap;
    L.btnPin = R(bx, btnY, btnS, btnS); bx -= btnGap;
    L.btnGear = R(bx, btnY, btnS, btnS);
    L.titleBar = R(0.f, 0.f, L.W, 46.f);

    const float tabW = 82.f, tabH = 34.f, tabGap = 6.f;
    float tabsW = 4.f * tabW + 3.f * tabGap;
    float tl = (L.W - tabsW) * 0.5f;
    const float tabY = 56.f;
    for (int i = 0; i < 4; ++i) L.tabs[i] = R(tl + i * (tabW + tabGap), tabY, tabW, tabH);

    L.footer = R(0.f, L.H - 30.f, L.W, 20.f);

    const float primaryH = 48.f;
    float primaryY = L.H - 126.f;
    const float sideW = 48.f, gapB = 12.f, primaryW = 176.f;
    float groupW = sideW + gapB + primaryW + gapB + sideW;   // 296
    float gl = (L.W - groupW) * 0.5f;
    L.soundBtn = R(gl, primaryY, sideW, sideW);
    L.primary = R(gl + sideW + gapB, primaryY, primaryW, primaryH);
    L.reset = R(gl + sideW + gapB + primaryW + gapB, primaryY, sideW, sideW);

    float presetsY = primaryY - 54.f;
    float contentTop = tabY + tabH + 14.f;

    // ---- 内容区 ----
    if (a.settingsOpen) {
        L.settingsPanel = R(pad, 52.f, L.W - pad * 2.f, L.H - 52.f - 16.f);

        float px = L.settingsPanel.l + 18.f;
        float pw = L.settingsPanel.W() - 36.f;
        float y = L.settingsPanel.t + 14.f;

        L.settingsTitle = R(px, y, pw, 24.f);
        y += 34.f;

        // 主题色块
        const float sw = 30.f, sgap = 12.f;
        float rowW = kThemeCount * sw + (kThemeCount - 1) * sgap;
        float sx = L.settingsPanel.cx() - rowW * 0.5f;
        y += 18.f;  // 主题标题
        for (int i = 0; i < kThemeCount; ++i)
            L.themeSwatch[i] = R(sx + i * (sw + sgap), y, sw, sw);
        y += sw + 8.f;
        L.themeName = R(px, y, pw, 18.f);
        y += 26.f;

        // 背景
        y += 18.f;
        const float bw = 92.f, bgap = 10.f;
        float bRowW = 3.f * bw + 2.f * bgap;
        float bx2 = L.settingsPanel.cx() - bRowW * 0.5f;
        for (int i = 0; i < 3; ++i) L.backdropChip[i] = R(bx2 + i * (bw + bgap), y, bw, 32.f);
        y += 42.f;

        L.pickImage = R(px, y, 116.f, 30.f);
        L.imageName = R(L.pickImage.r + 10.f, y, pw - 126.f, 30.f);
        y += 40.f;

        L.toggleTop = R(px, y, pw, 34.f);
        y += 42.f;
        L.toggleTray = R(px, y, pw, 34.f);

        L.settingsDone = R(L.settingsPanel.cx() - 70.f, L.settingsPanel.b - 56.f, 140.f, 44.f);

        AddHit(L, ID_CLOSE_BTN, L.btnClose);
        AddHit(L, ID_MIN_BTN, L.btnMin);
        AddHit(L, ID_PIN, L.btnPin);
        AddHit(L, ID_GEAR, L.btnGear);
        for (int i = 0; i < kThemeCount; ++i) AddHit(L, ID_THEME + i, L.themeSwatch[i]);
        for (int i = 0; i < 3; ++i) AddHit(L, ID_BACKDROP + i, L.backdropChip[i]);
        AddHit(L, ID_PICK_IMAGE, L.pickImage);
        AddHit(L, ID_TOGGLE_TOP, L.toggleTop);
        AddHit(L, ID_TOGGLE_TRAY, L.toggleTray);
        AddHit(L, ID_SETTINGS_DONE, L.settingsDone);
        return L;
    }

    // ---- 圆环区域 ----
    float regionBottom = presetsY - 16.f;
    if (a.mode == Mode::Stopwatch) regionBottom = primaryY - 18.f;
    float regionTop = contentTop;
    if (a.mode == Mode::Alarm) regionTop = 150.f;
    if (a.mode == Mode::Countdown && a.cdEditing) regionTop = contentTop;

    float regionH = (std::max)(120.f, regionBottom - regionTop);
    L.ringCx = L.W * 0.5f;
    L.ringCy = regionTop + regionH * 0.5f;
    L.ringRadius = (std::min)(regionH * 0.5f - 4.f, L.W * 0.5f - 54.f);
    L.ringRadius = (std::max)(L.ringRadius, 54.f);
    L.ringStroke = std::clamp(L.ringRadius * 0.09f, 7.f, 14.f);
    L.ringBounds = RectF{ L.ringCx - L.ringRadius, L.ringCy - L.ringRadius,
                          L.ringCx + L.ringRadius, L.ringCy + L.ringRadius };
    L.centerCy = L.ringCy;

    // ---- 交互区 ----
    AddHit(L, ID_CLOSE_BTN, L.btnClose);
    AddHit(L, ID_MIN_BTN, L.btnMin);
    AddHit(L, ID_PIN, L.btnPin);
    AddHit(L, ID_GEAR, L.btnGear);
    for (int i = 0; i < 4; ++i) AddHit(L, ID_TAB + i, L.tabs[i]);

    if (a.mode == Mode::Countdown) {
        const float cg = 8.f;
        float cw = (L.W - 56.f - 5.f * cg) / 6.f;
        float total = 6.f * cw + 5.f * cg;
        float left = (L.W - total) * 0.5f;
        L.presetCount = 6;
        for (int i = 0; i < 6; ++i) {
            L.presets[i] = R(left + i * (cw + cg), presetsY, cw, 34.f);
            AddHit(L, ID_PRESET + i, L.presets[i]);
        }

        if (a.cdEditing) {
            float fs = EditorFont(L);
            LayoutTimeEditor(L, L.ringCx, L.ringCy, fs);
            AddHit(L, ID_EDIT_BASE + 0, L.hourUp);
            AddHit(L, ID_EDIT_BASE + 1, L.hourDown);
            AddHit(L, ID_EDIT_BASE + 2, L.minUp);
            AddHit(L, ID_EDIT_BASE + 3, L.minDown);
        }
        AddHit(L, ID_PRIMARY, L.primary);
        AddHit(L, ID_RESET, L.reset);
    }
    else if (a.mode == Mode::Stopwatch) {
        AddHit(L, ID_TOGGLE_SOUND, L.soundBtn);
        AddHit(L, ID_PRIMARY, L.primary);
        AddHit(L, ID_RESET, L.reset);

        // 计次 / 记录
        const float cw = 106.f, cg = 10.f;
        float total = cw * 2.f + cg;
        float left = (L.W - total) * 0.5f;
        L.lapBtn = R(left, presetsY, cw, 34.f);
        L.lapListBtn = R(left + cw + cg, presetsY, cw, 34.f);
        AddHit(L, ID_LAP, L.lapBtn);
        if (!a.swLaps.empty()) AddHit(L, ID_LAP_LIST, L.lapListBtn);

        if (a.swLapOpen && !a.swLaps.empty()) {
            L.lapRowH = 28.f;
            float availW = L.ringBounds.W() - 20.f;
            float availH = L.ringBounds.H() - 20.f;
            float rows = static_cast<float>(a.swLaps.size()) * L.lapRowH;
            float h = (std::min)(availH, 40.f + rows + 8.f);
            L.lapPanel = R(L.ringBounds.cx() - availW * 0.5f, L.ringBounds.cy() - h * 0.5f,
                           availW, h);
            float bodyH = L.lapPanel.H() - 40.f;
            L.lapVisibleRows = (std::max)(1, static_cast<int>(bodyH / L.lapRowH));
        }
    }
    else if (a.mode == Mode::Alarm) {
        L.alarmDur = R(L.W * 0.5f - 100.f, 104.f, 96.f, 32.f);
        L.alarmTod = R(L.alarmDur.r + 8.f, 104.f, 96.f, 32.f);
        AddHit(L, ID_ALARM_DUR, L.alarmDur);
        AddHit(L, ID_ALARM_TOD, L.alarmTod);

        if (!a.alActive) {
            if (!a.alAsTimeOfDay) {
                L.presetCount = 5;
                const float cg = 8.f;
                float cw = (L.W - 56.f - 4.f * cg) / 5.f;
                float total = 5.f * cw + 4.f * cg;
                float left = (L.W - total) * 0.5f;
                for (int i = 0; i < 5; ++i) {
                    L.presets[i] = R(left + i * (cw + cg), presetsY, cw, 34.f);
                    AddHit(L, ID_PRESET + i, L.presets[i]);
                }
            }
            if (a.alAsTimeOfDay || a.alEditing) {
                float fs = EditorFont(L);
                LayoutTimeEditor(L, L.ringCx, L.ringCy, fs);
                AddHit(L, ID_ALARM_BASE + 0, L.hourUp);
                AddHit(L, ID_ALARM_BASE + 1, L.hourDown);
                AddHit(L, ID_ALARM_BASE + 2, L.minUp);
                AddHit(L, ID_ALARM_BASE + 3, L.minDown);
            }
            AddHit(L, ID_PRIMARY, L.primary);
            if (a.alEditing) AddHit(L, ID_RESET, L.reset);
        }
        else {
            AddHit(L, ID_CANCEL_ALARM, L.primary);
        }
    }
    else { // Clock
        float cy = contentTop + 84.f;
        L.centerCy = cy;
        L.clockTime = R(20.f, cy - 44.f, L.W - 40.f, 88.f);
        L.clockDate = R(0.f, cy + 46.f, L.W, 24.f);

        float pillW = (std::min)(320.f, L.W - 80.f);
        L.tzPill = R((L.W - pillW) * 0.5f, L.clockDate.b + 28.f, pillW, 42.f);
        L.tzPrev = R(L.tzPill.l + 10.f, L.tzPill.t + 7.f, 28.f, 28.f);
        L.tzNext = R(L.tzPill.r - 38.f, L.tzPill.t + 7.f, 28.f, 28.f);
        L.tzName = RectF{ L.tzPrev.r, L.tzPill.t, L.tzNext.l, L.tzPill.b };

        AddHit(L, ID_TZ_PREV, L.tzPrev);
        AddHit(L, ID_TZ_NEXT, L.tzNext);
        AddHit(L, ID_TZ_NAME, L.tzName);

        if (a.tzOpen) {
            float top = L.tzPill.b + 10.f;
            float bottom = L.H - 22.f;
            L.tzPanel = R(pad, top, L.W - pad * 2.f, (std::max)(100.f, bottom - top));
            L.tzRowH = 32.f;
            L.tzVisibleRows = static_cast<int>(L.tzPanel.H() / L.tzRowH);

            int count = static_cast<int>(zones.size());
            int first = static_cast<int>(a.tzScroll / L.tzRowH);
            int shown = (std::min)(L.tzVisibleRows + 1, count - first);
            for (int i = 0; i < shown; ++i) {
                int idx = first + i;
                RectF row = R(L.tzPanel.l, L.tzPanel.t + i * L.tzRowH - std::fmod(a.tzScroll, L.tzRowH),
                              L.tzPanel.W(), L.tzRowH);
                AddHit(L, ID_TZ_ROW + idx, row);
            }
        }
    }

    return L;
}

int HitTest(const Layout& L, float x, float y) {
    for (auto it = L.hits.rbegin(); it != L.hits.rend(); ++it) {
        if (it->r.Contains(x, y)) return it->id;
    }
    return ID_NONE;
}

// ------------------------------------------------------------------ 绘制：通用

static std::wstring TabLabel(int i) {
    switch (i) {
    case 0: return L"倒计时";
    case 1: return L"秒表";
    case 2: return L"定时";
    default: return L"时钟";
    }
}

static void DrawChip(Renderer& rr, const RectF& r, const std::wstring& label, bool sel, bool hov) {
    Color fill = sel ? WithAlpha(Pal::Accent, 0.16f)
                     : WithAlpha(Pal::Surface, hov ? 0.95f : 0.62f);
    rr.FillRoundRect(r, 9.f, fill);
    rr.StrokeRoundRect(r, 9.f, sel ? WithAlpha(Pal::Accent, 0.55f) : WithAlpha(Pal::Border, 0.9f), 1.f);
    rr.Text(label, r, TF_SMALL, sel ? Pal::Accent : (hov ? Pal::Text : Pal::TextDim));
}

static void DrawPrimary(Renderer& rr, const AppState& a, int id, const std::wstring& label,
                        bool filled, const RectF& rect) {
    bool hov = a.hover == id;
    bool prs = a.press == id;

    if (filled) {
        Color base = Pal::Accent;
        if (hov) base = LerpColor(Pal::Accent, C(1, 1, 1, 1), 0.12f);
        if (prs) base = LerpColor(Pal::Accent, C(0, 0, 0, 1), 0.14f);
        rr.FillRoundRect(rect, rect.H() * 0.5f, base);
        rr.Text(label, rect, TF_BUTTON, Pal::OnAccent);
    }
    else {
        Color fill = WithAlpha(Pal::SurfaceHi, prs ? 0.9f : (hov ? 0.8f : 0.52f));
        rr.FillRoundRect(rect, rect.H() * 0.5f, fill);
        rr.StrokeRoundRect(rect, rect.H() * 0.5f, WithAlpha(Pal::BorderHi, 0.75f), 1.f);
        rr.Text(label, rect, TF_BUTTON, hov ? Pal::Text : WithAlpha(Pal::Text, 0.9f));
    }
}

static void DrawReset(Renderer& rr, const AppState& a, const Layout& L, const wchar_t* label) {
    bool hov = a.hover == ID_RESET;
    rr.FillRoundRect(L.reset, 24.f, WithAlpha(Pal::SurfaceHi, hov ? 0.8f : 0.5f));
    rr.StrokeRoundRect(L.reset, 24.f, WithAlpha(Pal::BorderHi, 0.7f), 1.f);
    rr.Text(label, L.reset, TF_SMALL, hov ? Pal::Text : Pal::TextDim);
}

static void DrawTitleButtons(Renderer& rr, const AppState& a, const Layout& L) {
    // 设置（滑杆图标）
    bool hGear = a.hover == ID_GEAR;
    rr.FillRoundRect(L.btnGear, 8.f, hGear ? WithAlpha(Pal::SurfaceHi, 0.9f) : C(0, 0, 0, 0.f));
    {
        Color c = hGear ? Pal::Text : Pal::TextMute;
        float cx = L.btnGear.cx(), cy = L.btnGear.cy();
        for (int i = 0; i < 2; ++i) {
            float y = cy - 4.f + i * 8.f;
            rr.FillRoundRect(R(cx - 7.f, y - 1.f, 14.f, 2.f), 1.f, WithAlpha(c, 0.8f));
            rr.FillCircle(cx + (i == 0 ? -3.f : 3.f), y, 2.6f, c);
        }
    }

    // 置顶（图钉）
    bool hPin = a.hover == ID_PIN;
    rr.FillRoundRect(L.btnPin, 8.f, hPin ? WithAlpha(Pal::SurfaceHi, 0.9f) : C(0, 0, 0, 0.f));
    {
        Color c = a.alwaysOnTop ? Pal::Accent : (hPin ? Pal::Text : Pal::TextMute);
        float cx = L.btnPin.cx(), cy = L.btnPin.cy();
        rr.FillCircle(cx, cy - 4.f, 3.6f, c);
        rr.FillRoundRect(R(cx - 1.2f, cy - 1.f, 2.4f, 5.f), 1.2f, c);
        rr.FillRoundRect(R(cx - 5.f, cy + 4.f, 10.f, 2.f), 1.f, c);
    }

    // 最小化
    bool hMin = a.hover == ID_MIN_BTN;
    rr.FillRoundRect(L.btnMin, 8.f, hMin ? WithAlpha(Pal::SurfaceHi, 0.9f) : C(0, 0, 0, 0.f));
    rr.FillRoundRect(R(L.btnMin.cx() - 5.f, L.btnMin.cy() - 1.f, 10.f, 2.f), 1.f,
                     hMin ? Pal::Text : Pal::TextMute);

    // 关闭
    bool hClose = a.hover == ID_CLOSE_BTN;
    rr.FillRoundRect(L.btnClose, 8.f,
                     hClose ? WithAlpha(Pal::Danger, 0.85f) : C(0, 0, 0, 0.f));
    rr.Text(L"×", L.btnClose, TF_BODY, hClose ? C(1, 1, 1, 1) : Pal::TextMute);
}

static void DrawChrome(Renderer& rr, const AppState& a, const Layout& L) {
    rr.StrokeRoundRect(RectF{ 0.5f, 0.5f, L.W - 0.5f, L.H - 0.5f }, 9.f,
                       WithAlpha(Pal::BorderHi, Pal::IsLight(a.themeIndex) ? 0.9f : 0.55f), 1.f);

    rr.Text(L"计时器", R(22.f, 0.f, 150.f, 46.f), TF_LABEL, WithAlpha(Pal::TextDim, 0.95f), AL_LEFT);
    DrawTitleButtons(rr, a, L);

    for (int i = 0; i < 4; ++i) {
        bool active = (i == static_cast<int>(a.mode));
        bool hov = a.hover == ID_TAB + i;
        if (active) {
            rr.FillRoundRect(L.tabs[i], 9.f, Pal::SurfaceHi);
            rr.StrokeRoundRect(L.tabs[i], 9.f, WithAlpha(Pal::BorderHi, 0.8f), 1.f);
        }
        else if (hov) {
            rr.FillRoundRect(L.tabs[i], 9.f, WithAlpha(Pal::Surface, 0.7f));
        }
        rr.Text(TabLabel(i), L.tabs[i], TF_TAB,
                active ? Pal::Text : (hov ? WithAlpha(Pal::Text, 0.85f) : Pal::TextDim));
    }
}

// ------------------------------------------------------------------ 绘制：圆环

static void DrawRingTrack(Renderer& rr, const Layout& L) {
    rr.StrokeCircle(L.ringCx, L.ringCy, L.ringRadius, L.ringStroke, Pal::Track);
}

static void DrawProgress(Renderer& rr, const Layout& L, float frac, const Color& col,
                         float timeSec, bool animate) {
    frac = Clamp01(frac);
    if (frac <= 0.002f) return;

    float glow = 0.10f;
    if (animate) glow += 0.025f * sinf(timeSec * 2.2f);
    rr.RadialGlow(L.ringCx, L.ringCy, L.ringRadius + L.ringRadius * 0.24f, WithAlpha(col, glow));

    float sweep = 360.f * frac;
    rr.Arc(L.ringCx, L.ringCy, L.ringRadius, 0.f, sweep, L.ringStroke, col);

    if (animate && sweep > 2.f && sweep < 358.f) {
        float rad = (sweep - 90.f) * 3.14159265f / 180.f;
        rr.FillCircle(L.ringCx + L.ringRadius * cosf(rad), L.ringCy + L.ringRadius * sinf(rad),
                      L.ringStroke * 0.6f, WithAlpha(Pal::Text, 0.92f));
    }
}

// ------------------------------------------------------------------ 绘制：时间编辑器

static void DrawTimeEditor(Renderer& rr, const AppState& a, const Layout& L,
                           int hh, int mm, const wchar_t* hint) {
    (void)a;
    float fs = EditorFont(L);

    wchar_t hb[8], mb[8];
    swprintf_s(hb, L"%02d", hh);
    swprintf_s(mb, L"%02d", mm);

    rr.BigDigits(hb, L.editHCx, L.ringCy, fs, Pal::Text, 1.0f);
    rr.BigDigits(mb, L.editMCx, L.ringCy, fs, Pal::Text, 1.0f);

    rr.Text(L":", R(L.ringCx - 16.f, L.ringCy - fs * 0.5f, 32.f, fs), TF_MED,
            WithAlpha(Pal::TextDim, 0.9f));

    auto arrow = [&](const RectF& r, int id, const wchar_t* glyph) {
        bool hov = a.hover == id;
        rr.FillRoundRect(r, 9.f, WithAlpha(Pal::Surface, hov ? 0.98f : 0.7f));
        if (hov) rr.StrokeRoundRect(r, 9.f, WithAlpha(Pal::BorderHi, 0.9f), 1.f);
        rr.Text(glyph, r, TF_SMALL, hov ? Pal::Text : Pal::TextDim);
    };

    bool cd = (a.mode == Mode::Countdown);
    (void)cd;

    if (a.mode == Mode::Countdown) {
        arrow(L.hourUp, ID_EDIT_BASE + 0, L"▲");
        arrow(L.hourDown, ID_EDIT_BASE + 1, L"▼");
        arrow(L.minUp, ID_EDIT_BASE + 2, L"▲");
        arrow(L.minDown, ID_EDIT_BASE + 3, L"▼");
    }
    else {
        arrow(L.hourUp, ID_ALARM_BASE + 0, L"▲");
        arrow(L.hourDown, ID_ALARM_BASE + 1, L"▼");
        arrow(L.minUp, ID_ALARM_BASE + 2, L"▲");
        arrow(L.minDown, ID_ALARM_BASE + 3, L"▼");
    }

    if (hint) {
        float y = L.hourDown.b + 14.f;
        rr.Text(hint, R(10.f, y, L.W - 20.f, 20.f), TF_SMALL, WithAlpha(Pal::TextMute, 0.95f));
    }
}

// ------------------------------------------------------------------ 绘制：倒计时

static void DrawCountdown(Renderer& rr, const AppState& a, const Layout& L, float timeSec) {
    if (a.cdEditing) {
        // 编辑态不画圆环，避免文字压在环上
        DrawTimeEditor(rr, a, L, a.cdEditH, a.cdEditM, L"点击 ▲ ▼ 调整小时 / 分钟");

        for (int i = 0; i < L.presetCount; ++i) {
            std::wstring label = (i < 5) ? std::to_wstring(kCdPreset[i]) + L"分" : L"自定义";
            DrawChip(rr, L.presets[i], label, i == 5, a.hover == ID_PRESET + i);
        }
        DrawPrimary(rr, a, ID_PRIMARY, L"确定", true, L.primary);
        DrawReset(rr, a, L, L"取消");
        return;
    }

    double rem = CountdownRemaining(a);
    float frac = (a.cdTotal > 0.01) ? static_cast<float>(rem / a.cdTotal) : 0.f;

    Color arc = Pal::Accent;
    if (a.cdFinished) arc = Pal::Danger;
    else if (rem <= (std::min)(60.0, a.cdTotal * 0.15)) arc = Pal::Warn;

    DrawRingTrack(rr, L);
    float sweep = (a.cdRunning || a.cdFinished) ? a.ring : frac;
    DrawProgress(rr, L, sweep, arc, timeSec, a.cdRunning);

    std::wstring label;
    if (a.cdFinished) label = L"时间到";
    else if (a.cdRunning) label = L"正在倒计时";
    else if (rem < a.cdTotal - 0.5) label = L"已暂停";
    else label = L"准备开始";

    Color timeColor = a.cdFinished ? WithAlpha(Pal::Danger, 0.78f + 0.22f * a.pulse) : Pal::Text;

    bool useHours = a.cdTotal >= 3600.0;
    std::wstring t = FormatDur(rem, useHours);
    float fs = FitFont(L, t.size());
    float cy = L.ringCy - fs * 0.04f;
    rr.BigDigits(t, L.ringCx, cy, fs, timeColor, 1.0f);
    rr.Text(label, R(20.f, cy + fs * 0.56f + 10.f, L.W - 40.f, 22.f), TF_BODY,
            WithAlpha(Pal::TextDim, 0.95f));

    bool matchPreset = false;
    for (int i = 0; i < 5; ++i) {
        if (std::fabs(a.cdTotal - kCdPreset[i] * 60.0) < 0.5) matchPreset = true;
    }
    for (int i = 0; i < L.presetCount; ++i) {
        bool sel = (i < 5) ? (std::fabs(a.cdTotal - kCdPreset[i] * 60.0) < 0.5)
                           : !matchPreset;
        std::wstring label2 = (i < 5) ? std::to_wstring(kCdPreset[i]) + L"分" : L"自定义";
        DrawChip(rr, L.presets[i], label2, sel, a.hover == ID_PRESET + i);
    }

    std::wstring primaryLabel;
    if (a.cdFinished) primaryLabel = L"再来一次";
    else if (a.cdRunning) primaryLabel = L"暂停";
    else if (rem < a.cdTotal - 0.5 && rem > 0.01) primaryLabel = L"继续";
    else primaryLabel = L"开始";

    DrawPrimary(rr, a, ID_PRIMARY, primaryLabel, !a.cdRunning, L.primary);
    DrawReset(rr, a, L, L"重置");
}

// ------------------------------------------------------------------ 绘制：秒表

static std::wstring FormatCS(double sec) {
    if (sec < 0) sec = 0;
    long long total = static_cast<long long>(sec * 100.0 + 0.5);
    long long cs = total % 100;
    long long s = total / 100;
    long long h = s / 3600;
    long long m = (s % 3600) / 60;
    long long ss = s % 60;
    wchar_t buf[64];
    if (h > 0) swprintf_s(buf, L"%lld:%02lld:%02lld.%02lld", h, m, ss, cs);
    else swprintf_s(buf, L"%02lld:%02lld.%02lld", m, ss, cs);
    return buf;
}

static void DrawLapPanel(Renderer& rr, const AppState& a, const Layout& L, double elapsed) {
    (void)elapsed;
    const RectF& p = L.lapPanel;

    rr.FillRoundRect(p, 12.f, Pal::Surface);
    rr.StrokeRoundRect(p, 12.f, WithAlpha(Pal::BorderHi, 0.9f), 1.f);

    wchar_t head[64];
    swprintf_s(head, L"计次记录 · 共 %d 次", static_cast<int>(a.swLaps.size()));
    rr.Text(head, R(p.l + 16.f, p.t + 6.f, p.W() - 32.f, 26.f), TF_SMALL, Pal::TextDim, AL_LEFT);

    float bodyTop = p.t + 38.f;
    rr.PushClip(R(p.l + 4.f, bodyTop - 2.f, p.W() - 8.f, p.H() - 44.f));

    int n = static_cast<int>(a.swLaps.size());
    int first = static_cast<int>(a.swLapScroll / L.lapRowH);
    float yoff = std::fmod(a.swLapScroll, L.lapRowH);

    for (int i = 0; i < L.lapVisibleRows + 1; ++i) {
        int idx = n - 1 - (first + i);
        if (idx < 0) break;
        RectF row = R(p.l + 10.f, bodyTop + i * L.lapRowH - yoff, p.W() - 20.f, L.lapRowH);

        double totalT = a.swLaps[idx];
        double split = totalT - (idx > 0 ? a.swLaps[idx - 1] : 0.0);

        wchar_t no[24];
        swprintf_s(no, L"第 %d 次", idx + 1);
        rr.Text(no, R(row.l, row.t, 96.f, row.H()), TF_SMALL,
                WithAlpha(Pal::TextMute, 0.95f), AL_LEFT);
        rr.Text(FormatCS(split), R(row.l + 96.f, row.t, 110.f, row.H()), TF_SMALL, Pal::Text,
                AL_LEFT);
        rr.Text(FormatCS(totalT), R(row.l + 206.f, row.t, row.W() - 206.f, row.H()), TF_SMALL,
                WithAlpha(Pal::TextDim, 0.95f), AL_RIGHT);
    }
    rr.PopClip();
}

static void DrawStopwatch(Renderer& rr, const AppState& a, const Layout& L, float timeSec) {
    double elapsed = a.swBase + (a.swRunning ? (SteadyNow() - a.swStart) : 0.0);

    DrawRingTrack(rr, L);
    if (a.swRunning) DrawProgress(rr, L, a.ring, Pal::Accent, timeSec, true);

    // 主时间（整秒）+ 百分秒
    bool useHours = elapsed >= 3600.0;
    std::wstring main = FormatDur(std::floor(elapsed), useHours);
    int cs = static_cast<int>((elapsed - std::floor(elapsed)) * 100.0);
    if (cs < 0) cs = 0;
    if (cs > 99) cs = 99;
    wchar_t frac[8];
    swprintf_s(frac, L".%02d", cs);

    float avail = 2.f * (L.ringRadius - L.ringStroke) * 0.84f;
    float need = 0.56f * static_cast<float>(main.size()) + 0.62f;
    float fs = avail / need;
    fs = (std::min)(fs, L.ringRadius * 1.15f);
    fs = std::clamp(fs, 18.f, 180.f);

    const float track = 1.0f;
    float wMain = rr.BigDigitsWidth(main, fs, track, false);
    float wFrac = rr.BigDigitsWidth(frac, fs * 0.52f, track * 0.6f, false);
    float gap = fs * 0.05f;
    float total = wMain + gap + wFrac;
    float startX = L.ringCx - total * 0.5f;
    float cy = L.ringCy - fs * 0.10f;

    Color mainColor = a.swRunning ? Pal::Text : WithAlpha(Pal::Text, 0.92f);
    rr.BigDigits(main, startX + wMain * 0.5f, cy, fs, mainColor, track);
    rr.BigDigits(frac, startX + wMain + gap + wFrac * 0.5f, cy + fs * 0.26f, fs * 0.52f,
                 WithAlpha(Pal::Accent, 0.95f), track * 0.6f);

    std::wstring label;
    if (!a.swLaps.empty() && !a.swLapOpen) {
        double lastTotal = a.swLaps.back();
        double cur = (elapsed > lastTotal) ? (elapsed - lastTotal) : 0.0;
        wchar_t buf[96];
        swprintf_s(buf, L"第 %d 次 · 本轮 %s", static_cast<int>(a.swLaps.size()),
                   FormatCS(cur).c_str());
        label = buf;
    }
    else {
        label = a.swRunning ? L"计时中" : (elapsed > 0.005 ? L"已暂停" : L"准备开始");
    }
    float labelY = (std::min)(cy + fs * 0.58f + 8.f, L.ringCy + L.ringRadius - 28.f);
    rr.Text(label, R(20.f, labelY, L.W - 40.f, 22.f), TF_BODY, WithAlpha(Pal::TextDim, 0.95f));

    // 嘀嗒声开关（置于"开始"左侧）
    {
        static const wchar_t* kState[3] = { L"关", L"嘀嗒", L"快速" };
        int mode = std::clamp(a.swSoundMode, 0, 2);
        bool on = mode != 0;
        bool hov = a.hover == ID_TOGGLE_SOUND;

        rr.FillRoundRect(L.soundBtn, 13.f,
                         on ? WithAlpha(Pal::Accent, 0.16f) : WithAlpha(Pal::SurfaceHi, hov ? 0.85f : 0.5f));
        rr.StrokeRoundRect(L.soundBtn, 13.f,
                           on ? WithAlpha(Pal::Accent, 0.5f) : WithAlpha(Pal::BorderHi, 0.7f), 1.f);

        rr.Text(L"音效", R(L.soundBtn.l, L.soundBtn.t + 5.f, L.soundBtn.W(), 14.f), TF_TINY,
                WithAlpha(Pal::TextDim, hov ? 1.f : 0.85f));
        rr.Text(kState[mode], R(L.soundBtn.l, L.soundBtn.t + 19.f, L.soundBtn.W(), 20.f), TF_SMALL,
                on ? Pal::Accent : Pal::TextDim);
    }

    // 计次 / 计次记录
    {
        bool canLap = elapsed > 0.005;
        bool hovLap = a.hover == ID_LAP;
        bool hovList = a.hover == ID_LAP_LIST;
        bool hasLap = !a.swLaps.empty();
        bool open = a.swLapOpen && hasLap;

        rr.FillRoundRect(L.lapBtn, 9.f,
                         WithAlpha(Pal::Surface, canLap ? (hovLap ? 0.95f : 0.7f) : 0.35f));
        rr.StrokeRoundRect(L.lapBtn, 9.f, WithAlpha(Pal::Border, canLap ? 0.9f : 0.5f), 1.f);
        wchar_t buf[32];
        if (hasLap) swprintf_s(buf, L"计次 · %d", static_cast<int>(a.swLaps.size()));
        else swprintf_s(buf, L"计次");
        rr.Text(buf, L.lapBtn, TF_SMALL,
                canLap ? (hovLap ? Pal::Text : Pal::TextDim) : WithAlpha(Pal::TextMute, 0.6f));

        rr.FillRoundRect(L.lapListBtn, 9.f,
                         open ? WithAlpha(Pal::Accent, 0.16f)
                              : WithAlpha(Pal::Surface, hasLap ? (hovList ? 0.95f : 0.7f) : 0.35f));
        rr.StrokeRoundRect(L.lapListBtn, 9.f,
                           open ? WithAlpha(Pal::Accent, 0.55f)
                                : WithAlpha(Pal::Border, hasLap ? 0.9f : 0.5f), 1.f);
        rr.Text(open ? L"收起记录" : L"计次记录", L.lapListBtn, TF_SMALL,
                hasLap ? (open ? Pal::Accent : (hovList ? Pal::Text : Pal::TextDim))
                       : WithAlpha(Pal::TextMute, 0.6f));

        if (open) DrawLapPanel(rr, a, L, elapsed);
    }

    DrawPrimary(rr, a, ID_PRIMARY,
                a.swRunning ? L"暂停" : (elapsed > 0.005 ? L"继续" : L"开始"),
                !a.swRunning, L.primary);
    DrawReset(rr, a, L, L"重置");
}

// ------------------------------------------------------------------ 绘制：定时

static void DrawAlarm(Renderer& rr, const AppState& a, const Layout& L, float timeSec) {
    struct Seg { RectF r; const wchar_t* label; bool on; int id; };
    Seg segs[2] = {
        { L.alarmDur, L"相对时长", !a.alAsTimeOfDay, ID_ALARM_DUR },
        { L.alarmTod, L"指定时刻", a.alAsTimeOfDay, ID_ALARM_TOD },
    };
    for (int i = 0; i < 2; ++i) {
        bool hov = a.hover == segs[i].id;
        rr.FillRoundRect(segs[i].r, 9.f,
                         segs[i].on ? Pal::SurfaceHi : WithAlpha(Pal::Surface, hov ? 0.8f : 0.45f));
        if (segs[i].on) rr.StrokeRoundRect(segs[i].r, 9.f, WithAlpha(Pal::BorderHi, 0.85f), 1.f);
        rr.Text(segs[i].label, segs[i].r, TF_SMALL, segs[i].on ? Pal::Text : Pal::TextDim);
    }

    if (a.alActive) {
        double rem = AlarmRemaining(a);
        float frac = (a.alSpan > 0.01) ? static_cast<float>(rem / a.alSpan) : 0.f;
        Color col = a.alFinished ? Pal::Danger : Pal::Accent;

        DrawRingTrack(rr, L);
        DrawProgress(rr, L, frac, col, timeSec, !a.alFinished);

        std::wstring t = FormatDur(rem);
        float fs = FitFont(L, t.size());
        float cy = L.ringCy - fs * 0.04f;

        Color numCol = a.alFinished ? WithAlpha(Pal::Danger, 0.78f + 0.22f * a.pulse) : Pal::Text;
        std::wstring label;
        if (a.alFinished) {
            label = L"时间到";
        }
        else if (a.alAsTimeOfDay) {
            wchar_t buf[64];
            swprintf_s(buf, L"到 %02d:%02d 提醒", a.alHour, a.alMinute);
            label = buf;
        }
        else {
            label = L"时长结束提醒";
        }

        rr.BigDigits(t, L.ringCx, cy, fs, numCol, 1.0f);
        rr.Text(label, R(20.f, cy + fs * 0.56f + 10.f, L.W - 40.f, 22.f), TF_BODY,
                WithAlpha(Pal::TextDim, 0.95f));

        DrawPrimary(rr, a, ID_CANCEL_ALARM, L"取消定时", false, L.primary);
        return;
    }

    if (!a.alAsTimeOfDay) {
        double chosen = a.alSpan > 0.5 ? a.alSpan : 3600.0;

        if (a.alEditing) {
            DrawTimeEditor(rr, a, L, a.alEditH, a.alEditM, L"点击 ▲ ▼ 设定自定义时长");
        }
        else {
            DrawRingTrack(rr, L);
            rr.Arc(L.ringCx, L.ringCy, L.ringRadius, 0.f, 360.f, L.ringStroke,
                   WithAlpha(Pal::Accent, 0.20f));

            std::wstring t = FormatDur(chosen, chosen >= 3600.0);
            float fs = FitFont(L, t.size());
            float cy = L.ringCy - fs * 0.04f;
            rr.BigDigits(t, L.ringCx, cy, fs, Pal::Text, 1.0f);
            rr.Text(L"选择时长", R(20.f, cy + fs * 0.56f + 10.f, L.W - 40.f, 22.f), TF_BODY,
                    WithAlpha(Pal::TextDim, 0.95f));
        }

        bool matchPreset = false;
        for (int i = 0; i < 4; ++i) {
            if (std::fabs(chosen - kAlPreset[i] * 60.0) < 0.5) matchPreset = true;
        }

        for (int i = 0; i < L.presetCount; ++i) {
            bool sel;
            std::wstring lbl;
            if (i < 4) {
                sel = std::fabs(chosen - kAlPreset[i] * 60.0) < 0.5;
                lbl = std::to_wstring(kAlPreset[i]) + L"分";
            }
            else {
                sel = a.alEditing || !matchPreset;
                lbl = L"自定义";
            }
            DrawChip(rr, L.presets[i], lbl, sel, a.hover == ID_PRESET + i);
        }

        DrawPrimary(rr, a, ID_PRIMARY, a.alEditing ? L"确定" : L"开始定时", true, L.primary);
        if (a.alEditing) DrawReset(rr, a, L, L"取消");
    }
    else {
        DrawTimeEditor(rr, a, L, a.alHour, a.alMinute, L"点击 ▲ ▼ 调整小时 / 分钟");
        DrawPrimary(rr, a, ID_PRIMARY, L"开始定时", true, L.primary);
    }
}

// ------------------------------------------------------------------ 绘制：时钟

static void DrawClock(Renderer& rr, const AppState& a, const Layout& L,
                      const std::vector<TzItem>& zones) {
    SYSTEMTIME st{};
    ZoneLocalTime(zones, a.tzIndex, st);

    std::wstring t = FormatHMS(st);
    float fs = std::clamp((L.W - 96.f) / (8.f * 0.58f), 26.f, 96.f);
    rr.BigDigits(t, L.W * 0.5f, L.centerCy, fs, Pal::Text, 1.0f, true);
    rr.Text(FormatDateCN(st), L.clockDate, TF_BODY, Pal::TextDim);

    bool hovName = a.hover == ID_TZ_NAME;
    rr.FillRoundRect(L.tzPill, 21.f, WithAlpha(Pal::Surface, hovName ? 0.95f : 0.7f));
    rr.StrokeRoundRect(L.tzPill, 21.f, WithAlpha(Pal::Border, 0.95f), 1.f);

    bool hovPrev = a.hover == ID_TZ_PREV;
    bool hovNext = a.hover == ID_TZ_NEXT;
    rr.Text(L"‹", L.tzPrev, TF_TITLE, hovPrev ? Pal::Text : Pal::TextDim);
    rr.Text(L"›", L.tzNext, TF_TITLE, hovNext ? Pal::Text : Pal::TextDim);

    int idx = std::clamp(a.tzIndex, 0, (std::max)(0, static_cast<int>(zones.size()) - 1));
    std::wstring name = zones.empty() ? L"本地时间" : zones[idx].name;
    int offMin = zones.empty() ? 0 : zones[idx].offsetMin;

    wchar_t off[32] = L"";
    if (!zones.empty() && a.tzIndex > 0) {
        int sign = offMin < 0 ? -1 : 1;
        int abs = offMin * sign;
        swprintf_s(off, L"UTC%s%d:%02d", sign < 0 ? L"-" : L"+", abs / 60, abs % 60);
    }

    rr.Text(name, RectF{ L.tzName.l, L.tzName.t + 2.f, L.tzName.r, L.tzName.cy() }, TF_SMALL, Pal::Text);
    if (off[0])
        rr.Text(off, RectF{ L.tzName.l, L.tzName.cy() - 2.f, L.tzName.r, L.tzName.b }, TF_TINY,
                Pal::TextMute);

    if (!a.tzOpen) {
        rr.Text(L"点击可切换时区", R(0.f, L.tzPill.b + 12.f, L.W, 20.f), TF_SMALL,
                WithAlpha(Pal::TextMute, 0.9f));
    }

    if (a.tzOpen) {
        rr.FillRoundRect(L.tzPanel, 10.f, WithAlpha(Pal::Surface, 0.98f));
        rr.StrokeRoundRect(L.tzPanel, 10.f, WithAlpha(Pal::BorderHi, 0.9f), 1.f);

        rr.PushClip(L.tzPanel.Inset(2.f));
        int count = static_cast<int>(zones.size());
        int first = static_cast<int>(a.tzScroll / L.tzRowH);
        float yoff = std::fmod(a.tzScroll, L.tzRowH);
        for (int i = 0; i < L.tzVisibleRows + 1; ++i) {
            int j = first + i;
            if (j >= count) break;
            RectF row = R(L.tzPanel.l + 2.f, L.tzPanel.t + i * L.tzRowH - yoff,
                          L.tzPanel.W() - 4.f, L.tzRowH);
            bool sel = j == a.tzIndex;
            bool hov = a.hover == ID_TZ_ROW + j;
            if (sel) rr.FillRoundRect(row.Inset(3.f), 6.f, WithAlpha(Pal::Accent, 0.16f));
            else if (hov) rr.FillRoundRect(row.Inset(3.f), 6.f, WithAlpha(Pal::SurfaceHi, 0.8f));

            std::wstring label = zones[j].name;
            if (j > 0) {
                int sign = zones[j].offsetMin < 0 ? -1 : 1;
                int abs = zones[j].offsetMin * sign;
                wchar_t buf[128];
                swprintf_s(buf, L"UTC%s%d:%02d  %s", sign < 0 ? L"-" : L"+", abs / 60, abs % 60,
                           zones[j].name.c_str());
                label = buf;
            }
            rr.Text(label, RectF{ row.l + 12.f, row.t, row.r - 8.f, row.b }, TF_SMALL,
                    sel ? Pal::Accent : (hov ? Pal::Text : WithAlpha(Pal::Text, 0.82f)), AL_LEFT);
        }
        rr.PopClip();
    }
}

// ------------------------------------------------------------------ 绘制：设置

static void DrawToggle(Renderer& rr, const AppState& a, const RectF& r, const std::wstring& label,
                       bool on, int id) {
    bool hov = a.hover == id;
    if (hov) rr.FillRoundRect(r, 10.f, WithAlpha(Pal::Surface, 0.7f));
    rr.Text(label, RectF{ r.l + 10.f, r.t, r.r - 70.f, r.b }, TF_BODY,
            hov ? Pal::Text : WithAlpha(Pal::Text, 0.9f), AL_LEFT);

    RectF track = R(r.r - 54.f, r.cy() - 12.f, 44.f, 24.f);
    rr.FillRoundRect(track, 12.f, on ? Pal::Accent : WithAlpha(Pal::SurfaceHi, 0.95f));
    if (!on) rr.StrokeRoundRect(track, 12.f, WithAlpha(Pal::BorderHi, 0.8f), 1.f);
    float knobX = on ? track.r - 12.f : track.l + 12.f;
    rr.FillCircle(knobX, track.cy(), 9.f, on ? Pal::OnAccent : WithAlpha(Pal::TextDim, 0.95f));
}

static void DrawSettings(Renderer& rr, const AppState& a, const Layout& L) {
    rr.FillRoundRect(L.settingsPanel, 14.f, WithAlpha(Pal::Surface, 0.985f));
    rr.StrokeRoundRect(L.settingsPanel, 14.f, WithAlpha(Pal::BorderHi, 0.9f), 1.f);

    rr.Text(L"外观与行为", L.settingsTitle, TF_TITLE, Pal::Text, AL_LEFT);

    float px = L.settingsPanel.l + 18.f;
    rr.Text(L"主题", R(px, L.themeSwatch[0].t - 18.f, 120.f, 16.f), TF_SMALL,
            WithAlpha(Pal::TextMute, 0.95f), AL_LEFT);

    for (int i = 0; i < kThemeCount; ++i) {
        bool sel = (i == a.themeIndex);
        bool hov = (a.hover == ID_THEME + i);
        Color ac = Pal::AccentOf(i);
        rr.FillCircle(L.themeSwatch[i].cx(), L.themeSwatch[i].cy(), 14.f,
                      WithAlpha(ac, hov ? 1.f : 0.92f));
        if (sel) {
            rr.StrokeCircle(L.themeSwatch[i].cx(), L.themeSwatch[i].cy(), 17.f, 2.f, Pal::Text);
        }
        else if (hov) {
            rr.StrokeCircle(L.themeSwatch[i].cx(), L.themeSwatch[i].cy(), 16.f, 1.f,
                            WithAlpha(Pal::Text, 0.5f));
        }
    }
    rr.Text(kThemes[std::clamp(a.themeIndex, 0, kThemeCount - 1)].name, L.themeName, TF_SMALL,
            Pal::TextDim);

    rr.Text(L"背景", R(px, L.backdropChip[0].t - 18.f, 120.f, 16.f), TF_SMALL,
            WithAlpha(Pal::TextMute, 0.95f), AL_LEFT);

    const wchar_t* bdLabel[3] = { L"纯色渐变", L"极光动态", L"自定义图片" };
    for (int i = 0; i < 3; ++i) {
        bool sel = (a.backdrop == i);
        bool hov = (a.hover == ID_BACKDROP + i);
        rr.FillRoundRect(L.backdropChip[i], 9.f,
                         sel ? WithAlpha(Pal::Accent, 0.16f)
                             : WithAlpha(Pal::Surface, hov ? 0.95f : 0.62f));
        rr.StrokeRoundRect(L.backdropChip[i], 9.f,
                           sel ? WithAlpha(Pal::Accent, 0.55f) : WithAlpha(Pal::Border, 0.9f), 1.f);
        rr.Text(bdLabel[i], L.backdropChip[i], TF_SMALL,
                sel ? Pal::Accent : (hov ? Pal::Text : Pal::TextDim));
    }

    bool showImage = (a.backdrop == 2);
    if (showImage) {
        bool hov = (a.hover == ID_PICK_IMAGE);
        rr.FillRoundRect(L.pickImage, 8.f, WithAlpha(Pal::SurfaceHi, hov ? 0.95f : 0.62f));
        rr.StrokeRoundRect(L.pickImage, 8.f, WithAlpha(Pal::BorderHi, 0.85f), 1.f);
        rr.Text(L"选择图片…", L.pickImage, TF_SMALL, hov ? Pal::Text : Pal::TextDim);

        std::wstring fname = a.imagePath;
        size_t pos = fname.find_last_of(L"\\/");
        if (pos != std::wstring::npos) fname = fname.substr(pos + 1);
        if (fname.empty()) fname = L"未选择";
        rr.Text(fname, L.imageName, TF_SMALL, WithAlpha(Pal::TextDim, 0.95f), AL_LEFT);
    }

    DrawToggle(rr, a, L.toggleTop, L"窗口置顶（始终在前）", a.alwaysOnTop, ID_TOGGLE_TOP);
    DrawToggle(rr, a, L.toggleTray, L"关闭窗口时收起到托盘", a.closeToTray, ID_TOGGLE_TRAY);

    rr.Text(L"主题与背景立即生效，并自动保存", R(0.f, L.toggleTray.b + 14.f, L.W, 18.f), TF_TINY,
            WithAlpha(Pal::TextMute, 0.9f));

    // 配置文件位置（便于确认保存是否生效）
    if (!a.settingsFile.empty()) {
        std::wstring show = a.settingsFile;
        size_t p1 = show.find_last_of(L'\\');
        if (p1 != std::wstring::npos) {
            std::wstring file = show.substr(p1 + 1);
            size_t p0 = show.find_last_of(L'\\', p1 - 1);
            show = (p0 != std::wstring::npos) ? show.substr(p0 + 1) : file;
        }
        rr.Text(L"配置：" + show, R(L.settingsPanel.l + 18.f, L.settingsPanel.b - 92.f,
                                   L.settingsPanel.W() - 36.f, 16.f),
                TF_TINY, WithAlpha(Pal::TextMute, 0.9f), AL_LEFT);
    }

    DrawPrimary(rr, a, ID_SETTINGS_DONE, L"完成", true, L.settingsDone);
}

// ------------------------------------------------------------------ 绘制：提示

static std::wstring TipFor(const AppState& a, int id) {
    switch (id) {
    case ID_GEAR:      return L"外观与行为（主题 / 背景）";
    case ID_PIN:       return L"窗口置顶";
    case ID_MIN_BTN:   return L"最小化";
    case ID_CLOSE_BTN: return L"关闭（收起到托盘）";
    case ID_RESET:     return L"重置";
    case ID_PRESET + 5: return L"自定义任意时长";
    case ID_TOGGLE_SOUND: {
        switch (std::clamp(a.swSoundMode, 0, 2)) {
        case 0: return L"秒表音效：关（点击切换）";
        case 1: return L"秒表音效：嘀嗒 tick-tock（点击切换）";
        default: return L"秒表音效：快速嗒嗒（点击切换）";
        }
    }
    default:           return L"";
    }
}

static RectF AnchorFor(int id, const Layout& L) {
    switch (id) {
    case ID_GEAR:      return L.btnGear;
    case ID_PIN:       return L.btnPin;
    case ID_MIN_BTN:   return L.btnMin;
    case ID_CLOSE_BTN: return L.btnClose;
    case ID_RESET:     return L.reset;
    case ID_TOGGLE_SOUND: return L.soundBtn;
    default:           return L.presets[5];
    }
}

static void DrawTooltips(Renderer& rr, const AppState& a, const Layout& L) {
    std::wstring tip = TipFor(a, a.hover);
    if (tip.empty()) return;
    if (SteadyNow() - a.hoverSince < 0.45) return;

    float w = rr.Measure(tip, TF_SMALL) + 22.f;
    const float h = 26.f;
    RectF anchor = AnchorFor(a.hover, L);

    float x, y;
    if (anchor.b <= 46.f) {
        // 标题栏按钮：提示放左侧，避免压住下面的标签页
        x = anchor.l - 8.f - w;
        y = anchor.cy() - h * 0.5f;
        if (x < 8.f) x = 8.f;
    }
    else if (anchor.b > L.H - 140.f) {
        // 底部按钮行：提示放上方，避免压住页脚与提示条
        x = std::clamp(anchor.cx() - w * 0.5f, 8.f, (std::max)(8.f, L.W - 8.f - w));
        y = anchor.t - 8.f - h;
        if (y < 50.f) y = anchor.b + 8.f;
    }
    else {
        x = std::clamp(anchor.cx() - w * 0.5f, 8.f, (std::max)(8.f, L.W - 8.f - w));
        y = anchor.b + 8.f;
        if (y + h > L.H - 6.f) y = anchor.t - 8.f - h;
    }

    RectF box = R(x, y, w, h);
    rr.FillRoundRect(box, 7.f, WithAlpha(Pal::Surface, 0.99f));
    rr.StrokeRoundRect(box, 7.f, WithAlpha(Pal::BorderHi, 0.95f), 1.f);
    rr.Text(tip, box, TF_SMALL, Pal::Text);
}

static void DrawFirstRunHint(Renderer& rr, const AppState& a, const Layout& L) {
    if (a.hintSeen || a.settingsOpen) return;

    std::wstring text = L"提示：右上角图标可设置主题与背景 · 关闭窗口会收起到托盘";
    float w = rr.Measure(text, TF_TINY) + 24.f;
    if (w > L.W - 40.f) w = L.W - 40.f;
    RectF box = R((L.W - w) * 0.5f, L.H - 70.f, w, 26.f);

    rr.FillRoundRect(box, 8.f, WithAlpha(Pal::Accent, 0.14f));
    rr.StrokeRoundRect(box, 8.f, WithAlpha(Pal::Accent, 0.45f), 1.f);
    rr.Text(text, box, TF_TINY, WithAlpha(Pal::Text, 0.95f));
}

// ------------------------------------------------------------------ 入口

void DrawApp(Renderer& rr, const AppState& a, const Layout& L,
             const std::vector<TzItem>& zones, float timeSec) {
    DrawChrome(rr, a, L);

    if (a.settingsOpen) {
        DrawSettings(rr, a, L);
        return;
    }

    switch (a.mode) {
    case Mode::Countdown: DrawCountdown(rr, a, L, timeSec); break;
    case Mode::Stopwatch: DrawStopwatch(rr, a, L, timeSec); break;
    case Mode::Alarm:     DrawAlarm(rr, a, L, timeSec); break;
    case Mode::Clock:     DrawClock(rr, a, L, zones); break;
    }

    if (!(a.mode == Mode::Clock && a.tzOpen)) {
        SYSTEMTIME now = SystemLocalNow();
        std::wstring foot = L"系统时间 " + FormatHMS(now);
        if (a.mode == Mode::Clock) foot = L"时间来自 Windows 系统时钟 · 时区数据官方内置";
        rr.Text(foot, L.footer, TF_TINY, WithAlpha(Pal::TextMute, 0.9f));
    }

    DrawFirstRunHint(rr, a, L);
    DrawTooltips(rr, a, L);
}
