#include "RenderUI.h"
#include "Constants.h"
#include "CaroAPI.h"
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdlib>

namespace Cyber {
    const sf::Color Cyan{ 0,  255, 255, 255 };
    const sf::Color CyanDim{ 0,  180, 200, 180 };
    const sf::Color CyanGlow{ 0,  255, 255,  40 };

    const sf::Color Magenta{ 255,  0, 200, 255 };
    const sf::Color MagDim{ 200,  0, 160, 180 };

    const sf::Color Yellow{ 255, 220,  0, 255 };
    const sf::Color YellowD{ 200, 170,  0, 200 };

    const sf::Color NeonRed{ 255,  50,  80, 255 };

    const sf::Color BgDeep{ 8,   8,  14, 255 };
    const sf::Color BgPanel{ 12,  16,  28, 230 };
    const sf::Color BgBtn{ 18,  22,  38, 255 };
    const sf::Color BgHover{ 30,  40,  70, 255 };

    const sf::Color Grid{ 40,  55,  80, 255 };
    const sf::Color GridDim{ 25,  35,  55, 255 };
    const sf::Color White{ 220, 230, 255, 255 };
    const sf::Color Gray{ 100, 115, 145, 255 };
}

int gLoadPreviewSlotUI = 1;

static std::string SaveModeLabel(int virusMode)
{
    return (virusMode == 1) ? "VIRUS MODE" : "NORMAL";
}

static sf::Color SaveModeColor(int virusMode)
{
    return (virusMode == 1) ? sf::Color(90, 255, 120, 255) : Cyber::Cyan;
}

static float PanelX(int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float boardRight = static_cast<float>(Config::OFFSET_X + boardSize * cellSz);
    float gapWidth = static_cast<float>(Config::WIN_WIDTH) - boardRight;
    return boardRight + (gapWidth - static_cast<float>(Config::PANEL_W)) / 2.0f;
}

static void DrawCornerBrackets(sf::RenderWindow& window,
    float x, float y, float w, float h,
    sf::Color col, float arm = 14.f, float thick = 2.f)
{
    auto seg = [&](float ax, float ay, float bx, float by) {
        float dx = bx - ax;
        float dy = by - ay;
        float len = std::sqrt(dx * dx + dy * dy);
        float ang = std::atan2(dy, dx) * 180.f / 3.14159265f;
        sf::RectangleShape s({ len, thick });
        s.setOrigin(0, thick / 2.f);
        s.setPosition(ax, ay);
        s.setRotation(ang);
        s.setFillColor(col);
        window.draw(s);
        };
    seg(x, y + arm, x, y); seg(x, y, x + arm, y);
    seg(x + w - arm, y, x + w, y); seg(x + w, y, x + w, y + arm);
    seg(x, y + h - arm, x, y + h); seg(x, y + h, x + arm, y + h);
    seg(x + w - arm, y + h, x + w, y + h); seg(x + w, y + h, x + w, y + h - arm);
}

static void DrawNeonRect(sf::RenderWindow& window,
    float x, float y, float w, float h,
    sf::Color fill, sf::Color border, float thickness = 1.5f)
{
    sf::RectangleShape r({ w, h });
    r.setPosition(x, y);
    r.setFillColor(fill);
    r.setOutlineThickness(thickness);
    r.setOutlineColor(border);
    window.draw(r);
}

static void DrawSciFiButton(sf::RenderWindow& window, float x, float y, float w, float h,
    sf::Color fillCol, sf::Color borderCol, float thickness, bool hovered)
{
    float cut = 15.f;

    sf::ConvexShape bg;
    bg.setPointCount(6);
    bg.setPoint(0, sf::Vector2f(x + cut, y));
    bg.setPoint(1, sf::Vector2f(x + w, y));
    bg.setPoint(2, sf::Vector2f(x + w, y + h - cut));
    bg.setPoint(3, sf::Vector2f(x + w - cut, y + h));
    bg.setPoint(4, sf::Vector2f(x, y + h));
    bg.setPoint(5, sf::Vector2f(x, y + cut));

    bg.setFillColor(fillCol);
    bg.setOutlineColor(borderCol);
    bg.setOutlineThickness(thickness);
    window.draw(bg);

    if (hovered) {
        sf::RectangleShape accent({ 4.f, h - cut * 2.f });
        accent.setPosition(x - 2.f, y + cut);
        accent.setFillColor(borderCol);
        window.draw(accent);

        sf::ConvexShape dec;
        dec.setPointCount(3);
        dec.setPoint(0, sf::Vector2f(x + w - cut + 5.f, y + h));
        dec.setPoint(1, sf::Vector2f(x + w, y + h - cut + 5.f));
        dec.setPoint(2, sf::Vector2f(x + w, y + h));
        dec.setFillColor(borderCol);
        window.draw(dec);
    }
}

static void DrawCentredText(sf::RenderWindow& window, const sf::Font& font,
    const std::string& str, unsigned size, sf::Color col,
    float cx, float cy)
{
    sf::Text t(str, font, size);
    t.setFillColor(col);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    t.setPosition(cx, cy);
    window.draw(t);
}

static void DrawScanlines(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color lineCol)
{
    for (float ly = y; ly < y + h; ly += 4.f) {
        sf::RectangleShape sl({ w, 1.f });
        sl.setPosition(x, ly);
        sl.setFillColor(lineCol);
        window.draw(sl);
    }
}

static void DrawGlitchStripes(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color col)
{
    for (int i = 0; i < 4; ++i) {
        float oy = y + h * 0.25f * (i + 1);
        sf::RectangleShape s({ w * 0.4f, 1.5f });
        s.setPosition(x + w * 0.05f + i * 10.f, oy);
        s.setFillColor(col);
        window.draw(s);
    }
}

static void DrawSectionHeader(sf::RenderWindow& window, const sf::Font& font, const std::string& label, float x, float y, float w, sf::Color col)
{
    sf::Text t(label, font, 13);
    t.setFillColor(col);
    t.setPosition(x, y);
    window.draw(t);
    float tw = t.getLocalBounds().width + 8.f;
    sf::RectangleShape line({ w - tw, 1.f });
    line.setPosition(x + tw, y + 9.f);
    line.setFillColor(sf::Color(col.r, col.g, col.b, 80));
    window.draw(line);
}

static void Draw3DSciFiButton(sf::RenderWindow& window,
    float x, float y, float w, float h,
    sf::Color fillCol, sf::Color borderCol, float thickness,
    bool hovered, const sf::Color& accent)
{
    const float cut = 15.f;
    const float depth = 6.f;

    auto face3D = [&](std::initializer_list<sf::Vector2f> pts, float bright) {
        sf::ConvexShape face;
        face.setPointCount(pts.size());
        int idx = 0;
        for (auto& p : pts) face.setPoint(idx++, p);
        face.setFillColor(sf::Color(
            static_cast<sf::Uint8>(accent.r * bright),
            static_cast<sf::Uint8>(accent.g * bright),
            static_cast<sf::Uint8>(accent.b * bright), 230));
        face.setOutlineThickness(0);
        window.draw(face);
        };

    // Mặt phải
    face3D({
        {x + w,         y},
        {x + w + depth, y + depth},
        {x + w + depth, y + h - cut + depth},
        {x + w,         y + h - cut}
        }, 0.22f);

    // Mặt góc chamfer phải-dưới
    face3D({
        {x + w,               y + h - cut},
        {x + w + depth,       y + h - cut + depth},
        {x + w - cut + depth, y + h + depth},
        {x + w - cut,         y + h}
        }, 0.16f);

    // Mặt dưới
    face3D({
        {x,                    y + h},
        {x + depth,            y + h + depth},
        {x + w - cut + depth,  y + h + depth},
        {x + w - cut,          y + h}
        }, 0.22f);

    // Mặt chính
    DrawSciFiButton(window, x, y, w, h, fillCol, borderCol, thickness, hovered);

    // Highlight cạnh
    sf::Uint8 hlA = hovered ? 130 : 60;
    sf::RectangleShape topHL({ w - cut - 1.f, 1.5f });
    topHL.setPosition(x + cut, y + 1.f);
    topHL.setFillColor(sf::Color(210, 230, 255, hlA));
    window.draw(topHL);

    sf::RectangleShape leftHL({ 1.5f, h - cut - 1.f });
    leftHL.setPosition(x + 1.f, y + cut);
    leftHL.setFillColor(sf::Color(210, 230, 255, static_cast<sf::Uint8>(hlA * 0.55f)));
    window.draw(leftHL);
}

void DrawMenu(sf::RenderWindow& window, const sf::Font& font, sf::Sprite& bgSprite)
{
    static sf::Clock menuClock;
    float t = menuClock.getElapsedTime().asSeconds();

    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    // ── 1. Background + scanlines ──────────────────────────────
    sf::FloatRect bgRect = bgSprite.getLocalBounds();
    bgSprite.setScale(W / bgRect.width, H / bgRect.height);
    window.draw(bgSprite);

    // ── 2. Particle Rain (Mưa kỹ thuật số) ─────────────────────
    struct RainDrop { float x, y, speed, len; };
    static std::vector<RainDrop> drops;
    if (drops.empty()) {
        for (int i = 0; i < 60; ++i) {
            drops.push_back({ (float)(rand() % Config::WIN_WIDTH), (float)(rand() % Config::WIN_HEIGHT), 100.f + rand() % 200, 15.f + rand() % 40 });
        }
    }
    static sf::Clock rainClock;
    float rDt = rainClock.restart().asSeconds();
    for (auto& d : drops) {
        d.y += d.speed * rDt;
        if (d.y > H) {
            d.y = -d.len;
            d.x = (float)(rand() % Config::WIN_WIDTH);
        }
        sf::RectangleShape rect({ 2.f, d.len });
        rect.setPosition(d.x, d.y);
        rect.setFillColor(sf::Color(0, 255, 255, 30));
        window.draw(rect);
    }

    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 80));

    // ── 3. Glitch Effect cho Title ──────────────────────────────
    float titlePulse = (std::sin(t * 2.2f) + 1.f) * 0.5f;

    // Kiểm tra thời gian để giật Glitch
    bool isGlitch = std::fmod(t, 3.0f) > 2.8f;
    bool glitchFlicker = isGlitch && (std::fmod(t, 0.05f) > 0.025f);

    float glitchOffX = glitchFlicker ? (rand() % 9 - 4.f) : 0.f;
    sf::Color titleColor = glitchFlicker ? Cyber::Magenta : Cyber::Cyan;

    sf::Text glowTitle("CARO MASTER", font, 76);
    glowTitle.setFillColor(sf::Color(titleColor.r, titleColor.g, titleColor.b, static_cast<sf::Uint8>(15 + titlePulse * 35)));
    sf::FloatRect gr = glowTitle.getLocalBounds();
    glowTitle.setOrigin(gr.left + gr.width / 2.f, gr.top + gr.height / 2.f);
    glowTitle.setPosition(W / 2.f + glitchOffX, 150.f);
    window.draw(glowTitle);

    sf::Text shadow("CARO MASTER", font, 72);
    shadow.setFillColor(sf::Color(titleColor.r, titleColor.g, titleColor.b, 25));
    sf::FloatRect sr = shadow.getLocalBounds();
    shadow.setOrigin(sr.left + sr.width / 2.f, sr.top + sr.height / 2.f);
    shadow.setPosition(W / 2.f + 3.f + glitchOffX, 153.f);
    window.draw(shadow);

    sf::Text title("CARO MASTER", font, 70);
    title.setFillColor(titleColor);
    title.setStyle(sf::Text::Bold);
    sf::FloatRect r = title.getLocalBounds();
    title.setOrigin(r.left + r.width / 2.f, r.top + r.height / 2.f);
    title.setPosition(W / 2.f + glitchOffX, 150.f);
    window.draw(title);

    float lineW = 320.f + titlePulse * 30.f;
    sf::RectangleShape titleLine({ lineW, 1.5f });
    titleLine.setOrigin(lineW / 2.f, 0.f);
    titleLine.setPosition(W / 2.f + glitchOffX, 195.f);
    titleLine.setFillColor(sf::Color(titleColor.r, titleColor.g, titleColor.b, static_cast<sf::Uint8>(80 + titlePulse * 80)));
    window.draw(titleLine);

    // ── 4. Vẽ các nút bấm menu ──────────────────────────────────
    const char* menuItems[] = {
        "PVP - 2 Players",
        "PVE - vs AI",
        "Settings",
        "Load Game",
        "About",
        "Exit"
    };

    const sf::Color btnBorder[] = {
        Cyber::Cyan,
        Cyber::Magenta,
        Cyber::Yellow,
        sf::Color(80, 200, 255),
        sf::Color(50, 255, 150),
        sf::Color(255, 60, 80)
    };

    const float BTN_W = 380.f;
    const float BTN_H = 62.f;
    const float START_Y = 288.f;
    const float STEP_Y = 82.f;
    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));

    for (int i = 0; i < 6; ++i)
    {
        float bX = W / 2.f - BTN_W / 2.f;
        float bY = START_Y + i * STEP_Y;
        bool  hov = (mp.x >= bX && mp.x <= bX + BTN_W &&
            mp.y >= bY && mp.y <= bY + BTN_H);

        float phase = t * 2.8f + i * 0.65f;
        float pulse = (std::sin(phase) + 1.f) * 0.5f;

        const sf::Color& accent = btnBorder[i];

        if (hov)
        {
            for (int g = 4; g >= 1; --g)
            {
                float expand = g * 4.f;
                float alpha = static_cast<float>(5 - g) * 12.f + pulse * 12.f;
                sf::RectangleShape bloom({ BTN_W + expand * 2.f, BTN_H + expand * 2.f });
                bloom.setPosition(bX - expand, bY - expand);
                bloom.setFillColor(sf::Color::Transparent);
                bloom.setOutlineThickness(1.5f);
                bloom.setOutlineColor(sf::Color(accent.r, accent.g, accent.b, static_cast<sf::Uint8>(alpha)));
                window.draw(bloom);
            }
        }

        sf::Uint8 fillB = static_cast<sf::Uint8>(pulse * 15);
        sf::Color fillCol = hov ? sf::Color(18 + fillB, 35 + fillB, 68 + fillB, 255) : sf::Color(10, 12, 22, 245);
        sf::Uint8 borderA = hov ? static_cast<sf::Uint8>(180 + pulse * 75) : static_cast<sf::Uint8>(55 + pulse * 35);
        sf::Color outCol(accent.r, accent.g, accent.b, borderA);

        Draw3DSciFiButton(window, bX, bY, BTN_W, BTN_H, fillCol, outCol, hov ? 2.2f : 1.2f, hov, accent);

        if (hov)
        {
            float streakX = bX + std::fmod(t * 220.f, BTN_W + 60.f) - 30.f;
            sf::RectangleShape streak({ 12.f, BTN_H * 0.75f });
            streak.setPosition(streakX, bY + BTN_H * 0.125f);
            streak.setFillColor(sf::Color(accent.r, accent.g, accent.b, 35));
            window.draw(streak);
            sf::RectangleShape trail({ 5.f, BTN_H * 0.5f });
            trail.setPosition(streakX - 10.f, bY + BTN_H * 0.25f);
            trail.setFillColor(sf::Color(accent.r, accent.g, accent.b, 18));
            window.draw(trail);

            sf::Uint8 barA = static_cast<sf::Uint8>(180.f + pulse * 75.f);
            sf::RectangleShape powerBar({ 4.f, BTN_H - 10.f });
            powerBar.setPosition(bX - 8.f, bY + 5.f);
            powerBar.setFillColor(sf::Color(accent.r, accent.g, accent.b, barA));
            window.draw(powerBar);
            sf::CircleShape tipDot(4.f);
            tipDot.setOrigin(4.f, 4.f);
            tipDot.setPosition(bX - 6.f, bY + 5.f);
            tipDot.setFillColor(sf::Color(accent.r, accent.g, accent.b, barA));
            window.draw(tipDot);

            DrawCornerBrackets(window, bX, bY, BTN_W, BTN_H, accent, 14.f, 2.f);

            sf::CircleShape cornerDot(3.f);
            cornerDot.setOrigin(3.f, 3.f);
            cornerDot.setPosition(bX + BTN_W - 3.f, bY + BTN_H - 3.f);
            cornerDot.setFillColor(accent);
            window.draw(cornerDot);
        }
        else
        {
            sf::Uint8 idleA = static_cast<sf::Uint8>(25 + pulse * 20);
            DrawCornerBrackets(window, bX, bY, BTN_W, BTN_H, sf::Color(accent.r, accent.g, accent.b, idleA), 10.f, 1.f);
        }

        sf::Text txt(menuItems[i], font, 24);
        txt.setFillColor(hov ? Cyber::White : sf::Color(145, 160, 190));
        sf::FloatRect tr = txt.getLocalBounds();
        txt.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
        txt.setPosition(bX + BTN_W / 2.f, bY + BTN_H / 2.f);
        window.draw(txt);
    }

    // ── 5. Cursor Trail (Vệt sáng con trỏ chuột) ────────────────
    struct TrailNode { sf::Vector2f pos; float life; };
    static std::vector<TrailNode> mouseTrail;
    static sf::Clock trailClock;
    float trDt = trailClock.restart().asSeconds();

    sf::Vector2f mPos((float)mp.x, (float)mp.y);
    if (mouseTrail.empty() || std::abs(mouseTrail.back().pos.x - mPos.x) > 2.f || std::abs(mouseTrail.back().pos.y - mPos.y) > 2.f) {
        mouseTrail.push_back({ mPos, 1.0f });
    }

    for (auto it = mouseTrail.begin(); it != mouseTrail.end(); ) {
        it->life -= trDt * 3.f;
        if (it->life <= 0) {
            it = mouseTrail.erase(it);
        }
        else {
            sf::CircleShape dot(it->life * 3.f);
            dot.setOrigin(dot.getRadius(), dot.getRadius());
            dot.setPosition(it->pos);
            dot.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(it->life * 150)));
            window.draw(dot);
            ++it;
        }
    }
}

void DrawInGamePanel(sf::RenderWindow& window, const sf::Font& font, float timeRemaining, bool isPlayerTurn, int gameStatus, int boardSize, GameMode gameMode, int undoLeft[2], int hintLeft[2], float saveNotifTimer, int p1Char, int p2Char, const std::string& p1Name, const std::string& p2Name, sf::Sprite charSprites[4])
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);
    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);

    // --- TÍNH TOÁN ĐỂ CANH GIỮA ---
    float boardLeft = (W - boardW) / 2.f;
    float boardTop = (H - boardW - 130.f) / 2.f + 20.f; // Căn giữa dọc và nhích lên để chừa chỗ cho bảng dưới
    float boardBottom = boardTop + boardW;
    float boardCenterX = W / 2.f;

    const float CARD_W = static_cast<float>(Config::PANEL_W);
    const float CARD_H = 340.f;
    const float pY = boardTop; // Cho 2 thẻ nhân vật ngang hàng với đỉnh bàn cờ

    sf::Color timerColor = (timeRemaining > 20.f) ? Cyber::Cyan : (timeRemaining > 10.f) ? Cyber::Yellow : Cyber::NeonRed;

    // Clock dùng chung cho animation X & O (luôn chạy bất kể lượt ai)
    static sf::Clock symClock;
    float st = symClock.getElapsedTime().asSeconds();
    const float SYM_R = 30.f;  // Bán kính tối đa – vừa khít trong card, không đè tên hay undo dots

    // ==========================================
    // ── THÔNG BÁO LƯU GAME (SAVE ICON / DATA CHIP) ──
    // ==========================================
    if (saveNotifTimer > 0.f) {
        float fade = (saveNotifTimer < 0.45f) ? (saveNotifTimer / 0.45f) : 1.f;
        fade = std::max(0.f, std::min(fade, 1.f));
        float pulse = 0.5f + 0.5f * std::sin(st * 6.8f);
        float slide = (1.f - fade) * 10.f;
        sf::Uint8 a = static_cast<sf::Uint8>(255.f * fade);

        const float panelW = 500.f;
        const float panelH = 118.f;
        const float panelX = boardCenterX - panelW / 2.f;
        const float panelY = H / 2.f - panelH / 2.f - slide;

        // outer hologram glow
        DrawNeonRect(window, panelX - 7.f, panelY - 7.f, panelW + 14.f, panelH + 14.f,
            sf::Color::Transparent,
            sf::Color(60, 255, 150, static_cast<sf::Uint8>((68 + pulse * 72) * fade)),
            3.4f + pulse * 0.8f);

        // main body
        DrawNeonRect(window, panelX, panelY, panelW, panelH,
            sf::Color(7, 16, 24, static_cast<sf::Uint8>(195 * fade)),
            sf::Color(65, 255, 160, static_cast<sf::Uint8>(180 * fade)), 1.8f);
        DrawCornerBrackets(window, panelX, panelY, panelW, panelH,
            sf::Color(0, 255, 220, static_cast<sf::Uint8>(205 * fade)), 14.f, 1.7f);

        // subtle horizontal guide lines
        for (int i = 0; i < 4; ++i) {
            sf::RectangleShape guide({ panelW - 30.f, 1.2f });
            guide.setPosition(panelX + 15.f, panelY + 28.f + i * 21.f);
            guide.setFillColor(sf::Color(120, 255, 220, static_cast<sf::Uint8>(12 * fade)));
            window.draw(guide);
        }

        // header
        DrawSectionHeader(window, font, "[ SAVE COMPLETED ]", panelX + 16.f, panelY + 10.f, panelW - 32.f,
            sf::Color(0, 255, 220, static_cast<sf::Uint8>(220 * fade)));

        sf::CircleShape statDot(4.2f);
        statDot.setOrigin(4.2f, 4.2f);
        statDot.setPosition(panelX + panelW - 22.f, panelY + 18.f);
        statDot.setFillColor(sf::Color(120, 255, 120, static_cast<sf::Uint8>((160 + pulse * 95) * fade)));
        window.draw(statDot);

        // left icon module
        const float iconX = panelX + 22.f;
        const float iconY = panelY + 34.f;
        const float iconW = 82.f;
        const float iconH = 58.f;

        DrawNeonRect(window, iconX, iconY, iconW, iconH,
            sf::Color(10, 28, 24, static_cast<sf::Uint8>(175 * fade)),
            sf::Color(80, 255, 170, static_cast<sf::Uint8>(180 * fade)), 1.4f);
        DrawCornerBrackets(window, iconX, iconY, iconW, iconH,
            sf::Color(90, 255, 180, static_cast<sf::Uint8>(180 * fade)), 8.f, 1.2f);

        // data chip icon
        float chipCx = iconX + iconW / 2.f;
        float chipCy = iconY + iconH / 2.f - 1.f;
        sf::RectangleShape chipCore({ 26.f, 20.f });
        chipCore.setOrigin(13.f, 10.f);
        chipCore.setPosition(chipCx, chipCy);
        chipCore.setFillColor(sf::Color(20, 48, 35, static_cast<sf::Uint8>(220 * fade)));
        chipCore.setOutlineThickness(1.5f);
        chipCore.setOutlineColor(sf::Color(130, 255, 180, static_cast<sf::Uint8>(220 * fade)));
        window.draw(chipCore);

        for (int i = 0; i < 4; ++i) {
            float py = chipCy - 9.f + i * 6.f;
            sf::RectangleShape pinL({ 6.f, 2.f });
            pinL.setPosition(chipCx - 19.f, py);
            pinL.setFillColor(sf::Color(100, 255, 190, static_cast<sf::Uint8>(190 * fade)));
            window.draw(pinL);
            sf::RectangleShape pinR({ 6.f, 2.f });
            pinR.setPosition(chipCx + 13.f, py);
            pinR.setFillColor(sf::Color(100, 255, 190, static_cast<sf::Uint8>(190 * fade)));
            window.draw(pinR);
        }
        for (int i = 0; i < 3; ++i) {
            sf::RectangleShape trace({ 14.f + i * 5.f, 2.f });
            trace.setPosition(chipCx - 8.f, chipCy - 5.f + i * 5.f);
            trace.setFillColor(sf::Color(100, 255, 190, static_cast<sf::Uint8>((120 + pulse * 80) * fade)));
            window.draw(trace);
        }
        sf::CircleShape iconPulse(24.f + pulse * 2.8f, 34);
        iconPulse.setOrigin(iconPulse.getRadius(), iconPulse.getRadius());
        iconPulse.setPosition(chipCx, chipCy);
        iconPulse.setFillColor(sf::Color::Transparent);
        iconPulse.setOutlineThickness(1.8f);
        iconPulse.setOutlineColor(sf::Color(90, 255, 170, static_cast<sf::Uint8>((36 + pulse * 45) * fade)));
        window.draw(iconPulse);

        // save label under icon
        sf::Text iconLabel("DATA CHIP", font, 11);
        iconLabel.setFillColor(sf::Color(120, 255, 180, static_cast<sf::Uint8>(205 * fade)));
        sf::FloatRect ilr = iconLabel.getLocalBounds();
        iconLabel.setOrigin(ilr.left + ilr.width / 2.f, 0.f);
        iconLabel.setPosition(chipCx, iconY + iconH + 4.f);
        window.draw(iconLabel);

        // title block
        float textCX = panelX + 292.f;
        sf::Text glowTitle("GAME SAVED", font, 35);
        glowTitle.setStyle(sf::Text::Bold);
        glowTitle.setFillColor(sf::Color(90, 255, 170, static_cast<sf::Uint8>((100 + pulse * 65) * fade)));
        sf::FloatRect gtr = glowTitle.getLocalBounds();
        glowTitle.setOrigin(gtr.left + gtr.width / 2.f, gtr.top + gtr.height / 2.f);
        glowTitle.setPosition(textCX + 1.7f, panelY + 49.f);
        window.draw(glowTitle);

        sf::Text title("GAME SAVED", font, 35);
        title.setStyle(sf::Text::Bold);
        title.setFillColor(sf::Color(245, 250, 255, a));
        sf::FloatRect tr = title.getLocalBounds();
        title.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
        title.setPosition(textCX, panelY + 47.f);
        window.draw(title);

        DrawCentredText(window, font, "MATCH DATA HAS BEEN SAVED SAFELY", 15,
            sf::Color(130, 255, 190, static_cast<sf::Uint8>(210 * fade)),
            textCX, panelY + 75.f);

        // right-side status chip
        sf::RectangleShape statusChip({ 102.f, 22.f });
        statusChip.setPosition(panelX + panelW - 128.f, panelY + panelH - 31.f);
        statusChip.setFillColor(sf::Color(18, 42, 28, static_cast<sf::Uint8>(178 * fade)));
        statusChip.setOutlineThickness(1.f);
        statusChip.setOutlineColor(sf::Color(90, 255, 140, static_cast<sf::Uint8>(195 * fade)));
        window.draw(statusChip);
        sf::Text statusTxt("ARCHIVED", font, 12);
        statusTxt.setFillColor(sf::Color(120, 255, 160, a));
        statusTxt.setPosition(panelX + panelW - 99.f, panelY + panelH - 28.f);
        window.draw(statusTxt);

        // bottom data bars
        float baseBarY = panelY + panelH - 14.f;
        for (int i = 0; i < 12; ++i) {
            bool active = (i == static_cast<int>(st * 9.f) % 12);
            float hBar = 4.f + ((i % 3) * 2.f) + (active ? 5.f : 0.f);
            sf::RectangleShape b({ 6.f, hBar });
            b.setPosition(panelX + 178.f + i * 10.f, baseBarY - hBar);
            b.setFillColor(i % 2 == 0
                ? sf::Color(0, 255, 220, static_cast<sf::Uint8>((active ? 225 : 155) * fade))
                : sf::Color(110, 255, 120, static_cast<sf::Uint8>((active ? 225 : 155) * fade)));
            window.draw(b);
        }

        // tiny scanning particles around icon
        for (int i = 0; i < 4; ++i) {
            float px = iconX + 8.f + std::fmod(st * (28.f + i * 9.f) + i * 13.f, iconW - 14.f);
            sf::RectangleShape bit({ 4.f, 2.f });
            bit.setPosition(px, iconY - 8.f + i * 4.f);
            bit.setFillColor(sf::Color(100, 255, 180, static_cast<sf::Uint8>(120 * fade)));
            window.draw(bit);
        }
    }

    // ==========================================
    // ── BẢNG BÊN TRÁI (PLAYER 1 - X) ──
    // ==========================================
    float pX_left = (boardLeft - CARD_W) / 2.f;
    if (pX_left < 10.f) pX_left = 10.f;
    bool p1Active = (gameStatus == 0) && isPlayerTurn;
    bool p1Winner = (gameStatus == 1);
    bool drawResult = (gameStatus == 3);
    bool p1Powered = p1Active || p1Winner;
    float winnerPulse = 0.5f + 0.5f * std::sin(st * 4.2f);
    float winnerSweep1 = std::fmod(st * 92.f, CARD_H - 34.f);
    sf::Color p1Col = p1Winner ? sf::Color(80, 255, 255) : (p1Active ? Cyber::Cyan : sf::Color(50, 60, 70, 150));

    if (p1Winner) {
        DrawNeonRect(window, pX_left - 5.f, pY - 5.f, CARD_W + 10.f, CARD_H + 10.f,
            sf::Color::Transparent, sf::Color(80, 255, 255, static_cast<sf::Uint8>(75 + winnerPulse * 100)), 3.4f + winnerPulse * 1.2f);

        sf::RectangleShape pulseFill({ CARD_W - 10.f, CARD_H - 10.f });
        pulseFill.setPosition(pX_left + 5.f, pY + 5.f);
        pulseFill.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(16 + winnerPulse * 28)));
        window.draw(pulseFill);

        sf::RectangleShape sweep({ CARD_W - 24.f, 18.f });
        sweep.setPosition(pX_left + 12.f, pY + 12.f + winnerSweep1);
        sweep.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(10 + winnerPulse * 12)));
        window.draw(sweep);

        float runX = std::fmod(st * 165.f, CARD_W - 56.f);
        float runY = std::fmod(st * 135.f, CARD_H - 56.f);
        sf::RectangleShape edge1({ 42.f, 3.f });
        edge1.setPosition(pX_left + 14.f + runX, pY + 6.f);
        edge1.setFillColor(sf::Color(180, 255, 255, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge1);
        sf::RectangleShape edge2({ 3.f, 42.f });
        edge2.setPosition(pX_left + CARD_W - 9.f, pY + 14.f + runY);
        edge2.setFillColor(sf::Color(180, 255, 255, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge2);
        sf::RectangleShape edge3({ 42.f, 3.f });
        edge3.setPosition(pX_left + CARD_W - 56.f - runX, pY + CARD_H - 9.f);
        edge3.setFillColor(sf::Color(180, 255, 255, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge3);
        sf::RectangleShape edge4({ 3.f, 42.f });
        edge4.setPosition(pX_left + 6.f, pY + CARD_H - 56.f - runY);
        edge4.setFillColor(sf::Color(180, 255, 255, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge4);
    }
    DrawNeonRect(window, pX_left, pY, CARD_W, CARD_H, sf::Color(10, 15, 25, p1Winner ? 225 : 200), p1Col, p1Powered ? 3.f : 1.f);
    if (p1Powered) DrawCornerBrackets(window, pX_left, pY, CARD_W, CARD_H, Cyber::Cyan, 15.f, p1Winner ? 2.4f : 2.f);

    if (p1Char != -1) {
        float avSz = 140.f;
        float avX = pX_left + (CARD_W - avSz) / 2.f, avY = pY + 20.f;
        if (p1Winner) {
            sf::CircleShape avGlow(avSz * 0.42f);
            avGlow.setOrigin(avSz * 0.42f, avSz * 0.42f);
            avGlow.setPosition(avX + avSz / 2.f, avY + avSz / 2.f);
            avGlow.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(18 + winnerPulse * 40)));
            window.draw(avGlow);

            float ringR1 = avSz * 0.38f + winnerPulse * 5.f;
            sf::CircleShape pulseRing1(ringR1, 60);
            pulseRing1.setOrigin(ringR1, ringR1);
            pulseRing1.setPosition(avX + avSz / 2.f, avY + avSz / 2.f);
            pulseRing1.setFillColor(sf::Color::Transparent);
            pulseRing1.setOutlineThickness(1.2f);
            pulseRing1.setOutlineColor(sf::Color(120, 255, 255, static_cast<sf::Uint8>(60 + winnerPulse * 70)));
            window.draw(pulseRing1);

            float ringR2 = avSz * 0.49f + (1.f - winnerPulse) * 4.f;
            sf::CircleShape pulseRing2(ringR2, 60);
            pulseRing2.setOrigin(ringR2, ringR2);
            pulseRing2.setPosition(avX + avSz / 2.f, avY + avSz / 2.f);
            pulseRing2.setFillColor(sf::Color::Transparent);
            pulseRing2.setOutlineThickness(1.f);
            pulseRing2.setOutlineColor(sf::Color(80, 220, 255, static_cast<sf::Uint8>(28 + winnerPulse * 42)));
            window.draw(pulseRing2);
        }
        sf::Color tint = p1Powered ? sf::Color::White : sf::Color(60, 60, 60, 150);
        charSprites[p1Char].setColor(tint);
        sf::FloatRect b = charSprites[p1Char].getLocalBounds();
        charSprites[p1Char].setScale(avSz / b.width, avSz / b.height);
        charSprites[p1Char].setPosition(avX, avY);
        window.draw(charSprites[p1Char]);
        charSprites[p1Char].setColor(sf::Color::White);
        DrawNeonRect(window, avX, avY, avSz, avSz, sf::Color::Transparent, p1Col, p1Winner ? 2.5f : 2.f);
    }
    if (p1Winner) {
        DrawCentredText(window, font, p1Name.empty() ? "PLAYER 1" : p1Name, 24,
            sf::Color(80, 255, 255, static_cast<sf::Uint8>(55 + winnerPulse * 65)), pX_left + CARD_W / 2.f + 1.5f, pY + 185.f);
    }
    DrawCentredText(window, font, p1Name.empty() ? "PLAYER 1" : p1Name, 24,
        p1Winner ? Cyber::White : (p1Active ? Cyber::White : Cyber::Gray), pX_left + CARD_W / 2.f, pY + 185.f);

    // --- VẼ CHỮ X (MECHA CROSSHAIR – REDESIGNED) ---
    float cX1 = pX_left + CARD_W / 2.f;
    float cY1 = pY + 250.f;  // Căn giữa khoảng trống name↔undo
    float pulse1 = (std::sin(st * 2.5f) + 1.f) * 0.5f;

    // Glow nền – luôn hiển thị, chỉ độ sáng khác nhau
    {
        sf::CircleShape glow(SYM_R + 10.f);
        glow.setOrigin(SYM_R + 10.f, SYM_R + 10.f);
        glow.setPosition(cX1, cY1);
        glow.setFillColor(sf::Color(0, 255, 255,
            p1Powered ? static_cast<sf::Uint8>(20 + pulse1 * 40) : 8));
        window.draw(glow);
    }
    if (p1Winner) {
        float haloR = SYM_R + 14.f + winnerPulse * 4.f;
        sf::CircleShape halo(haloR, 64);
        halo.setOrigin(haloR, haloR);
        halo.setPosition(cX1, cY1);
        halo.setFillColor(sf::Color::Transparent);
        halo.setOutlineThickness(1.2f);
        halo.setOutlineColor(sf::Color(80, 255, 255, static_cast<sf::Uint8>(55 + winnerPulse * 90)));
        window.draw(halo);
    }

    // Vòng ngoài – nhịp sáng tối
    {
        sf::CircleShape ring(SYM_R, 64);
        ring.setOrigin(SYM_R, SYM_R);
        ring.setPosition(cX1, cY1);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(1.5f);
        ring.setOutlineColor(sf::Color(0, 255, 255,
            p1Powered ? static_cast<sf::Uint8>(80 + pulse1 * 120) : 40));
        window.draw(ring);
    }

    // 4 vạch tick quay chậm trên vòng ngoài (+20°/s)
    {
        float rotOff = st * 20.f;
        for (int r : {0, 90, 180, 270}) {
            sf::RectangleShape tick({ 9.f, 2.5f });
            tick.setOrigin(-SYM_R + 2.f, 1.25f);
            tick.setPosition(cX1, cY1);
            tick.setRotation(static_cast<float>(r) + rotOff);
            tick.setFillColor(sf::Color(0, 255, 255,
                p1Powered ? static_cast<sf::Uint8>(180 + pulse1 * 75) : 60));
            window.draw(tick);
        }
    }

    // Vòng trong nhỏ
    {
        sf::CircleShape iRing(SYM_R * 0.48f, 64);
        iRing.setOrigin(SYM_R * 0.48f, SYM_R * 0.48f);
        iRing.setPosition(cX1, cY1);
        iRing.setFillColor(sf::Color::Transparent);
        iRing.setOutlineThickness(1.f);
        iRing.setOutlineColor(sf::Color(0, 200, 255,
            p1Powered ? static_cast<sf::Uint8>(50 + pulse1 * 70) : 25));
        window.draw(iRing);
    }

    // 4 cánh X (45°/135°/225°/315°) – vừa khít SYM_R
    {
        float bladeLen = SYM_R * 0.67f;
        float bladeOff = SYM_R * 0.20f;
        sf::Uint8 bAlpha = p1Powered ? 255 : 110;
        for (int rot : {45, 135, 225, 315}) {
            // Cánh chính
            sf::RectangleShape blade({ bladeLen, 7.f });
            blade.setOrigin(-bladeOff, 3.5f);
            blade.setPosition(cX1, cY1);
            blade.setRotation(static_cast<float>(rot));
            blade.setFillColor(sf::Color(0, 255, 255, bAlpha));
            window.draw(blade);

            // Vạch highlight mỏng trên cánh – nhịp sáng khi active
            sf::RectangleShape stripe({ bladeLen * 0.65f, 2.f });
            stripe.setOrigin(-bladeOff - 1.f, -3.f);
            stripe.setPosition(cX1, cY1);
            stripe.setRotation(static_cast<float>(rot));
            stripe.setFillColor(sf::Color(200, 255, 255,
                p1Powered ? static_cast<sf::Uint8>(120 + pulse1 * 135) : 55));
            window.draw(stripe);
        }
    }

    // Tâm năng lượng – nhịp đập
    {
        float cr = p1Powered ? (3.5f + pulse1 * 2.f) : 3.f;
        sf::CircleShape core(cr);
        core.setOrigin(cr, cr);
        core.setPosition(cX1, cY1);
        core.setFillColor(sf::Color(255, 255, 255,
            p1Powered ? static_cast<sf::Uint8>(200 + pulse1 * 55) : 90));
        core.setOutlineThickness(1.5f);
        core.setOutlineColor(sf::Color(0, 255, 255, p1Powered ? 255 : 70));
        window.draw(core);
    }

    // Lượt Undo P1
    float dotsX1 = pX_left + CARD_W / 2.f - (Config::UNDO_MAX * 20.f) / 2.f;
    for (int k = 0; k < Config::UNDO_MAX; ++k) {
        sf::CircleShape dot(6.f); dot.setOrigin(6.f, 6.f); dot.setPosition(dotsX1 + k * 20.f + 10.f, pY + 320.f);
        dot.setFillColor(k < undoLeft[0] ? p1Col : sf::Color(20, 25, 40));
        if (k >= undoLeft[0]) { dot.setOutlineThickness(1.5f); dot.setOutlineColor(sf::Color(50, 60, 90)); }
        window.draw(dot);
    }

    // ==========================================
    // ── BẢNG BÊN PHẢI (PLAYER 2 / AI - O) ──
    // ==========================================
    float pX_right = W - CARD_W - pX_left;
    bool p2Active = (gameStatus == 0) && !isPlayerTurn;
    bool p2Winner = (gameStatus == 2);
    bool p2Powered = p2Active || p2Winner;
    float winnerSweep2 = std::fmod(st * 92.f + 34.f, CARD_H - 34.f);
    sf::Color p2Col = p2Winner ? sf::Color(255, 70, 220) : (p2Active ? Cyber::Magenta : sf::Color(70, 50, 60, 150));

    if (p2Winner) {
        DrawNeonRect(window, pX_right - 5.f, pY - 5.f, CARD_W + 10.f, CARD_H + 10.f,
            sf::Color::Transparent, sf::Color(255, 70, 220, static_cast<sf::Uint8>(75 + winnerPulse * 100)), 3.4f + winnerPulse * 1.2f);

        sf::RectangleShape pulseFill({ CARD_W - 10.f, CARD_H - 10.f });
        pulseFill.setPosition(pX_right + 5.f, pY + 5.f);
        pulseFill.setFillColor(sf::Color(255, 70, 220, static_cast<sf::Uint8>(16 + winnerPulse * 28)));
        window.draw(pulseFill);

        sf::RectangleShape sweep({ CARD_W - 24.f, 18.f });
        sweep.setPosition(pX_right + 12.f, pY + 12.f + winnerSweep2);
        sweep.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(10 + winnerPulse * 12)));
        window.draw(sweep);

        float runX = std::fmod(st * 165.f, CARD_W - 56.f);
        float runY = std::fmod(st * 135.f, CARD_H - 56.f);
        sf::RectangleShape edge1({ 42.f, 3.f });
        edge1.setPosition(pX_right + 14.f + runX, pY + 6.f);
        edge1.setFillColor(sf::Color(255, 180, 245, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge1);
        sf::RectangleShape edge2({ 3.f, 42.f });
        edge2.setPosition(pX_right + CARD_W - 9.f, pY + 14.f + runY);
        edge2.setFillColor(sf::Color(255, 180, 245, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge2);
        sf::RectangleShape edge3({ 42.f, 3.f });
        edge3.setPosition(pX_right + CARD_W - 56.f - runX, pY + CARD_H - 9.f);
        edge3.setFillColor(sf::Color(255, 180, 245, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge3);
        sf::RectangleShape edge4({ 3.f, 42.f });
        edge4.setPosition(pX_right + 6.f, pY + CARD_H - 56.f - runY);
        edge4.setFillColor(sf::Color(255, 180, 245, static_cast<sf::Uint8>(150 + winnerPulse * 90)));
        window.draw(edge4);
    }
    DrawNeonRect(window, pX_right, pY, CARD_W, CARD_H, sf::Color(10, 15, 25, p2Winner ? 225 : 200), p2Col, p2Powered ? 3.f : 1.f);
    if (p2Powered) DrawCornerBrackets(window, pX_right, pY, CARD_W, CARD_H, Cyber::Magenta, 15.f, p2Winner ? 2.4f : 2.f);

    int drawP2Char = (gameMode == GameMode::PVE && p2Char == -1) ? 1 : p2Char;
    if (drawP2Char != -1) {
        float avSz = 140.f;
        float avX = pX_right + (CARD_W - avSz) / 2.f, avY = pY + 20.f;
        if (p2Winner) {
            sf::CircleShape avGlow(avSz * 0.42f);
            avGlow.setOrigin(avSz * 0.42f, avSz * 0.42f);
            avGlow.setPosition(avX + avSz / 2.f, avY + avSz / 2.f);
            avGlow.setFillColor(sf::Color(255, 0, 200, static_cast<sf::Uint8>(18 + winnerPulse * 40)));
            window.draw(avGlow);

            float ringR1 = avSz * 0.38f + winnerPulse * 5.f;
            sf::CircleShape pulseRing1(ringR1, 60);
            pulseRing1.setOrigin(ringR1, ringR1);
            pulseRing1.setPosition(avX + avSz / 2.f, avY + avSz / 2.f);
            pulseRing1.setFillColor(sf::Color::Transparent);
            pulseRing1.setOutlineThickness(1.2f);
            pulseRing1.setOutlineColor(sf::Color(255, 120, 240, static_cast<sf::Uint8>(60 + winnerPulse * 70)));
            window.draw(pulseRing1);

            float ringR2 = avSz * 0.49f + (1.f - winnerPulse) * 4.f;
            sf::CircleShape pulseRing2(ringR2, 60);
            pulseRing2.setOrigin(ringR2, ringR2);
            pulseRing2.setPosition(avX + avSz / 2.f, avY + avSz / 2.f);
            pulseRing2.setFillColor(sf::Color::Transparent);
            pulseRing2.setOutlineThickness(1.f);
            pulseRing2.setOutlineColor(sf::Color(255, 120, 220, static_cast<sf::Uint8>(28 + winnerPulse * 42)));
            window.draw(pulseRing2);
        }
        sf::Color tint = p2Powered ? sf::Color::White : sf::Color(80, 80, 80, 150);
        charSprites[drawP2Char].setColor(tint);
        sf::FloatRect b = charSprites[drawP2Char].getLocalBounds();
        charSprites[drawP2Char].setScale(avSz / b.width, avSz / b.height);
        charSprites[drawP2Char].setPosition(avX, avY);
        window.draw(charSprites[drawP2Char]);
        charSprites[drawP2Char].setColor(sf::Color::White);
        DrawNeonRect(window, avX, avY, avSz, avSz, sf::Color::Transparent, p2Col, p2Winner ? 2.5f : 2.f);
    }
    std::string name2 = (gameMode == GameMode::PVE) ? "AI" : (p2Name.empty() ? "PLAYER 2" : p2Name);
    if (p2Winner) {
        DrawCentredText(window, font, name2, 24,
            sf::Color(255, 120, 235, static_cast<sf::Uint8>(55 + winnerPulse * 65)), pX_right + CARD_W / 2.f + 1.5f, pY + 185.f);
    }
    DrawCentredText(window, font, name2, 24,
        p2Winner ? Cyber::White : (p2Active ? Cyber::White : Cyber::Gray), pX_right + CARD_W / 2.f, pY + 185.f);

    // --- VẼ CHỮ O (RADAR LOCK-ON – REDESIGNED) ---
    float cX2 = pX_right + CARD_W / 2.f;
    float cY2 = pY + 250.f;
    float pulse2 = (std::sin(st * 2.5f + 1.6f) + 1.f) * 0.5f;  // Lệch pha so với X

    // Glow nền – luôn hiển thị
    {
        sf::CircleShape glow(SYM_R + 10.f);
        glow.setOrigin(SYM_R + 10.f, SYM_R + 10.f);
        glow.setPosition(cX2, cY2);
        glow.setFillColor(sf::Color(255, 0, 200,
            p2Powered ? static_cast<sf::Uint8>(20 + pulse2 * 40) : 8));
        window.draw(glow);
    }
    if (p2Winner) {
        float haloR = SYM_R + 14.f + winnerPulse * 4.f;
        sf::CircleShape halo(haloR, 64);
        halo.setOrigin(haloR, haloR);
        halo.setPosition(cX2, cY2);
        halo.setFillColor(sf::Color::Transparent);
        halo.setOutlineThickness(1.2f);
        halo.setOutlineColor(sf::Color(255, 120, 240, static_cast<sf::Uint8>(55 + winnerPulse * 90)));
        window.draw(halo);
    }

    // Vòng ngoài mỏng
    {
        sf::CircleShape outerRing(SYM_R, 64);
        outerRing.setOrigin(SYM_R, SYM_R);
        outerRing.setPosition(cX2, cY2);
        outerRing.setFillColor(sf::Color::Transparent);
        outerRing.setOutlineThickness(1.5f);
        outerRing.setOutlineColor(sf::Color(255, 0, 200,
            p2Powered ? static_cast<sf::Uint8>(60 + pulse2 * 110) : 35));
        window.draw(outerRing);
    }

    // Vòng lõi dày (chữ O chính) – nhịp sáng
    {
        sf::CircleShape innerRing(SYM_R * 0.62f, 64);
        innerRing.setOrigin(SYM_R * 0.62f, SYM_R * 0.62f);
        innerRing.setPosition(cX2, cY2);
        innerRing.setFillColor(sf::Color::Transparent);
        innerRing.setOutlineThickness(7.f);
        innerRing.setOutlineColor(sf::Color(255, 0, 200,
            p2Powered ? static_cast<sf::Uint8>(180 + pulse2 * 75) : 100));
        window.draw(innerRing);
    }

    // Sweep / Quét radar (chỉ active) – 1 vòng / 4 giây
    if (p2Powered) {
        float scanAngle = st * 90.f;
        for (int seg = 0; seg < 10; ++seg) {
            float sLen = SYM_R * 0.58f * (10 - seg) / 10.f;
            sf::RectangleShape ray({ sLen, 2.f });
            ray.setOrigin(0.f, 1.f);
            ray.setPosition(cX2, cY2);
            ray.setRotation(scanAngle - seg * 5.f);
            ray.setFillColor(sf::Color(255, 80, 220,
                static_cast<sf::Uint8>(170 - seg * 15)));
            window.draw(ray);
        }
    }

    // 4 vạch ngắm quay ngược chiều kim đồng hồ (-15°/s)
    {
        float rotOff = -(st * 15.f);
        for (int r : {0, 90, 180, 270}) {
            sf::RectangleShape tick({ 10.f, 3.f });
            tick.setOrigin(-SYM_R * 0.66f, 1.5f);
            tick.setPosition(cX2, cY2);
            tick.setRotation(static_cast<float>(r) + rotOff);
            tick.setFillColor(sf::Color(255, 80, 220,
                p2Powered ? static_cast<sf::Uint8>(200 + pulse2 * 55) : 70));
            window.draw(tick);
        }
    }

    // Tâm năng lượng – nhịp đập
    {
        float cr2 = p2Powered ? (3.5f + pulse2 * 2.f) : 3.f;
        sf::CircleShape core(cr2);
        core.setOrigin(cr2, cr2);
        core.setPosition(cX2, cY2);
        core.setFillColor(sf::Color(255, 255, 255,
            p2Powered ? static_cast<sf::Uint8>(200 + pulse2 * 55) : 90));
        core.setOutlineThickness(1.5f);
        core.setOutlineColor(sf::Color(255, 0, 200, p2Powered ? 255 : 70));
        window.draw(core);
    }

    // Lượt Undo P2
    float dotsX2 = pX_right + CARD_W / 2.f - (Config::UNDO_MAX * 20.f) / 2.f;
    for (int k = 0; k < Config::UNDO_MAX; ++k) {
        sf::CircleShape dot(6.f); dot.setOrigin(6.f, 6.f); dot.setPosition(dotsX2 + k * 20.f + 10.f, pY + 320.f);
        dot.setFillColor(k < undoLeft[1] ? p2Col : sf::Color(20, 25, 40));
        if (k >= undoLeft[1]) { dot.setOutlineThickness(1.5f); dot.setOutlineColor(sf::Color(50, 60, 90)); }
        window.draw(dot);
    }

    // ==========================================
    // ── BẢNG ĐIỀU KHIỂN DƯỚI CÙNG (PANEL) ──
    // ==========================================
    const float BTN_W = 170.f;
    const float BTN_H = 50.f;
    const float BTN_GAP = 22.f;
    float totalBtnsW = 3 * BTN_W + 2 * BTN_GAP; // Tinh tong chieu rong 3 nut

    // Ep chieu rong panel phai du to de chua 4 nut
    float bottomPanelW = std::max(boardW + 40.f, totalBtnsW + 60.f);
    float bottomPanelH = 150.f;
    float bottomPanelX = boardCenterX - bottomPanelW / 2.f;
    float bottomPanelY = std::max(boardBottom, boardTop + CARD_H) + 20.f;

    // Vẽ khung bao bọc
    DrawNeonRect(window, bottomPanelX, bottomPanelY, bottomPanelW, bottomPanelH, sf::Color(10, 15, 25, 200), Cyber::Cyan, 2.f);
    DrawCornerBrackets(window, bottomPanelX, bottomPanelY, bottomPanelW, bottomPanelH, Cyber::Cyan, 15.f, 2.f);

    // 1. Ô Đồng hồ (Thanh Năng Lượng / Time Bar)
    float timerW = 400.f; // Kéo dài thanh ra một chút nhìn cho đã mắt
    float timerH = 35.f;  // Ép lùn lại thành hình dẹt cho giống thanh máu
    float timerX = boardCenterX - timerW / 2.f;
    float timerY = bottomPanelY + 20.f;

    // Giới hạn max time là 60s để tính phần trăm chiều dài thanh
    float maxTime = 60.f;
    float ratio = timeRemaining / maxTime;
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;

    float fillW = timerW * ratio;

    // Vẽ nền rỗng của thanh thời gian
    DrawNeonRect(window, timerX, timerY, timerW, timerH, sf::Color(5, 5, 10, 200), timerColor, 1.5f);

    // Vẽ phần lõi màu (rút ngắn dần từ phải sang trái)
    if (fillW > 0.f) {
        sf::RectangleShape fillBar({ fillW, timerH });
        fillBar.setPosition(timerX, timerY);
        // Để alpha (độ mờ) tầm 100 để nhìn xuyên thấu kiểu Hologram
        fillBar.setFillColor(sf::Color(timerColor.r, timerColor.g, timerColor.b, 100));
        window.draw(fillBar);

        // Vẽ vạch laze trắng chốt chặn ở đầu thanh (Cảm giác tia laze đang quét lùi lại)
        sf::RectangleShape head({ 4.f, timerH });
        head.setPosition(timerX + fillW - 4.f, timerY);
        head.setFillColor(sf::Color(255, 255, 255, 220));
        window.draw(head);
    }

    // Chèn chữ đè lên chính giữa thanh năng lượng
    // Khi chữ đè lên nền màu thì nên dùng màu Trắng (Cyber::White) cho dễ đọc
    DrawCentredText(window, font, "TIME: " + std::to_string((int)timeRemaining) + "s", 22, Cyber::White, boardCenterX, timerY + timerH / 2.f - 2.f);

    // 2. Dan 4 nut (Nam duoi dong ho)
    float startBtnsX = boardCenterX - totalBtnsW / 2.f;
    float btnsY = timerY + timerH + 20.f;

    std::string gameBtns[3] = { "UNDO", "HINT", "SAVE GAME" };
    sf::Color btnColors[] = { Cyber::Cyan, sf::Color(90, 255, 170), Cyber::Yellow };

    int curHintIdx = isPlayerTurn ? 0 : 1;
    bool hintAvailable = (gameStatus == 0 && hintLeft[curHintIdx] > 0 && !(gameMode == GameMode::PVE && !isPlayerTurn));

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    for (int i = 0; i < 3; ++i) {
        float bX = startBtnsX + i * (BTN_W + BTN_GAP);
        bool disabled = (i == 1 && !hintAvailable);
        bool hov = !disabled && (mPos.x >= bX && mPos.x <= bX + BTN_W && mPos.y >= btnsY && mPos.y <= btnsY + BTN_H);

        sf::Color accent = disabled ? sf::Color(70, 80, 95, 140) : btnColors[i];
        sf::Color fill = disabled
            ? sf::Color(12, 14, 20, 135)
            : (hov ? sf::Color(accent.r / 4, accent.g / 4, accent.b / 4, 220) : Cyber::BgBtn);

        Draw3DSciFiButton(window, bX, btnsY, BTN_W, BTN_H, fill, accent, hov ? 2.5f : (disabled ? 0.9f : 1.2f), hov, accent);

        sf::Color textCol = disabled ? sf::Color(85, 95, 110, 155) : (hov ? Cyber::White : sf::Color(175, 210, 200));
        DrawCentredText(window, font, gameBtns[i], 20, textCol, bX + BTN_W / 2.f, btnsY + BTN_H / 2.f);

        if (i == 1 && hintAvailable) {
            float p = 0.5f + 0.5f * std::sin(st * 4.8f);
            DrawCornerBrackets(window, bX - 2.f, btnsY - 2.f, BTN_W + 4.f, BTN_H + 4.f,
                sf::Color(90, 255, 170, static_cast<sf::Uint8>(120 + 95 * p)), 9.f, 1.6f);
        }
        else if (hov) {
            DrawCornerBrackets(window, bX, btnsY, BTN_W, BTN_H, accent, 8.f, 1.5f);
        }
    }

    // ==========================================
    // ── HUD PHÍM TẮT (DƯỚI CÙNG MÀN HÌNH) ──
    // ==========================================
    float hudY = H - 35.f;
    sf::RectangleShape hudLine({ W - 100.f, 1.5f });
    hudLine.setPosition(50.f, hudY - 10.f);
    hudLine.setFillColor(sf::Color(0, 255, 255, 50));
    window.draw(hudLine);


    // ==========================================
    // ── THÔNG BÁO KẾT QUẢ (GLASS HOLOGRAM CARD) ──
    // ==========================================
    static int lastStatus = 0;
    static sf::Clock resultClock;
    if (gameStatus == 0) lastStatus = 0;

    if (gameStatus != 0) {
        if (lastStatus == 0) resultClock.restart();
        lastStatus = gameStatus;

        float rt = resultClock.getElapsedTime().asSeconds();
        float appear = std::min(rt / 0.40f, 1.f);
        float ease = 1.f - (1.f - appear) * (1.f - appear);
        float pulse = 0.5f + 0.5f * std::sin(rt * 3.6f);

        sf::Color resultCol = (gameStatus == 1) ? Cyber::Cyan : ((gameStatus == 2) ? Cyber::Magenta : Cyber::Yellow);
        std::string resultTitle = (gameStatus == 3) ? "DRAW" : "VICTORY";
        std::string winnerText;
        if (gameStatus == 1) winnerText = (p1Name.empty() ? "PLAYER 1" : p1Name) + " IS THE WINNER";
        else if (gameStatus == 2) winnerText = (gameMode == GameMode::PVE ? "AI" : (p2Name.empty() ? "PLAYER 2" : p2Name)) + " IS THE WINNER";
        else winnerText = "NO ONE WINS THIS MATCH";

        // chỉ phủ tối rất nhẹ để vẫn thấy rõ toàn bộ bàn cờ
        sf::RectangleShape softOverlay({ W, H });
        softOverlay.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(8 + ease * 14)));
        window.draw(softOverlay);

        // card đặt trên đầu bàn cờ, không chạm vào board
        float cardW = 500.f;
        float cardH = 84.f;
        float finalX = boardCenterX - cardW / 2.f;
        float finalY = 18.f;
        float cardX = finalX;
        float cardY = -100.f + (finalY + 100.f) * ease;

        sf::Uint8 fillA = static_cast<sf::Uint8>(145 * appear);
        sf::Uint8 edgeA = static_cast<sf::Uint8>(190 * appear);

        // glow ngoài card
        DrawNeonRect(window, cardX - 5.f, cardY - 5.f, cardW + 10.f, cardH + 10.f,
            sf::Color::Transparent,
            sf::Color(resultCol.r, resultCol.g, resultCol.b, static_cast<sf::Uint8>((26 + pulse * 36) * appear)),
            3.f + pulse * 1.2f);

        // body kính hologram: nhiều lớp mờ
        DrawNeonRect(window, cardX, cardY, cardW, cardH,
            sf::Color(6, 10, 18, fillA), sf::Color(resultCol.r, resultCol.g, resultCol.b, edgeA), 1.4f);
        DrawCornerBrackets(window, cardX, cardY, cardW, cardH,
            sf::Color(resultCol.r, resultCol.g, resultCol.b, edgeA), 14.f, 1.6f);

        sf::RectangleShape topGlow({ cardW - 26.f, 10.f });
        topGlow.setPosition(cardX + 13.f, cardY + 10.f);
        topGlow.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(8 * appear)));
        window.draw(topGlow);

        // scan shimmer
        float shimmerX = cardX + 22.f + std::fmod(rt * 180.f, cardW - 70.f);
        sf::RectangleShape shimmer({ 58.f, cardH - 26.f });
        shimmer.setPosition(shimmerX, cardY + 13.f);
        shimmer.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(10 * appear)));
        window.draw(shimmer);

        // header nhỏ
        DrawSectionHeader(window, font, "[ MATCH RESULT ]", cardX + 16.f, cardY + 10.f, cardW - 32.f,
            sf::Color(resultCol.r, resultCol.g, resultCol.b, edgeA));

        sf::CircleShape dot(4.f);
        dot.setOrigin(4.f, 4.f);
        dot.setPosition(cardX + cardW - 24.f, cardY + 19.f);
        dot.setFillColor(sf::Color(resultCol.r, resultCol.g, resultCol.b, static_cast<sf::Uint8>((150 + pulse * 95) * appear)));
        window.draw(dot);

        // title
        sf::Text title(resultTitle, font, 34 + static_cast<unsigned>((1.f - ease) * 4.f));
        title.setStyle(sf::Text::Bold | sf::Text::Italic);
        title.setFillColor(sf::Color(245, 248, 255, static_cast<sf::Uint8>(255 * appear)));
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        title.setPosition(boardCenterX, cardY + 32.f);

        sf::Text titleGlow = title;
        titleGlow.setFillColor(sf::Color(resultCol.r, resultCol.g, resultCol.b, static_cast<sf::Uint8>((90 + pulse * 55) * appear)));
        titleGlow.move(2.f, 0.f);
        window.draw(titleGlow);
        window.draw(title);

        // subtitle
        DrawCentredText(window, font, winnerText, 18,
            sf::Color(resultCol.r, resultCol.g, resultCol.b, static_cast<sf::Uint8>(245 * appear)),
            boardCenterX, cardY + 57.f);

        // thin animated underline
        float lineW = 150.f + pulse * 85.f;
        sf::RectangleShape under({ lineW, 2.f });
        under.setOrigin(lineW / 2.f, 1.f);
        under.setPosition(boardCenterX, cardY + 72.f);
        under.setFillColor(sf::Color(resultCol.r, resultCol.g, resultCol.b, static_cast<sf::Uint8>((90 + pulse * 85) * appear)));
        window.draw(under);
    }

}

void DrawBoard(sf::RenderWindow& window, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);
    float bW = boardW;
    float bH = boardW;
    float ox = (static_cast<float>(Config::WIN_WIDTH) - boardW) / 2.f;
    float oy = (static_cast<float>(Config::WIN_HEIGHT) - boardW - 130.f) / 2.f + 20.f;

    // Dùng Clock tĩnh để tạo hiệu ứng động (Animation) cho bàn cờ
    static sf::Clock boardClock;
    float t = boardClock.getElapsedTime().asSeconds();

    // Tính toán độ chớp (Pulse) từ 0.0 đến 1.0
    float pulse = (std::sin(t * 3.f) + 1.f) * 0.5f;

    // Nền tối bên trong bàn cờ
    DrawNeonRect(window, ox - 2.f, oy - 2.f, bW + 4.f, bH + 4.f, sf::Color(8, 12, 20, 230), Cyber::Cyan, 2.f);

    // Lớp Glow chìm tỏa ra xung quanh viền bàn cờ (Nhịp nhàng theo Pulse)
    DrawNeonRect(window, ox - 5.f, oy - 5.f, bW + 10.f, bH + 10.f, sf::Color::Transparent, sf::Color(0, 255, 255, 30 + static_cast<sf::Uint8>(pulse * 50)), 4.f + pulse * 2.f);

    DrawScanlines(window, ox, oy, bW, bH, sf::Color(0, 0, 0, 80));

    // ── VẼ LƯỚI BÀN CỜ ──
    for (int i = 0; i <= boardSize; ++i) {
        bool edge = (i == 0 || i == boardSize); // Đường viền ngoài cùng

        // Trục Ngang
        sf::RectangleShape hGlow({ bW, edge ? 4.f : 2.f });
        hGlow.setPosition(ox, oy + i * cellSz - hGlow.getSize().y / 2.f);
        hGlow.setFillColor(edge ? sf::Color(0, 255, 255, 100) : sf::Color(0, 150, 200, 50));
        window.draw(hGlow);

        sf::RectangleShape hLine({ bW, edge ? 2.f : 1.f });
        hLine.setPosition(ox, oy + i * cellSz - hLine.getSize().y / 2.f);
        hLine.setFillColor(edge ? Cyber::Cyan : sf::Color(60, 120, 160, 200));
        window.draw(hLine);

        // Trục Dọc
        sf::RectangleShape vGlow({ edge ? 4.f : 2.f, bH });
        vGlow.setPosition(ox + i * cellSz - vGlow.getSize().x / 2.f, oy);
        vGlow.setFillColor(edge ? sf::Color(0, 255, 255, 100) : sf::Color(0, 150, 200, 50));
        window.draw(vGlow);

        sf::RectangleShape vLine({ edge ? 2.f : 1.f, bH });
        vLine.setPosition(ox + i * cellSz - vLine.getSize().x / 2.f, oy);
        vLine.setFillColor(edge ? Cyber::Cyan : sf::Color(60, 120, 160, 200));
        window.draw(vLine);
    }

    // ĐÃ XÓA TOÀN BỘ CÁC CHẤM TỌA ĐỘ THEO YÊU CẦU CỦA M

    // ── VẼ TIA SÁNG CHẠY QUANH VIỀN (RUNNING LIGHTS) ──
    auto drawRunningLight = [&](float offsetTime) {
        float speed = 600.f; // Tốc độ chạy 600 pixel / giây
        float perimeter = (bW + bH) * 2.f;
        float distance = std::fmod((t + offsetTime) * speed, perimeter);

        float lx, ly, lw, lh;
        if (distance < bW) { // Cạnh trên (chạy sang phải)
            lx = ox + distance; ly = oy; lw = 60.f; lh = 4.f;
        }
        else if (distance < bW + bH) { // Cạnh phải (chạy xuống)
            lx = ox + bW; ly = oy + (distance - bW); lw = 4.f; lh = 60.f;
        }
        else if (distance < 2.f * bW + bH) { // Cạnh dưới (chạy sang trái)
            lx = ox + bW - (distance - bW - bH); ly = oy + bH; lw = 60.f; lh = 4.f;
        }
        else { // Cạnh trái (chạy lên)
            lx = ox; ly = oy + bH - (distance - 2.f * bW - bH); lw = 4.f; lh = 60.f;
        }

        // Lõi trắng
        sf::RectangleShape light({ lw, lh });
        light.setOrigin(lw / 2.f, lh / 2.f);
        light.setPosition(lx, ly);
        light.setFillColor(sf::Color::White);

        // Glow xanh tỏa ra
        sf::RectangleShape glow({ lw * 1.5f, lh * 1.5f });
        glow.setOrigin(lw * 1.5f / 2.f, lh * 1.5f / 2.f);
        glow.setPosition(lx, ly);
        glow.setFillColor(sf::Color(0, 255, 255, 180));

        window.draw(glow);
        window.draw(light);
        };

    // Tạo 2 luồng sáng chạy rượt đuổi nhau
    drawRunningLight(0.f);
    drawRunningLight(2.5f); // Luồng thứ 2 chạy lệch 2.5 giây

    // ── 4 Ngàm cơ khí ở 4 góc bàn cờ (Co giãn nhẹ theo nhịp Pulse) ──
    float expand = pulse * 3.f; // Giật ra vô 3 pixel
    DrawCornerBrackets(window, ox - 6.f - expand, oy - 6.f - expand, bW + 12.f + expand * 2.f, bH + 12.f + expand * 2.f, Cyber::Cyan, 25.f, 3.f);
}

void DrawPieces(sf::RenderWindow& window, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);

    float ox = (static_cast<float>(Config::WIN_WIDTH) - boardW) / 2.f;
    float oy = (static_cast<float>(Config::WIN_HEIGHT) - boardW - 130.f) / 2.f + 20.f;

    static sf::Clock fxClock;
    float st = fxClock.getElapsedTime().asSeconds();
    float basePulse = 0.5f + 0.5f * std::sin(st * 8.5f);

    int sx = -1, sy = -1, ex = -1, ey = -1;
    GetWinLine(&sx, &sy, &ex, &ey);

    auto signi = [](int v) -> int { return (v > 0) - (v < 0); };
    int stepX = signi(ex - sx);
    int stepY = signi(ey - sy);
    int winCount = (sx >= 0 && sy >= 0 && ex >= 0 && ey >= 0) ? std::max(std::abs(ex - sx), std::abs(ey - sy)) + 1 : 0;

    // Fix load/chơi tiếp save: win line trong caro_logic đôi khi còn lưu từ ván trước.
    // Chỉ bật hiệu ứng thắng nếu đường win hiện tại thật sự là một chuỗi cùng loại trên bàn cờ.
    bool winLineValid = false;
    if (winCount >= 5 && sx >= 0 && sy >= 0 && ex >= 0 && ey >= 0 &&
        sx < boardSize && sy < boardSize && ex < boardSize && ey < boardSize &&
        (stepX != 0 || stepY != 0))
    {
        int owner = GetCell(sx, sy);
        if (owner != 0) {
            winLineValid = true;
            int cx = sx, cy = sy;
            for (int i = 0; i < winCount; ++i) {
                if (cx < 0 || cx >= boardSize || cy < 0 || cy >= boardSize || GetCell(cx, cy) != owner) {
                    winLineValid = false;
                    break;
                }
                cx += stepX;
                cy += stepY;
            }
        }
    }

    if (!winLineValid) {
        winCount = 0;
    }

    int activeIdx = winCount > 0 ? (static_cast<int>(st * 5.0f) % winCount) : -1;

    auto getWinIndex = [&](int gx, int gy) -> int {
        if (winCount <= 0 || !winLineValid) return -1;
        int cx = sx, cy = sy;
        for (int i = 0; i < winCount; ++i) {
            if (cx == gx && cy == gy) return i;
            cx += stepX;
            cy += stepY;
        }
        return -1;
        };

    for (int x = 0; x < boardSize; ++x) {
        for (int y = 0; y < boardSize; ++y) {
            int cell = GetCell(x, y);
            if (!cell) continue;

            int winIdx = getWinIndex(x, y);
            bool isWinningCell = (winIdx >= 0);
            float seqBoost = 0.f;
            if (isWinningCell && winCount > 0) {
                int dist = std::abs(winIdx - activeIdx);
                if (dist == 0) seqBoost = 1.0f;
                else if (dist == 1) seqBoost = 0.45f;
                else seqBoost = 0.14f;
            }

            float pulse = isWinningCell ? (0.35f + 0.65f * seqBoost) * (0.65f + 0.35f * basePulse) : basePulse;
            float cx = ox + x * cellSz + cellSz / 2.f;
            float cy = oy + y * cellSz + cellSz / 2.f;
            float arm = cellSz / 2.f - 6.f;

            // Highlight ô thắng bằng bracket + glow ô, không dùng gạch ngang
            if (isWinningCell) {
                sf::RectangleShape cellGlow({ static_cast<float>(cellSz) - 6.f, static_cast<float>(cellSz) - 6.f });
                cellGlow.setOrigin(cellGlow.getSize().x / 2.f, cellGlow.getSize().y / 2.f);
                cellGlow.setPosition(cx, cy);
                if (cell == 1) {
                    cellGlow.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(8 + 26 * seqBoost + 10 * basePulse)));
                }
                else {
                    cellGlow.setFillColor(sf::Color(255, 0, 200, static_cast<sf::Uint8>(8 + 26 * seqBoost + 10 * basePulse)));
                }
                window.draw(cellGlow);

                DrawCornerBrackets(window,
                    cx - (cellSz - 6.f) / 2.f,
                    cy - (cellSz - 6.f) / 2.f,
                    static_cast<float>(cellSz) - 6.f,
                    static_cast<float>(cellSz) - 6.f,
                    cell == 1
                    ? sf::Color(120, 255, 255, static_cast<sf::Uint8>(90 + 120 * seqBoost))
                    : sf::Color(255, 170, 245, static_cast<sf::Uint8>(90 + 120 * seqBoost)),
                    6.f + 3.f * seqBoost,
                    1.5f + 1.2f * seqBoost);
            }

            if (cell == 1) {
                // ── QUÂN X (CYAN) ──
                sf::Uint8 glowA = isWinningCell ? static_cast<sf::Uint8>(80 + 120 * seqBoost + 40 * basePulse) : 60;
                sf::Uint8 coreA = isWinningCell ? static_cast<sf::Uint8>(210 + 45 * seqBoost) : 255;

                // hiệu ứng thắng: chớp đúng hình chữ X và sáng lần lượt từng ô
                if (isWinningCell) {
                    for (int rot : {45, -45}) {
                        sf::RectangleShape pulseGlow({ arm * 2.32f, 8.f + 4.f * seqBoost + basePulse * 1.2f });
                        pulseGlow.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(55 + 120 * seqBoost)));
                        pulseGlow.setOrigin(pulseGlow.getSize().x / 2.f, pulseGlow.getSize().y / 2.f);
                        pulseGlow.setPosition(cx, cy);
                        pulseGlow.setRotation(static_cast<float>(rot));
                        window.draw(pulseGlow);
                    }

                    sf::CircleShape centerGlow(3.5f + 3.f * seqBoost, 28);
                    centerGlow.setOrigin(centerGlow.getRadius(), centerGlow.getRadius());
                    centerGlow.setPosition(cx, cy);
                    centerGlow.setFillColor(sf::Color(180, 255, 255, static_cast<sf::Uint8>(50 + 120 * seqBoost)));
                    window.draw(centerGlow);
                }

                for (int rot : {45, -45}) {
                    sf::RectangleShape gl({ arm * 2.f, isWinningCell ? (6.5f + 2.f * seqBoost) : 6.f });
                    gl.setFillColor(sf::Color(0, 255, 255, glowA));
                    gl.setOrigin(arm, gl.getSize().y / 2.f);
                    gl.setPosition(cx, cy);
                    gl.setRotation(static_cast<float>(rot));
                    window.draw(gl);
                }

                for (int rot : {45, -45}) {
                    sf::RectangleShape ln({ arm * 2.f, isWinningCell ? (3.2f + 1.2f * seqBoost) : 3.f });
                    ln.setFillColor(sf::Color(220, 255, 255, coreA));
                    ln.setOrigin(arm, ln.getSize().y / 2.f);
                    ln.setPosition(cx, cy);
                    ln.setRotation(static_cast<float>(rot));
                    window.draw(ln);
                }
            }
            else if (cell == 2) {
                // ── QUÂN O (MAGENTA) ──
                sf::Uint8 glowA = isWinningCell ? static_cast<sf::Uint8>(80 + 120 * seqBoost + 40 * basePulse) : 60;
                sf::Uint8 ringA = isWinningCell ? static_cast<sf::Uint8>(215 + 35 * seqBoost) : 255;

                // hiệu ứng thắng: chớp đúng hình chữ O và sáng lần lượt từng ô
                if (isWinningCell) {
                    float pulseR = arm + 1.5f + 2.8f * seqBoost;
                    sf::CircleShape pulseGlow(pulseR, 50);
                    pulseGlow.setOrigin(pulseR, pulseR);
                    pulseGlow.setPosition(cx, cy);
                    pulseGlow.setFillColor(sf::Color::Transparent);
                    pulseGlow.setOutlineThickness(5.5f + 2.2f * seqBoost);
                    pulseGlow.setOutlineColor(sf::Color(255, 90, 225, static_cast<sf::Uint8>(60 + 120 * seqBoost)));
                    window.draw(pulseGlow);

                    sf::CircleShape pulseCore(arm, 50);
                    pulseCore.setOrigin(arm, arm);
                    pulseCore.setPosition(cx, cy);
                    pulseCore.setFillColor(sf::Color::Transparent);
                    pulseCore.setOutlineThickness(3.2f + 1.3f * seqBoost);
                    pulseCore.setOutlineColor(sf::Color(255, 210, 245, static_cast<sf::Uint8>(135 + 95 * seqBoost)));
                    window.draw(pulseCore);
                }

                sf::CircleShape glow(arm + 1.f + (isWinningCell ? 1.2f * seqBoost : 0.f));
                glow.setOrigin(glow.getRadius(), glow.getRadius());
                glow.setPosition(cx, cy);
                glow.setFillColor(sf::Color::Transparent);
                glow.setOutlineThickness(isWinningCell ? (5.5f + 1.8f * seqBoost) : 5.f);
                glow.setOutlineColor(sf::Color(255, 0, 200, glowA));
                window.draw(glow);

                sf::CircleShape ring(arm);
                ring.setOrigin(arm, arm);
                ring.setPosition(cx, cy);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(isWinningCell ? (2.8f + 1.1f * seqBoost) : 2.5f);
                ring.setOutlineColor(sf::Color(255, 180, 245, ringA));
                window.draw(ring);

                sf::CircleShape dot(isWinningCell ? (3.f + 1.4f * seqBoost) : 3.f);
                dot.setOrigin(dot.getRadius(), dot.getRadius());
                dot.setPosition(cx, cy);
                dot.setFillColor(sf::Color(255, 170, 235, isWinningCell ? static_cast<sf::Uint8>(165 + 80 * seqBoost) : 255));
                window.draw(dot);
            }
        }
    }
}

// ============================================================
//  DrawHoverEffect
// ============================================================
void DrawHoverEffect(sf::RenderWindow& window, int gX, int gY, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);

    float ox = (static_cast<float>(Config::WIN_WIDTH) - boardW) / 2.f;
    float oy = (static_cast<float>(Config::WIN_HEIGHT) - boardW - 130.f) / 2.f + 20.f;

    if (gX >= 0 && gX < boardSize && gY >= 0 && gY < boardSize) {
        float rx = ox + gX * cellSz;
        float ry = oy + gY * cellSz;

        int cellState = GetCell(gX, gY);

        sf::Color fillCol = sf::Color(0, 200, 255, 22);
        sf::Color borderCol = sf::Color(0, 255, 255, 180);

        if (cellState != 0) {
            fillCol = sf::Color(255, 220, 0, 25);   // Vàng nền mờ
            borderCol = sf::Color(255, 220, 0, 200); // Vàng viền rõ
        }

        sf::RectangleShape hr({ static_cast<float>(cellSz), static_cast<float>(cellSz) });
        hr.setPosition(rx, ry);
        hr.setFillColor(fillCol);
        window.draw(hr);

        DrawCornerBrackets(window, rx + 2.f, ry + 2.f, static_cast<float>(cellSz) - 4.f, static_cast<float>(cellSz) - 4.f, borderCol, 6.f, 1.5f);
    }
}


void DrawHintEffect(sf::RenderWindow& window, const sf::Font& font, int gX, int gY, int boardSize)
{
    if (gX < 0 || gY < 0 || gX >= boardSize || gY >= boardSize) return;
    if (GetCell(gX, gY) != 0) return;

    static sf::Clock hintClock;
    float t = hintClock.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 5.0f);

    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);
    float ox = (static_cast<float>(Config::WIN_WIDTH) - boardW) / 2.f;
    float oy = (static_cast<float>(Config::WIN_HEIGHT) - boardW - 130.f) / 2.f + 20.f;

    float rx = ox + gX * cellSz;
    float ry = oy + gY * cellSz;
    float cx = rx + cellSz / 2.f;
    float cy = ry + cellSz / 2.f;

    sf::RectangleShape fill({ static_cast<float>(cellSz), static_cast<float>(cellSz) });
    fill.setPosition(rx, ry);
    fill.setFillColor(sf::Color(90, 255, 170, static_cast<sf::Uint8>(28 + pulse * 35)));
    window.draw(fill);

    DrawCornerBrackets(window, rx + 2.f, ry + 2.f,
        static_cast<float>(cellSz) - 4.f,
        static_cast<float>(cellSz) - 4.f,
        sf::Color(90, 255, 170, static_cast<sf::Uint8>(190 + pulse * 65)),
        7.f, 1.8f);

    sf::CircleShape ring(cellSz * (0.28f + pulse * 0.08f), 48);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(cx, cy);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.f);
    ring.setOutlineColor(sf::Color(90, 255, 170, static_cast<sf::Uint8>(150 + pulse * 85)));
    window.draw(ring);

    sf::Text h("H", font, static_cast<unsigned>(std::max(13, cellSz / 2)));
    h.setStyle(sf::Text::Bold);
    h.setFillColor(sf::Color(220, 255, 235, static_cast<sf::Uint8>(210 + pulse * 45)));
    sf::FloatRect hb = h.getLocalBounds();
    h.setOrigin(hb.left + hb.width / 2.f, hb.top + hb.height / 2.f);
    h.setPosition(cx, cy - 1.f);
    window.draw(h);
}

static void DrawVirusSkullIcon(sf::RenderWindow& window, float cx, float cy, float cellSz, float pulse)
{
    sf::Color boneCol(135, 255, 140, static_cast<sf::Uint8>(215 + pulse * 35));
    sf::Color glowCol(40, 220, 70, static_cast<sf::Uint8>(85 + pulse * 45));
    sf::Color darkCol(3, 16, 6, 235);

    sf::CircleShape glow(cellSz * (0.24f + pulse * 0.04f), 32);
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(cx, cy);
    glow.setFillColor(glowCol);
    window.draw(glow);

    sf::RectangleShape bone1({ cellSz * 0.30f, cellSz * 0.055f });
    bone1.setOrigin(bone1.getSize().x / 2.f, bone1.getSize().y / 2.f);
    bone1.setPosition(cx, cy + cellSz * 0.06f);
    bone1.setRotation(35.f);
    bone1.setFillColor(sf::Color(80, 235, 105, 170));
    window.draw(bone1);

    sf::RectangleShape bone2({ cellSz * 0.30f, cellSz * 0.055f });
    bone2.setOrigin(bone2.getSize().x / 2.f, bone2.getSize().y / 2.f);
    bone2.setPosition(cx, cy + cellSz * 0.06f);
    bone2.setRotation(-35.f);
    bone2.setFillColor(sf::Color(80, 235, 105, 170));
    window.draw(bone2);

    sf::CircleShape head(cellSz * 0.18f, 32);
    head.setOrigin(head.getRadius(), head.getRadius());
    head.setPosition(cx, cy - cellSz * 0.045f);
    head.setFillColor(boneCol);
    window.draw(head);

    sf::RectangleShape jaw({ cellSz * 0.19f, cellSz * 0.12f });
    jaw.setOrigin(jaw.getSize().x / 2.f, jaw.getSize().y / 2.f);
    jaw.setPosition(cx, cy + cellSz * 0.09f);
    jaw.setFillColor(boneCol);
    window.draw(jaw);

    sf::CircleShape eyeL(cellSz * 0.04f, 18);
    eyeL.setOrigin(eyeL.getRadius(), eyeL.getRadius());
    eyeL.setPosition(cx - cellSz * 0.065f, cy - cellSz * 0.065f);
    eyeL.setFillColor(darkCol);
    window.draw(eyeL);

    sf::CircleShape eyeR(cellSz * 0.04f, 18);
    eyeR.setOrigin(eyeR.getRadius(), eyeR.getRadius());
    eyeR.setPosition(cx + cellSz * 0.065f, cy - cellSz * 0.065f);
    eyeR.setFillColor(darkCol);
    window.draw(eyeR);

    sf::ConvexShape nose;
    nose.setPointCount(3);
    nose.setPoint(0, { 0.f, 0.f });
    nose.setPoint(1, { cellSz * 0.035f, cellSz * 0.065f });
    nose.setPoint(2, { -cellSz * 0.035f, cellSz * 0.065f });
    nose.setPosition(cx, cy - cellSz * 0.005f);
    nose.setFillColor(darkCol);
    window.draw(nose);

    for (int i = -1; i <= 1; ++i)
    {
        sf::RectangleShape tooth({ cellSz * 0.018f, cellSz * 0.055f });
        tooth.setOrigin(tooth.getSize().x / 2.f, tooth.getSize().y / 2.f);
        tooth.setPosition(cx + i * cellSz * 0.038f, cy + cellSz * 0.105f);
        tooth.setFillColor(darkCol);
        window.draw(tooth);
    }
}

static void DrawVirusToxicSmoke(sf::RenderWindow& window, float rx, float ry, float cellSz, float t, int seed)
{
    for (int i = 0; i < 5; ++i)
    {
        float phase = std::fmod(t * (0.55f + 0.08f * i) + seed * 0.17f + i * 0.21f, 1.0f);
        float px = rx + cellSz * (0.18f + 0.16f * i + 0.05f * std::sin(t * 2.1f + i));
        float py = ry + cellSz * (0.88f - phase * 0.95f);
        float r = cellSz * (0.035f + phase * 0.055f);
        sf::Uint8 alpha = static_cast<sf::Uint8>(95 * (1.0f - phase));

        sf::CircleShape smoke(r, 20);
        smoke.setOrigin(r, r);
        smoke.setPosition(px, py);
        smoke.setFillColor(sf::Color(70, 255, 95, alpha));
        window.draw(smoke);
    }
}

void DrawVirusCells(sf::RenderWindow& window, const sf::Font& font, int boardSize)
{
    static sf::Clock virusClock;
    float t = virusClock.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);

    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);
    float ox = (static_cast<float>(Config::WIN_WIDTH) - boardW) / 2.f;
    float oy = (static_cast<float>(Config::WIN_HEIGHT) - boardW - 130.f) / 2.f + 20.f;

    for (int x = 0; x < boardSize; ++x)
    {
        for (int y = 0; y < boardSize; ++y)
        {
            int ttl = GetVirusCell(x, y);
            if (ttl <= 0) continue;

            float rx = ox + x * cellSz;
            float ry = oy + y * cellSz;
            float cx = rx + cellSz / 2.f;
            float cy = ry + cellSz / 2.f;

            DrawVirusToxicSmoke(window, rx, ry, static_cast<float>(cellSz), t, x * 31 + y * 17);

            sf::RectangleShape fill({ static_cast<float>(cellSz), static_cast<float>(cellSz) });
            fill.setPosition(rx, ry);
            fill.setFillColor(sf::Color(20, 85, 30, static_cast<sf::Uint8>(42 + pulse * 48)));
            window.draw(fill);

            sf::RectangleShape scan({ static_cast<float>(cellSz), 2.f });
            scan.setPosition(rx, ry + cellSz * std::fmod(t * 0.85f + (x + y) * 0.09f, 1.0f));
            scan.setFillColor(sf::Color(110, 255, 130, 105));
            window.draw(scan);

            DrawCornerBrackets(window,
                rx + 2.f, ry + 2.f,
                static_cast<float>(cellSz) - 4.f,
                static_cast<float>(cellSz) - 4.f,
                sf::Color(90, 255, 120, static_cast<sf::Uint8>(175 + pulse * 70)),
                7.f, 1.8f);

            sf::RectangleShape inner({ static_cast<float>(cellSz - 8), static_cast<float>(cellSz - 8) });
            inner.setPosition(rx + 4.f, ry + 4.f);
            inner.setFillColor(sf::Color::Transparent);
            inner.setOutlineThickness(1.f);
            inner.setOutlineColor(sf::Color(110, 255, 140, static_cast<sf::Uint8>(85 + pulse * 55)));
            window.draw(inner);

            DrawVirusSkullIcon(window, cx, cy, static_cast<float>(cellSz), pulse);

            // Khong hien so tren o virus nua, chi giu icon dau lau + hieu ung doc.
        }
    }
}

void DrawVirusStatusBadge(sf::RenderWindow& window, const sf::Font& font)
{
    bool enabled = false;
    int active = 0;
    int counter = 0;
    GetVirusInfo(&enabled, &active, &counter);
    if (!enabled) return;

    float W = static_cast<float>(Config::WIN_WIDTH);
    float x = W / 2.f - 240.f;
    float y = 72.f;
    float w = 430.f;
    float h = 34.f;

    static sf::Clock badgeClock;
    float t = badgeClock.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 5.0f);

    DrawNeonRect(window, x, y, w, h,
        sf::Color(35, 8, 24, 220),
        sf::Color(255, 60, 130, static_cast<sf::Uint8>(160 + pulse * 80)),
        1.5f);
    DrawCornerBrackets(window, x, y, w, h, sf::Color(255, 70, 140, 210), 8.f, 1.3f);

    int threat = GetVirusThreatLevel();
    int maxCells = GetVirusMaxCells();
    std::string txt = "VIRUS Lv" + std::to_string(threat) +
        "  |  ACTIVE " + std::to_string(active) + "/" + std::to_string(maxCells) +
        "  |  NEXT " + std::to_string(4 - counter) +
        "  |  WAVE +" + std::to_string(threat);
    DrawCentredText(window, font, txt, 15, sf::Color(255, 205, 225), x + w / 2.f, y + h / 2.f);
}


void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY) { DrawWinLine(window, sX, sY, eX, eY, 15); }

void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY, int boardSize)
{
    // Không vẽ gạch ngang qua chuỗi thắng nữa.
    // Hiệu ứng thắng được thể hiện trực tiếp trong DrawPieces:
    // glow ô, bracket từng ô, và nhịp sáng chạy lần lượt qua 5 quân thắng.
}


void DrawSettings(sf::RenderWindow& window, const sf::Font& font, int boardSize, bool ruleBlock2, int aiLevel, float sfxVolume, bool bgmEnabled, bool virusMode, bool audioOnly)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    static sf::Clock setAnimClk;
    float t = setAnimClk.getElapsedTime().asSeconds();

    for (float gx = 0; gx < W; gx += 60.f) {
        sf::RectangleShape vl({ 1.f, H });
        vl.setPosition(gx, 0);
        vl.setFillColor(sf::Color(0, 255, 255, 6));
        window.draw(vl);
    }
    for (float gy = 0; gy < H; gy += 60.f) {
        sf::RectangleShape hl({ W, 1.f });
        hl.setPosition(0, gy);
        hl.setFillColor(sf::Color(0, 255, 255, 6));
        window.draw(hl);
    }
    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 14));

    sf::CircleShape radar(400.f, 64);
    radar.setOrigin(400.f, 400.f);
    radar.setPosition(W / 2.f, H / 2.f);
    radar.setFillColor(sf::Color::Transparent);
    radar.setOutlineThickness(2.f);
    radar.setOutlineColor(sf::Color(0, 255, 255, 15));
    window.draw(radar);

    for (int i = 0; i < 3; i++) {
        sf::CircleShape ring(300.f + i * 40.f, 64);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(W / 2.f, H / 2.f);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(i == 1 ? 8.f : 1.f);
        ring.setOutlineColor(sf::Color(0, 255, 255, i == 1 ? 8 : 15));
        window.draw(ring);
    }

    float rotAngle = t * 15.f;
    for (int r : {0, 90, 180, 270}) {
        sf::RectangleShape ray({ 800.f, 2.f });
        ray.setOrigin(400.f, 1.f);
        ray.setPosition(W / 2.f, H / 2.f);
        ray.setRotation(r + rotAngle);
        ray.setFillColor(sf::Color(0, 255, 255, 10));
        window.draw(ray);
    }

    float panelW = 900.f;
    float panelH = 540.f;
    float pX = W / 2.f - panelW / 2.f;
    float pY = 160.f;

    DrawNeonRect(window, pX, pY, panelW, panelH, sf::Color(8, 12, 18, 230), sf::Color::Transparent, 0.f);
    DrawNeonRect(window, pX, pY, panelW, panelH, sf::Color::Transparent, sf::Color(0, 255, 255, 80), 1.5f);

    float pulse = (std::sin(t * 3.f) + 1.f) * 0.5f;
    float expand = 5.f + 5.f * pulse;
    DrawCornerBrackets(window, pX - expand, pY - expand, panelW + 2 * expand, panelH + 2 * expand, sf::Color(0, 255, 255, 200), 40.f, 4.f);

    float scanY = pY + std::fmod(t * 250.f, panelH);
    sf::RectangleShape scan({ panelW, 2.f });
    scan.setPosition(pX, scanY);
    scan.setFillColor(sf::Color(0, 255, 255, 120));
    window.draw(scan);

    sf::RectangleShape scanGlow({ panelW, 40.f });
    scanGlow.setPosition(pX, scanY - 40.f);
    scanGlow.setFillColor(sf::Color(0, 255, 255, 15));
    window.draw(scanGlow);

    for (int b = 0; b < 18; ++b) {
        float bW = 2.f + ((b * 7) % 6);
        sf::RectangleShape bar({ bW, 8.f });
        bar.setPosition(pX + panelW - 180.f + b * 9.f, pY + panelH - 12.f);
        bar.setFillColor(sf::Color(0, 255, 255, 120));
        window.draw(bar);
    }
    DrawCentredText(window, font, "SYS.CFG // SECURE", 12, sf::Color(0, 255, 255, 120), pX + 80.f, pY + panelH - 12.f);

    float titlePulse = (std::sin(t * 5.f) + 1.f) * 0.5f;
    std::string titleStr = "SYSTEM SETTINGS";

    sf::Text tTitle(titleStr, font, 50);
    tTitle.setStyle(sf::Text::Bold);
    sf::FloatRect tr = tTitle.getLocalBounds();
    tTitle.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
    tTitle.setFillColor(sf::Color(255, 0, 50, 150));
    tTitle.setPosition(W / 2.f - 3.f, 90.f);
    window.draw(tTitle);
    tTitle.setFillColor(sf::Color(0, 255, 255, 150));
    tTitle.setPosition(W / 2.f + 3.f, 90.f);
    window.draw(tTitle);
    tTitle.setFillColor(Cyber::Yellow);
    tTitle.setPosition(W / 2.f, 90.f);
    window.draw(tTitle);

    float arrowOffset = 25.f + titlePulse * 15.f;
    DrawCentredText(window, font, ">>", 40, Cyber::Cyan, W / 2.f - tr.width / 2.f - arrowOffset - 30.f, 85.f);
    DrawCentredText(window, font, "<<", 40, Cyber::Cyan, W / 2.f + tr.width / 2.f + arrowOffset + 30.f, 85.f);

    const float LX = pX + 60.f;
    const float CX = pX + 450.f;
    const float RG = 68.f;
    float startY = pY + 54.f;

    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));

    auto drawPill = [&](float px, float py, float pw, float ph, sf::Color fill, sf::Color out) {
        float pr = ph / 2.f;
        sf::CircleShape oL(pr + 2.f); oL.setPosition(px - 2.f, py - 2.f); oL.setFillColor(out); window.draw(oL);
        sf::CircleShape oR(pr + 2.f); oR.setPosition(px + pw - ph + 2.f, py - 2.f); oR.setFillColor(out); window.draw(oR);
        sf::RectangleShape oM({ pw - ph, ph + 4.f }); oM.setPosition(px + pr, py - 2.f); oM.setFillColor(out); window.draw(oM);
        sf::CircleShape cL(pr); cL.setPosition(px, py); cL.setFillColor(fill); window.draw(cL);
        sf::CircleShape cR(pr); cR.setPosition(px + pw - ph, py); cR.setFillColor(fill); window.draw(cR);
        sf::RectangleShape rM({ pw - ph, ph }); rM.setPosition(px + pr, py); rM.setFillColor(fill); window.draw(rM);
        };

    auto drawSlider = [&](const std::string& label, float cy, float currentV, float minV, float maxV, bool enabled) {
        sf::Text lbl(label, font, 24);
        lbl.setFillColor(enabled ? Cyber::White : sf::Color(90, 100, 120, 170));
        lbl.setPosition(LX, cy);
        window.draw(lbl);
        float ratio = (currentV - minV) / (maxV - minV);
        if (ratio < 0.f) ratio = 0.f;
        if (ratio > 1.f) ratio = 1.f;
        float barW = 320.f, barH = 24.f;
        float barX = CX, barY = cy + 3.f;
        sf::Color baseCol = enabled ? sf::Color(0, 150, 200, 100) : sf::Color(70, 80, 95, 70);
        sf::Color fillCol = enabled ? Cyber::Cyan : sf::Color(85, 95, 110, 130);
        drawPill(barX, barY, barW, barH, sf::Color(10, 15, 25, 200), baseCol);
        if (ratio > 0.f) {
            float fillW = std::max(barH, barW * ratio);
            drawPill(barX, barY, fillW, barH, fillCol, sf::Color::Transparent);
        }
        float knobW = 20.f, knobH = barH + 16.f;
        float knobX = barX + barW * ratio - knobW / 2.f;
        float knobY = barY - 8.f;
        bool hovKnob = enabled && (mp.x >= barX && mp.x <= barX + barW && mp.y >= cy - 10.f && mp.y <= cy + 40.f);
        sf::Color kCol = enabled ? (hovKnob ? Cyber::White : Cyber::Cyan) : sf::Color(95, 105, 120, 140);
        sf::ConvexShape hexKnob(6);
        hexKnob.setPoint(0, { knobX, knobY + 6.f });
        hexKnob.setPoint(1, { knobX + knobW / 2.f, knobY });
        hexKnob.setPoint(2, { knobX + knobW, knobY + 6.f });
        hexKnob.setPoint(3, { knobX + knobW, knobY + knobH - 6.f });
        hexKnob.setPoint(4, { knobX + knobW / 2.f, knobY + knobH });
        hexKnob.setPoint(5, { knobX, knobY + knobH - 6.f });
        hexKnob.setFillColor(sf::Color(15, 20, 30, 240));
        hexKnob.setOutlineThickness(2.f);
        hexKnob.setOutlineColor(kCol);
        window.draw(hexKnob);
        sf::RectangleShape core({ 2.f, knobH - 12.f });
        core.setPosition(knobX + knobW / 2.f - 1.f, knobY + 6.f);
        core.setFillColor(enabled ? Cyber::White : sf::Color(120, 130, 145, 160));
        window.draw(core);
        DrawCentredText(window, font, std::to_string((int)currentV), 24, enabled ? Cyber::Yellow : sf::Color(100, 110, 130), barX + barW + 45.f, cy + 14.f);
        };

    auto drawToggle = [&](const std::string& label, float cy, bool isOn, bool enabled) {
        sf::Text lbl(label, font, 24);
        lbl.setFillColor(enabled ? Cyber::White : sf::Color(90, 100, 120, 170));
        lbl.setPosition(LX, cy);
        window.draw(lbl);
        sf::Color actCol = enabled ? (isOn ? Cyber::Cyan : Cyber::NeonRed) : sf::Color(95, 105, 120, 140);
        float tW = 80.f, tH = 30.f;
        float tY = cy;
        bool hovT = enabled && (mp.x >= CX && mp.x <= CX + tW + 80.f && mp.y >= tY - 10.f && mp.y <= tY + tH + 10.f);
        drawPill(CX, tY, tW, tH, sf::Color(15, 20, 25, 255), actCol);
        float knobR = tH / 2.f - 4.f;
        float knobY = tY + tH / 2.f;
        float knobX = isOn ? (CX + tW - knobR - 4.f) : (CX + knobR + 4.f);
        sf::CircleShape glow(knobR + 6.f);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(knobX, knobY);
        glow.setFillColor(sf::Color(actCol.r, actCol.g, actCol.b, hovT ? 120 : 60));
        window.draw(glow);
        sf::CircleShape knobTop(knobR);
        knobTop.setOrigin(knobR, knobR);
        knobTop.setPosition(knobX, knobY);
        knobTop.setFillColor(actCol);
        window.draw(knobTop);
        sf::CircleShape knobCore(knobR - 4.f);
        knobCore.setOrigin(knobR - 4.f, knobR - 4.f);
        knobCore.setPosition(knobX, knobY);
        knobCore.setFillColor(enabled ? Cyber::White : sf::Color(140, 145, 155));
        window.draw(knobCore);
        DrawCentredText(window, font, isOn ? "ON" : "OFF", 24, actCol, CX + tW + 40.f, tY + tH / 2.f - 2.f);
        if (hovT) DrawCornerBrackets(window, CX - 10.f, tY - 8.f, tW + 100.f, tH + 16.f, actCol, 8.f, 1.5f);
        };

    if (audioOnly) {
        DrawCentredText(window, font, "In match: gameplay settings are locked", 17,
            sf::Color(150, 170, 200), W / 2.f, pY + 38.f);
    }

    drawSlider("Board Size (10-30):", startY + 0 * RG, static_cast<float>(boardSize), 10.f, 30.f, !audioOnly);
    drawToggle("Blocked Ends Rule:", startY + 1 * RG, ruleBlock2, !audioOnly);
    drawSlider("AI Difficulty (1-3):", startY + 2 * RG, static_cast<float>(aiLevel), 1.f, 3.f, !audioOnly);
    drawSlider("SFX Volume (0-100):", startY + 3 * RG, sfxVolume, 0.f, 100.f, true);
    drawToggle("BGM Music:", startY + 4 * RG, bgmEnabled, true);
    drawToggle("Virus Mode:", startY + 5 * RG, virusMode, !audioOnly);

    if (audioOnly)
    {
        bool hoverLocked = false;
        for (int i = 0; i < 6; ++i)
        {
            if (i == 3 || i == 4) continue;
            float rowY = startY + i * RG;
            if (mp.x >= LX - 8.f && mp.x <= CX + 380.f && mp.y >= rowY - 12.f && mp.y <= rowY + 48.f)
            {
                hoverLocked = true;
                break;
            }
        }

        if (hoverLocked)
        {
            const float tipW = 300.f;
            const float tipH = 46.f;
            float tipX = std::min(mp.x + 18.f, W - tipW - 18.f);
            float tipY = std::min(mp.y + 18.f, H - tipH - 18.f);

            DrawNeonRect(window, tipX, tipY, tipW, tipH,
                sf::Color(8, 12, 22, 245),
                sf::Color(255, 220, 0, 210),
                1.6f);
            DrawCornerBrackets(window, tipX, tipY, tipW, tipH, Cyber::Yellow, 8.f, 1.2f);
            DrawCentredText(window, font, "LOCKED DURING MATCH", 18,
                Cyber::Yellow, tipX + tipW / 2.f, tipY + tipH / 2.f);
        }
    }

    const float BW = 300.f, BH = 60.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = pY + panelH - 76.f;
    bool bh = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);
    DrawNeonRect(window, BX, BY, BW, BH, bh ? sf::Color(20, 40, 70) : Cyber::BgBtn, bh ? Cyber::Magenta : Cyber::Grid, 2.f);
    if (bh) DrawCornerBrackets(window, BX, BY, BW, BH, Cyber::Magenta, 10.f, 2.f);
    DrawCentredText(window, font, audioOnly ? "BACK TO GAME" : "BACK TO MENU", 24,
        bh ? Cyber::White : sf::Color(160, 175, 200), BX + BW / 2.f, BY + BH / 2.f);
}

static void DrawLoadSlotRowClean(sf::RenderWindow& window, const sf::Font& font,
    float x, float y, float w, float h,
    int slotId, bool hasData, bool hovered,
    const std::string& name, int boardSize, int moves, int turn,
    bool showDeleteButton,
    float animT)
{
    float pulse = (std::sin(animT * 3.0f + slotId * 0.8f) + 1.f) * 0.5f;

    sf::Color border = hovered
        ? sf::Color(0, 255, 255, static_cast<sf::Uint8>(190 + pulse * 60))
        : sf::Color(40, 55, 80, 170);

    sf::Color fill = hovered
        ? sf::Color(16, 32, 54, 238)
        : sf::Color(9, 13, 24, 225);

    DrawNeonRect(window, x, y, w, h, fill, border, hovered ? 2.f : 1.f);

    // Glow nhe khi hover
    if (hovered) {
        DrawNeonRect(window, x - 3.f, y - 3.f, w + 6.f, h + 6.f,
            sf::Color::Transparent,
            sf::Color(0, 255, 255, static_cast<sf::Uint8>(35 + pulse * 55)),
            2.f);
    }

    sf::RectangleShape activeBar({ 5.f, h - 14.f });
    activeBar.setPosition(x + 8.f, y + 7.f);
    activeBar.setFillColor(hasData
        ? (hovered ? sf::Color(0, 255, 255, static_cast<sf::Uint8>(190 + pulse * 65)) : sf::Color(0, 170, 190, 180))
        : sf::Color(45, 55, 75, 180));
    window.draw(activeBar);

    std::string slotText = "SLOT ";
    if (slotId < 10) slotText += "0";
    slotText += std::to_string(slotId);

    sf::Text slot(slotText, font, 15);
    slot.setStyle(sf::Text::Bold);
    slot.setFillColor(hasData ? Cyber::Yellow : sf::Color(95, 110, 140));
    slot.setPosition(x + 24.f, y + 10.f);
    window.draw(slot);

    // Slot ben trai chi hien ten game/save
    sf::Text title(hasData ? name : "Empty Slot", font, hasData ? 22 : 20);
    title.setStyle(hasData ? sf::Text::Bold : sf::Text::Regular);
    title.setFillColor(hasData ? Cyber::White : sf::Color(120, 135, 160));
    title.setPosition(x + 24.f, y + 30.f);
    window.draw(title);

    if (hasData) {
        int mode = GetSlotVirusMode(slotId);
        std::string meta = "MODE: " + SaveModeLabel(mode) + "  |  MOVES: " + std::to_string(moves);
        sf::Text metaText(meta, font, 11);
        metaText.setStyle(sf::Text::Bold);
        metaText.setFillColor(mode == 1 ? sf::Color(255, 0, 200, 185) : sf::Color(0, 255, 255, 185));
        metaText.setPosition(x + 24.f, y + 55.f);
        window.draw(metaText);
    }

    // Status + dot nhip nhe
    sf::CircleShape dot(4.f);
    dot.setOrigin(4.f, 4.f);
    dot.setPosition(x + w - 86.f, y + 18.f);
    dot.setFillColor(hasData
        ? sf::Color(90, 255, 170, static_cast<sf::Uint8>(120 + pulse * 135))
        : sf::Color(105, 110, 130, 140));
    window.draw(dot);

    sf::Text status(hasData ? "READY" : "EMPTY", font, 12);
    status.setFillColor(hasData ? sf::Color(90, 255, 170) : sf::Color(105, 110, 130));
    status.setPosition(x + w - 72.f, y + 12.f);
    window.draw(status);

    // Nut xoa slot - chi hien trong man Load Game
    if (hasData && showDeleteButton) {
        const float delW = 34.f;
        const float delH = 22.f;
        const float delX = x + w - delW - 10.f;
        const float delY = y + 8.f;

        sf::Color delFill = hovered
            ? sf::Color(65, 16, 26, 235)
            : sf::Color(34, 14, 20, 215);

        sf::Color delBorder = hovered
            ? sf::Color(255, 95, 145, 220)
            : sf::Color(190, 70, 110, 170);

        DrawNeonRect(window, delX, delY, delW, delH, delFill, delBorder, 1.1f);

        sf::Text delTxt("X", font, 14);
        delTxt.setStyle(sf::Text::Bold);
        delTxt.setFillColor(sf::Color(255, 170, 200, 240));

        sf::FloatRect dr = delTxt.getLocalBounds();
        delTxt.setOrigin(dr.left + dr.width / 2.f, dr.top + dr.height / 2.f);
        delTxt.setPosition(delX + delW / 2.f, delY + delH / 2.f - 0.5f);
        window.draw(delTxt);
    }

    if (hovered) {
        // Scanline chay ngang trong slot
        float scanW = 72.f;
        float local = std::fmod(animT * 130.f, w - 75.f);
        sf::RectangleShape scan({ scanW, 2.f });
        scan.setPosition(x + 24.f + local, y + h - 12.f);
        scan.setFillColor(sf::Color(0, 255, 255, 85));
        window.draw(scan);

        DrawCornerBrackets(window, x - 2.f, y - 2.f, w + 4.f, h + 4.f, Cyber::Cyan, 8.f, 1.4f);
    }
}

static void DrawInfoMiniCard(sf::RenderWindow& window, const sf::Font& font,
    float x, float y, float w, float h,
    const std::string& label, const std::string& value, sf::Color accent)
{
    DrawNeonRect(window, x, y, w, h,
        sf::Color(10, 16, 30, 230),
        sf::Color(accent.r, accent.g, accent.b, 130), 1.f);

    sf::Text l(label, font, 12);
    l.setFillColor(sf::Color(120, 150, 190));
    l.setPosition(x + 10.f, y + 8.f);
    window.draw(l);

    sf::Text v(value, font, 20);
    v.setStyle(sf::Text::Bold);
    v.setFillColor(accent);
    v.setPosition(x + 10.f, y + 28.f);
    window.draw(v);
}

static void DrawLoadPreviewClean(sf::RenderWindow& window, const sf::Font& font,
    float x, float y, float w, float h, int slotId, float animT)
{
    float pulse = (std::sin(animT * 2.6f) + 1.f) * 0.5f;

    DrawNeonRect(window, x, y, w, h,
        sf::Color(8, 12, 22, 225),
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(85 + pulse * 45)), 1.4f);
    DrawCornerBrackets(window, x, y, w, h, Cyber::Cyan, 14.f, 1.5f);

    int bSize = 0, moves = 0, turn = 0;
    char dateBuf[32] = "";
    char nameBuf[64] = "";
    int previewBoard[30][30] = { 0 };

    bool hasPreview = (slotId >= 1 && slotId <= 5 &&
        GetSlotPreview(slotId, &bSize, &moves, &turn, dateBuf, nameBuf, previewBoard));

    int savedMode = GetSlotVirusMode(slotId);

    if (!hasPreview) {
        // Khung rong van co pulse nhe
        sf::CircleShape waitRing(42.f, 64);
        waitRing.setOrigin(42.f, 42.f);
        waitRing.setPosition(x + w / 2.f, y + h / 2.f - 42.f);
        waitRing.setFillColor(sf::Color::Transparent);
        waitRing.setOutlineThickness(1.2f);
        waitRing.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(35 + pulse * 55)));
        window.draw(waitRing);

        sf::CircleShape waitCore(5.f + pulse * 2.f, 32);
        waitCore.setOrigin(waitCore.getRadius(), waitCore.getRadius());
        waitCore.setPosition(x + w / 2.f, y + h / 2.f - 42.f);
        waitCore.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(90 + pulse * 110)));
        window.draw(waitCore);

        DrawCentredText(window, font, "HOVER A SLOT", 25, Cyber::Gray, x + w / 2.f, y + h / 2.f + 16.f);
        DrawCentredText(window, font, "to preview game information", 17, sf::Color(100, 125, 160), x + w / 2.f, y + h / 2.f + 50.f);
        return;
    }


    // Nut Xoa o goc tren phai cua bang thong tin lon ben phai
    {
        const float delW = 34.f;
        const float delH = 24.f;
        const float delX = x + w - delW - 10.f;
        const float delY = y + 10.f;

        sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        bool delHover = (mp.x >= delX && mp.x <= delX + delW && mp.y >= delY && mp.y <= delY + delH);

        DrawNeonRect(window, delX, delY, delW, delH,
            delHover ? sf::Color(70, 18, 28, 235) : sf::Color(35, 14, 22, 215),
            delHover ? sf::Color(255, 95, 145, 230) : sf::Color(190, 70, 110, 180),
            1.2f);

        sf::Text delTxt("X", font, 14);
        delTxt.setStyle(sf::Text::Bold);
        delTxt.setFillColor(sf::Color(255, 180, 210));
        sf::FloatRect dtr = delTxt.getLocalBounds();
        delTxt.setOrigin(dtr.left + dtr.width / 2.f, dtr.top + dtr.height / 2.f);
        delTxt.setPosition(delX + delW / 2.f, delY + delH / 2.f - 1.f);
        window.draw(delTxt);
    }

    // Header
    sf::Text head("GAME INFORMATION", font, 17);
    head.setStyle(sf::Text::Bold);
    head.setFillColor(Cyber::Yellow);
    head.setPosition(x + 20.f, y + 16.f);
    window.draw(head);

    // Doi mau title rat nhe cho song dong
    sf::Text name(nameBuf, font, 27);
    name.setStyle(sf::Text::Bold);
    name.setFillColor(sf::Color(220, 230, 255, 230 + static_cast<sf::Uint8>(pulse * 25)));
    name.setPosition(x + 20.f, y + 43.f);
    window.draw(name);

    sf::Text date(std::string("Saved date: ") + dateBuf, font, 14);
    date.setFillColor(sf::Color(120, 155, 190));
    date.setPosition(x + 22.f, y + 80.f);
    window.draw(date);

    const float cardY = y + 112.f;
    const float cardW = 132.f;
    const float cardH = 60.f;
    const float cardGap = 12.f;
    // Che do da hien ngay duoi ten slot ben trai, nen bang thong tin lon chi giu thong tin van dau.
    DrawInfoMiniCard(window, font, x + 20.f, cardY, cardW, cardH, "BOARD", std::to_string(bSize) + "x" + std::to_string(bSize), Cyber::Cyan);
    DrawInfoMiniCard(window, font, x + 20.f + cardW + cardGap, cardY, cardW, cardH, "MOVES", std::to_string(moves), Cyber::Yellow);
    DrawInfoMiniCard(window, font, x + 20.f + (cardW + cardGap) * 2.f, cardY, cardW, cardH, "TURN", turn == 1 ? "X" : "O", Cyber::Magenta);

    const float boardBox = 215.f;
    float bx = x + 20.f;
    float by = y + 184.f;
    DrawNeonRect(window, bx, by, boardBox, boardBox,
        sf::Color(4, 8, 16, 245),
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(95 + pulse * 45)), 1.f);
    DrawCornerBrackets(window, bx, by, boardBox, boardBox, Cyber::Cyan, 10.f, 1.2f);

    if (bSize > 0) {
        float cell = boardBox / static_cast<float>(bSize);
        for (int i = 0; i <= bSize; ++i) {
            sf::RectangleShape v({ 1.f, boardBox });
            v.setPosition(bx + i * cell, by);
            v.setFillColor(sf::Color(0, 255, 255, 38));
            window.draw(v);

            sf::RectangleShape hLine({ boardBox, 1.f });
            hLine.setPosition(bx, by + i * cell);
            hLine.setFillColor(sf::Color(0, 255, 255, 38));
            window.draw(hLine);
        }

        // Scanline doc ngang tren board preview
        float scanY = by + std::fmod(animT * 55.f, boardBox);
        sf::RectangleShape scanLine({ boardBox, 3.f });
        scanLine.setPosition(bx, scanY);
        scanLine.setFillColor(sf::Color(0, 255, 255, 55));
        window.draw(scanLine);

        for (int r = 0; r < bSize; ++r) {
            for (int c = 0; c < bSize; ++c) {
                // previewBoard trong logic đang lưu theo [x][y], còn preview UI vẽ theo [row][col].
                // Nếu dùng previewBoard[r][c] thì preview bị xoay 90 độ. Dùng [c][r] để đúng vị trí như bàn cờ thật.
                int val = previewBoard[c][r];
                if (val == 0) continue;

                unsigned pieceSize = static_cast<unsigned>((cell * 0.62f > 11.f) ? cell * 0.62f : 11.f);
                std::string symbol = (val == 1) ? "X" : ((val == 2) ? "O" : "V");
                sf::Text piece(symbol, font, pieceSize);
                if (val == 1) {
                    piece.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(200 + pulse * 55)));
                }
                else if (val == 2) {
                    piece.setFillColor(sf::Color(255, 0, 200, static_cast<sf::Uint8>(200 + pulse * 55)));
                }
                else {
                    float vx = bx + c * cell;
                    float vy = by + r * cell;
                    float vcx = vx + cell / 2.f;
                    float vcy = vy + cell / 2.f;

                    sf::RectangleShape virusGlow({ cell, cell });
                    virusGlow.setPosition(vx, vy);
                    virusGlow.setFillColor(sf::Color(20, 85, 30, static_cast<sf::Uint8>(38 + pulse * 38)));
                    window.draw(virusGlow);

                    DrawCornerBrackets(window,
                        vx + 1.f, vy + 1.f,
                        cell - 2.f, cell - 2.f,
                        sf::Color(90, 255, 120, static_cast<sf::Uint8>(145 + pulse * 70)),
                        5.f, 1.2f);

                    DrawVirusSkullIcon(window, vcx, vcy, cell, pulse);
                    continue;
                }
                sf::FloatRect pr = piece.getLocalBounds();
                piece.setOrigin(pr.left + pr.width / 2.f, pr.top + pr.height / 2.f);
                piece.setPosition(bx + c * cell + cell / 2.f, by + r * cell + cell / 2.f);
                window.draw(piece);
            }
        }
    }

    // Cot thong tin/huong dan ben phai board
    float infoX = bx + boardBox + 18.f;
    float infoY = by;
    float infoW = w - (infoX - x) - 20.f;
    DrawNeonRect(window, infoX, infoY, infoW, boardBox,
        sf::Color(10, 16, 30, 210),
        sf::Color(255, 0, 200, static_cast<sf::Uint8>(75 + pulse * 45)), 1.f);

    sf::Text infoHead("SAVE DETAILS", font, 14);
    infoHead.setStyle(sf::Text::Bold);
    infoHead.setFillColor(Cyber::Magenta);
    infoHead.setPosition(infoX + 14.f, infoY + 14.f);
    window.draw(infoHead);

    auto line = [&](const std::string& label, const std::string& value, float oy, sf::Color valueCol) {
        sf::Text l(label, font, 12);
        l.setFillColor(sf::Color(115, 140, 175));
        l.setPosition(infoX + 12.f, infoY + oy);
        window.draw(l);

        sf::Text v(value, font, 15);
        v.setStyle(sf::Text::Bold);
        v.setFillColor(valueCol);
        v.setPosition(infoX + 12.f, infoY + oy + 17.f);
        window.draw(v);
        };

    line("SAVE NAME", std::string(nameBuf), 44.f, Cyber::White);
    line("SAVE DATE", std::string(dateBuf), 88.f, Cyber::Cyan);
    line("STATUS", "READY TO LOAD", 132.f, sf::Color(90, 255, 170));

    sf::RectangleShape div({ infoW - 24.f, 1.f });
    div.setPosition(infoX + 12.f, infoY + 178.f);
    div.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(45 + pulse * 45)));
    window.draw(div);

    // Dot trang thai nhip nhe
    sf::CircleShape readyDot(4.f + pulse * 1.5f);
    readyDot.setOrigin(readyDot.getRadius(), readyDot.getRadius());
    readyDot.setPosition(infoX + infoW - 24.f, infoY + 155.f);
    readyDot.setFillColor(sf::Color(90, 255, 170, static_cast<sf::Uint8>(120 + pulse * 135)));
    window.draw(readyDot);

}


void DrawLoadScreen(sf::RenderWindow& window, const sf::Font& font)
{
    static sf::Clock loadAnimClock;
    float animT = loadAnimClock.getElapsedTime().asSeconds();

    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    // Nen giong man cai dat: toi, co vong HUD nhe
    sf::RectangleShape bg({ W, H });
    bg.setFillColor(sf::Color(8, 10, 18));
    window.draw(bg);

    for (float x = 0.f; x < W; x += 64.f) {
        sf::RectangleShape line({ 1.f, H });
        line.setPosition(x, 0.f);
        line.setFillColor(sf::Color(0, 255, 255, 5));
        window.draw(line);
    }
    for (float y = 0.f; y < H; y += 48.f) {
        sf::RectangleShape line({ W, 1.f });
        line.setPosition(0.f, y);
        line.setFillColor(sf::Color(255, 0, 200, 4));
        window.draw(line);
    }

    // HUD circle nen co pulse + sweep nhe
    float circlePulse = (std::sin(animT * 1.5f) + 1.f) * 0.5f;

    sf::CircleShape bigCircle(370.f, 96);
    bigCircle.setOrigin(370.f, 370.f);
    bigCircle.setPosition(W / 2.f, H / 2.f + 10.f);
    bigCircle.setFillColor(sf::Color::Transparent);
    bigCircle.setOutlineThickness(1.f);
    bigCircle.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(18 + circlePulse * 18)));
    window.draw(bigCircle);

    sf::CircleShape innerCircle(270.f, 96);
    innerCircle.setOrigin(270.f, 270.f);
    innerCircle.setPosition(W / 2.f, H / 2.f + 10.f);
    innerCircle.setFillColor(sf::Color::Transparent);
    innerCircle.setOutlineThickness(1.f);
    innerCircle.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(12 + circlePulse * 15)));
    window.draw(innerCircle);

    // Sweep line quay rat nhe quanh background circle
    float sweepAng = animT * 0.8f;
    sf::RectangleShape sweep({ 370.f, 1.5f });
    sweep.setOrigin(0.f, 0.75f);
    sweep.setPosition(W / 2.f, H / 2.f + 10.f);
    sweep.setRotation(sweepAng * 180.f / 3.14159265f);
    sweep.setFillColor(sf::Color(0, 255, 255, 28));
    window.draw(sweep);

    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Title nhu man cai dat, co glow nhe
    float titlePulse = (std::sin(animT * 2.2f) + 1.f) * 0.5f;
    float arrowOffset = std::sin(animT * 3.0f) * 8.f;

    sf::Text arrowL(">>", font, 38);
    arrowL.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(150 + titlePulse * 105)));
    arrowL.setPosition(W / 2.f - 300.f - arrowOffset, 72.f);
    window.draw(arrowL);

    sf::Text glowTitle("LOAD GAME", font, 46);
    glowTitle.setStyle(sf::Text::Bold);
    glowTitle.setFillColor(sf::Color(255, 220, 0, static_cast<sf::Uint8>(35 + titlePulse * 35)));
    sf::FloatRect gtr = glowTitle.getLocalBounds();
    glowTitle.setOrigin(gtr.left + gtr.width / 2.f, gtr.top + gtr.height / 2.f);
    glowTitle.setPosition(W / 2.f + 2.f, 92.f + 2.f);
    window.draw(glowTitle);

    sf::Text title("LOAD GAME", font, 46);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(Cyber::Yellow);
    sf::FloatRect tr = title.getLocalBounds();
    title.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
    title.setPosition(W / 2.f, 92.f);
    window.draw(title);

    sf::Text arrowR("<<", font, 38);
    arrowR.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(150 + titlePulse * 105)));
    arrowR.setPosition(W / 2.f + 245.f + arrowOffset, 72.f);
    window.draw(arrowR);

    const float panelW = 980.f;
    const float panelH = 520.f;
    const float panelX = W / 2.f - panelW / 2.f;
    const float panelY = 150.f;

    DrawNeonRect(window, panelX, panelY, panelW, panelH,
        sf::Color(6, 10, 18, 225),
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(90 + titlePulse * 45)), 1.5f);
    DrawCornerBrackets(window, panelX - 8.f, panelY - 8.f, panelW + 16.f, panelH + 16.f,
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(190 + titlePulse * 65)), 38.f, 3.f);

    DrawSectionHeader(window, font, "[ SAVE SLOT LIST ]", panelX + 28.f, panelY + 26.f, 390.f, Cyber::Cyan);
    DrawSectionHeader(window, font, "[ PREVIEW ]", panelX + 455.f, panelY + 26.f, 475.f, Cyber::Magenta);

    const float slotX = panelX + 28.f;
    const float slotY = panelY + 58.f;
    const float slotW = 390.f;
    const float slotH = 72.f;
    const float gap = 18.f;

    int hoveredSlot = -1;
    for (int i = 1; i <= 5; ++i) {
        float y = slotY + (i - 1) * (slotH + gap);
        if (mp.x >= slotX && mp.x <= slotX + slotW &&
            mp.y >= y && mp.y <= y + slotH) {
            hoveredSlot = i;
        }
    }

    if (hoveredSlot != -1) {
        gLoadPreviewSlotUI = hoveredSlot;
    }
    if (gLoadPreviewSlotUI < 1 || gLoadPreviewSlotUI > 5) {
        gLoadPreviewSlotUI = 1;
    }

    for (int i = 1; i <= 5; ++i) {
        float y = slotY + (i - 1) * (slotH + gap);
        int bSize = 0, moves = 0, turn = 0;
        char name[64] = "";
        bool hasData = PeekGameSlot(i, &bSize, &moves, &turn, name);
        DrawLoadSlotRowClean(window, font, slotX, y, slotW, slotH,
            i, hasData, hoveredSlot == i, hasData ? std::string(name) : "", bSize, moves, turn,
            false, animT);
    }

    const float prevX = panelX + 455.f;
    const float prevY = panelY + 58.f;
    const float prevW = 490.f;
    const float prevH = 420.f;
    DrawLoadPreviewClean(window, font, prevX, prevY, prevW, prevH, gLoadPreviewSlotUI, animT);

    // footer nho trong panel
    sf::Text foot("LOAD.SYS // SECURE SAVE ACCESS", font, 12);
    foot.setFillColor(sf::Color(0, 255, 255, 110));
    foot.setPosition(panelX + 36.f, panelY + panelH - 22.f);
    window.draw(foot);

    // Thanh data bits chay nhe
    float bitsX = panelX + panelW - 210.f;
    float bitsY = panelY + panelH - 24.f;
    for (int i = 0; i < 18; ++i) {
        float bh = 4.f + ((i % 4) * 2.f);
        float alphaPulse = (std::sin(animT * 4.f + i * 0.65f) + 1.f) * 0.5f;
        sf::RectangleShape bit({ 5.f, bh });
        bit.setPosition(bitsX + i * 8.f, bitsY - bh + 5.f);
        bit.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(45 + alphaPulse * 100)));
        window.draw(bit);
    }

    // Back button
    const float BW = 300.f;
    const float BH = 60.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = panelY + panelH + 32.f;
    bool bh = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);
    DrawNeonRect(window, BX, BY, BW, BH,
        bh ? sf::Color(20, 40, 70) : Cyber::BgBtn,
        bh ? Cyber::Magenta : Cyber::Grid, 2.f);
    if (bh) DrawCornerBrackets(window, BX, BY, BW, BH, Cyber::Magenta, 10.f, 2.f);
    DrawCentredText(window, font, "BACK TO MENU", 24, bh ? Cyber::White : sf::Color(160, 175, 200), BX + BW / 2.f, BY + BH / 2.f);
}

void DrawSaveScreen(sf::RenderWindow& window, const sf::Font& font, bool isNaming, const std::string& inputName, sf::Clock& clock, bool isConfirmOverwrite)
{
    static sf::Clock saveAnimClock;
    float animT = saveAnimClock.getElapsedTime().asSeconds();

    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);
    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Nen dong bo voi man Load Game / Settings
    sf::RectangleShape bg({ W, H });
    bg.setFillColor(sf::Color(8, 10, 18));
    window.draw(bg);

    for (float x = 0.f; x < W; x += 64.f) {
        sf::RectangleShape line({ 1.f, H });
        line.setPosition(x, 0.f);
        line.setFillColor(sf::Color(0, 255, 255, 5));
        window.draw(line);
    }
    for (float y = 0.f; y < H; y += 48.f) {
        sf::RectangleShape line({ W, 1.f });
        line.setPosition(0.f, y);
        line.setFillColor(sf::Color(255, 0, 200, 4));
        window.draw(line);
    }

    float circlePulse = (std::sin(animT * 1.5f) + 1.f) * 0.5f;
    sf::CircleShape bigCircle(370.f, 96);
    bigCircle.setOrigin(370.f, 370.f);
    bigCircle.setPosition(W / 2.f, H / 2.f + 10.f);
    bigCircle.setFillColor(sf::Color::Transparent);
    bigCircle.setOutlineThickness(1.f);
    bigCircle.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(18 + circlePulse * 18)));
    window.draw(bigCircle);

    sf::CircleShape innerCircle(270.f, 96);
    innerCircle.setOrigin(270.f, 270.f);
    innerCircle.setPosition(W / 2.f, H / 2.f + 10.f);
    innerCircle.setFillColor(sf::Color::Transparent);
    innerCircle.setOutlineThickness(1.f);
    innerCircle.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(12 + circlePulse * 15)));
    window.draw(innerCircle);

    float sweepAng = animT * 0.8f;
    sf::RectangleShape sweep({ 370.f, 1.5f });
    sweep.setOrigin(0.f, 0.75f);
    sweep.setPosition(W / 2.f, H / 2.f + 10.f);
    sweep.setRotation(sweepAng * 180.f / 3.14159265f);
    sweep.setFillColor(sf::Color(0, 255, 255, 28));
    window.draw(sweep);

    float titlePulse = (std::sin(animT * 2.2f) + 1.f) * 0.5f;
    float arrowOffset = std::sin(animT * 3.0f) * 8.f;

    sf::Text arrowL(">>", font, 38);
    arrowL.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(150 + titlePulse * 105)));
    arrowL.setPosition(W / 2.f - 300.f - arrowOffset, 72.f);
    window.draw(arrowL);

    sf::Text glowTitle("SAVE GAME", font, 46);
    glowTitle.setStyle(sf::Text::Bold);
    glowTitle.setFillColor(sf::Color(255, 220, 0, static_cast<sf::Uint8>(35 + titlePulse * 35)));
    sf::FloatRect gtr = glowTitle.getLocalBounds();
    glowTitle.setOrigin(gtr.left + gtr.width / 2.f, gtr.top + gtr.height / 2.f);
    glowTitle.setPosition(W / 2.f + 2.f, 92.f + 2.f);
    window.draw(glowTitle);

    sf::Text title("SAVE GAME", font, 46);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(Cyber::Yellow);
    sf::FloatRect tr = title.getLocalBounds();
    title.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
    title.setPosition(W / 2.f, 92.f);
    window.draw(title);

    sf::Text arrowR("<<", font, 38);
    arrowR.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(150 + titlePulse * 105)));
    arrowR.setPosition(W / 2.f + 245.f + arrowOffset, 72.f);
    window.draw(arrowR);

    const float panelW = 980.f;
    const float panelH = 520.f;
    const float panelX = W / 2.f - panelW / 2.f;
    const float panelY = 150.f;

    DrawNeonRect(window, panelX, panelY, panelW, panelH,
        sf::Color(6, 10, 18, 225),
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(90 + titlePulse * 45)), 1.5f);
    DrawCornerBrackets(window, panelX - 8.f, panelY - 8.f, panelW + 16.f, panelH + 16.f,
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(190 + titlePulse * 65)), 38.f, 3.f);

    DrawSectionHeader(window, font, "[ CHOOSE SAVE SLOT ]", panelX + 28.f, panelY + 26.f, 390.f, Cyber::Cyan);
    DrawSectionHeader(window, font, "[ PREVIEW / OVERWRITE ]", panelX + 455.f, panelY + 26.f, 475.f, Cyber::Magenta);

    const float slotX = panelX + 28.f;
    const float slotY = panelY + 58.f;
    const float slotW = 390.f;
    const float slotH = 72.f;
    const float gap = 18.f;

    int hoveredSlot = -1;
    if (!isNaming) {
        for (int i = 1; i <= 5; ++i) {
            float y = slotY + (i - 1) * (slotH + gap);
            if (mp.x >= slotX && mp.x <= slotX + slotW && mp.y >= y && mp.y <= y + slotH)
                hoveredSlot = i;
        }
    }

    for (int i = 1; i <= 5; ++i) {
        float y = slotY + (i - 1) * (slotH + gap);
        int bSize = 0, moves = 0, turn = 0;
        char name[64] = "";
        bool hasData = PeekGameSlot(i, &bSize, &moves, &turn, name);

        DrawLoadSlotRowClean(window, font, slotX, y, slotW, slotH,
            i, hasData, hoveredSlot == i, hasData ? std::string(name) : "", bSize, moves, turn,
            false, animT);

        // Ghi chú riêng cho Save: slot có dữ liệu sẽ ghi đè
        if (hasData) {
            sf::Text overwrite("OVERWRITE", font, 11);
            overwrite.setStyle(sf::Text::Bold);
            overwrite.setFillColor(sf::Color(255, 220, 0, 210));
            overwrite.setPosition(slotX + slotW - 74.f, y + 43.f);
            window.draw(overwrite);
        }
        else {
            sf::Text saveHere("SAVE", font, 11);
            saveHere.setStyle(sf::Text::Bold);
            saveHere.setFillColor(sf::Color(90, 255, 170, 190));
            saveHere.setPosition(slotX + slotW - 64.f, y + 43.f);
            window.draw(saveHere);
        }
    }

    const float prevX = panelX + 455.f;
    const float prevY = panelY + 58.f;
    const float prevW = 490.f;
    const float prevH = 420.f;

    if (hoveredSlot != -1) {
        int bSize = 0, moves = 0, turn = 0;
        char name[64] = "";
        bool hasData = PeekGameSlot(hoveredSlot, &bSize, &moves, &turn, name);

        if (hasData) {
            DrawLoadPreviewClean(window, font, prevX, prevY, prevW, prevH, hoveredSlot, animT);

        }
        else {
            float pulse = (std::sin(animT * 2.6f) + 1.f) * 0.5f;
            DrawNeonRect(window, prevX, prevY, prevW, prevH,
                sf::Color(8, 12, 22, 225),
                sf::Color(0, 255, 255, static_cast<sf::Uint8>(85 + pulse * 45)), 1.4f);
            DrawCornerBrackets(window, prevX, prevY, prevW, prevH, Cyber::Cyan, 14.f, 1.5f);

            sf::CircleShape ring(48.f + pulse * 4.f, 64);
            ring.setOrigin(ring.getRadius(), ring.getRadius());
            ring.setPosition(prevX + prevW / 2.f, prevY + prevH / 2.f - 52.f);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(1.4f);
            ring.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(40 + pulse * 85)));
            window.draw(ring);

            sf::CircleShape core(6.f + pulse * 2.f, 32);
            core.setOrigin(core.getRadius(), core.getRadius());
            core.setPosition(prevX + prevW / 2.f, prevY + prevH / 2.f - 52.f);
            core.setFillColor(sf::Color(90, 255, 170, static_cast<sf::Uint8>(120 + pulse * 120)));
            window.draw(core);

            DrawCentredText(window, font, "EMPTY SLOT", 28, Cyber::Cyan, prevX + prevW / 2.f, prevY + prevH / 2.f + 15.f);
            DrawCentredText(window, font, "Ready to save game here", 17, sf::Color(120, 150, 185), prevX + prevW / 2.f, prevY + prevH / 2.f + 50.f);
        }
    }
    else {
        float pulse = (std::sin(animT * 2.6f) + 1.f) * 0.5f;
        DrawNeonRect(window, prevX, prevY, prevW, prevH,
            sf::Color(8, 12, 22, 225),
            sf::Color(0, 255, 255, static_cast<sf::Uint8>(85 + pulse * 45)), 1.4f);
        DrawCornerBrackets(window, prevX, prevY, prevW, prevH, Cyber::Cyan, 14.f, 1.5f);
        DrawCentredText(window, font, "HOVER A SLOT", 25, Cyber::Gray, prevX + prevW / 2.f, prevY + prevH / 2.f - 10.f);
        DrawCentredText(window, font, "to choose save destination", 17, sf::Color(100, 125, 160), prevX + prevW / 2.f, prevY + prevH / 2.f + 26.f);
    }

    sf::Text foot("SAVE.SYS // SECURE WRITE ACCESS", font, 12);
    foot.setFillColor(sf::Color(0, 255, 255, 110));
    foot.setPosition(panelX + 36.f, panelY + panelH - 22.f);
    window.draw(foot);

    float bitsX = panelX + panelW - 210.f;
    float bitsY = panelY + panelH - 24.f;
    for (int i = 0; i < 18; ++i) {
        float bh = 4.f + ((i % 4) * 2.f);
        float alphaPulse = (std::sin(animT * 4.f + i * 0.65f) + 1.f) * 0.5f;
        sf::RectangleShape bit({ 5.f, bh });
        bit.setPosition(bitsX + i * 8.f, bitsY - bh + 5.f);
        bit.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(45 + alphaPulse * 100)));
        window.draw(bit);
    }

    const float BW = 300.f;
    const float BH = 60.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = panelY + panelH + 32.f;
    bool bh = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);
    DrawNeonRect(window, BX, BY, BW, BH,
        bh ? sf::Color(20, 40, 70) : Cyber::BgBtn,
        bh ? Cyber::Magenta : Cyber::Grid, 2.f);
    if (bh) DrawCornerBrackets(window, BX, BY, BW, BH, Cyber::Magenta, 10.f, 2.f);
    DrawCentredText(window, font, "BACK TO GAME", 24, bh ? Cyber::White : sf::Color(160, 175, 200), BX + BW / 2.f, BY + BH / 2.f);

    // Dialog nhap ten giu logic cu nhung style gon hon
    if (isNaming) {
        sf::RectangleShape overlay({ W, H });
        overlay.setFillColor(sf::Color(0, 0, 0, 190));
        window.draw(overlay);

        float bX = W / 2.f - 260.f, bY = H / 2.f - 135.f;
        DrawNeonRect(window, bX, bY, 520.f, 270.f, Cyber::BgPanel, Cyber::Cyan, 2.5f);
        DrawCornerBrackets(window, bX, bY, 520.f, 270.f, Cyber::Cyan, 20.f, 2.5f);

        DrawCentredText(window, font, "ENTER SAVE NAME", 25, Cyber::Yellow, W / 2.f, H / 2.f - 86.f);
        DrawNeonRect(window, W / 2.f - 205.f, H / 2.f - 38.f, 410.f, 54.f, sf::Color(4, 8, 16, 230), Cyber::CyanDim, 1.5f);

        sf::Text inputText(inputName.empty() ? "Untitled Game" : inputName, font, 22);
        inputText.setFillColor(inputName.empty() ? sf::Color(100, 120, 150) : Cyber::Cyan);
        sf::FloatRect textBounds = inputText.getLocalBounds();
        inputText.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
        inputText.setPosition(W / 2.f, H / 2.f - 10.f);
        window.draw(inputText);

        static sf::Clock blinkClock;
        if (blinkClock.getElapsedTime().asMilliseconds() % 1000 < 500) {
            sf::RectangleShape cursor({ 2.f, 24.f });
            cursor.setFillColor(Cyber::Yellow);
            float cursorX = (W / 2.f) + (inputName.empty() ? 80.f : textBounds.width / 2.f + 4.f);
            cursor.setOrigin(1.f, 12.f);
            cursor.setPosition(cursorX, H / 2.f - 10.f);
            window.draw(cursor);
        }

        DrawNeonRect(window, W / 2.f - 105.f, H / 2.f + 44.f, 210.f, 52.f,
            sf::Color(12, 45, 30, 230), sf::Color(90, 255, 170), 2.f);
        DrawCentredText(window, font, "CONFIRM", 20, Cyber::White, W / 2.f, H / 2.f + 70.f);

        if (isConfirmOverwrite) {
            sf::RectangleShape overlay2({ W, H });
            overlay2.setFillColor(sf::Color(0, 0, 0, 210));
            window.draw(overlay2);

            float cw = 500.f, ch = 200.f;
            float cx = W / 2.f - cw / 2.f;
            float cy = H / 2.f - ch / 2.f;

            DrawNeonRect(window, cx, cy, cw, ch, Cyber::BgPanel, Cyber::Yellow, 2.5f);
            DrawCornerBrackets(window, cx, cy, cw, ch, Cyber::Yellow, 15.f, 2.f);

            DrawCentredText(window, font, "NAME ALREADY EXISTS!", 26, Cyber::NeonRed, W / 2.f, cy + 30.f);
            DrawCentredText(window, font, "Do you want to overwrite", 20, Cyber::White, W / 2.f, cy + 70.f);
            DrawCentredText(window, font, "the save file with this name?", 20, Cyber::White, W / 2.f, cy + 100.f);

            float btnW = 120.f, btnH = 50.f;
            float yesX = W / 2.f - 140.f, yesY = H / 2.f + 30.f;
            float noX = W / 2.f + 20.f, noY = H / 2.f + 30.f;
            bool hovYes = (mp.x >= yesX && mp.x <= yesX + btnW && mp.y >= yesY && mp.y <= yesY + btnH);
            bool hovNo = (mp.x >= noX && mp.x <= noX + btnW && mp.y >= noY && mp.y <= noY + btnH);
            DrawNeonRect(window, yesX, yesY, btnW, btnH, hovYes ? sf::Color(80, 20, 20) : sf::Color(30, 10, 10), Cyber::NeonRed, 2.f);
            DrawCentredText(window, font, "YES", 20, Cyber::White, yesX + btnW / 2.f, yesY + btnH / 2.f);
            DrawNeonRect(window, noX, noY, btnW, btnH, hovNo ? sf::Color(20, 60, 20) : sf::Color(10, 30, 10), sf::Color(50, 200, 100), 2.f);
            DrawCentredText(window, font, "NO", 20, Cyber::White, noX + btnW / 2.f, noY + btnH / 2.f);
        }
    }
}

// ── HÀM BỔ TRỢ CHO HIỆU ỨNG (Để ngay trên hàm vẽ) ──
static float EaseOutQuart(float t) { return 1.f - std::pow(1.f - t, 4.f); }

void DrawCharacterSelectScreen(sf::RenderWindow& window, const sf::Font& font, bool isPVP,
    int p1Char, int p2Char, const std::string& p1Name, const std::string& p2Name,
    int typingState, int selectionStep, sf::Sprite charSprites[4],
    int animatingCharIdx, sf::Clock& confirmAnimClk)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    sf::RectangleShape darkBg({ W, H });
    darkBg.setFillColor(sf::Color(5, 8, 12, 160));
    window.draw(darkBg);
    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 100));

    DrawCentredText(window, font, isPVP ? "CHOOSE CHARACTER & ENTER NAME" : "CHOOSE CHARACTER", 46, Cyber::Yellow, W / 2.f, 50.f);

    std::string stepStr = "";
    if (selectionStep == 0) stepStr = isPVP ? ">> PLAYER 1: CLICK TO CHOOSE, THEN PRESS [ENTER] <<" : ">> CLICK TO CHOOSE CHARACTER, THEN PRESS [ENTER] <<";
    else if (selectionStep == 1) stepStr = isPVP ? ">> PLAYER 1: ENTER NAME, THEN PRESS [ENTER] <<" : ">> ENTER NAME, THEN PRESS [ENTER] <<";
    else if (selectionStep == 2) stepStr = ">> PLAYER 2: CLICK TO CHOOSE, THEN PRESS [ENTER] <<";
    else if (selectionStep == 3) stepStr = ">> PLAYER 2: ENTER NAME, THEN PRESS [ENTER] <<";
    else if (selectionStep == 4) stepStr = ">> PRESS [ENTER] TO START <<";

    static sf::Clock blinkClk;
    if (std::fmod(blinkClk.getElapsedTime().asSeconds(), 0.8f) < 0.6f) {
        DrawCentredText(window, font, stepStr, 24, Cyber::Cyan, W / 2.f, 100.f);
    }

    const float BOX_W = 220.f, BOX_H = 300.f, SPACING = 30.f;
    const float TOTAL_W = 4 * BOX_W + 3 * SPACING;
    const float START_X = W / 2.f - TOTAL_W / 2.f;
    const float BOX_Y = 160.f;

    const char* charNames[] = { "HACKER", "CYBORG", "NINJA", "CORPO" };
    sf::Color charColors[] = { Cyber::Cyan, Cyber::Magenta, Cyber::Yellow, Cyber::NeonRed };
    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));
    int hoveredChar = -1;

    bool p1Selecting = (selectionStep == 0);
    bool p2Selecting = (selectionStep == 2);

    float animDT = confirmAnimClk.getElapsedTime().asSeconds();
    float animDur = 1.0f;
    float p = std::max(0.f, std::min(1.f, animDT / animDur));
    float easeP = EaseOutQuart(p);

    for (int i = 0; i < 4; ++i) {
        float cX = START_X + i * (BOX_W + SPACING);
        bool hov = (mp.x >= cX && mp.x <= cX + BOX_W && mp.y >= BOX_Y && mp.y <= BOX_Y + BOX_H);
        if (hov) hoveredChar = i;

        // [SỬA LOGIC SÁNG MỜ] - Đã chọn là sáng luôn
        bool isSelected = (i == p1Char) || (isPVP && i == p2Char);
        bool isHovered = hov && (p1Selecting || p2Selecting);
        bool isFocus = isSelected || isHovered;

        float dynamicScale = 1.0f;
        sf::Color dynamicBorderCol = charColors[i];
        if (!isFocus) dynamicBorderCol = sf::Color(dynamicBorderCol.r / 3, dynamicBorderCol.g / 3, dynamicBorderCol.b / 3, 150);

        if (animatingCharIdx == i && p < 1.0f) {
            float pulseScale = 1.0f + 0.05f * std::sin(p * 3.14159f);
            dynamicScale = pulseScale;

            float whiteLerp = std::pow(1.f - p, 3.f);
            dynamicBorderCol.r = static_cast<sf::Uint8>(dynamicBorderCol.r * (1.f - whiteLerp) + 255 * whiteLerp);
            dynamicBorderCol.g = static_cast<sf::Uint8>(dynamicBorderCol.g * (1.f - whiteLerp) + 255 * whiteLerp);
            dynamicBorderCol.b = static_cast<sf::Uint8>(dynamicBorderCol.b * (1.f - whiteLerp) + 255 * whiteLerp);
            dynamicBorderCol.a = 255;
        }

        sf::RectangleShape box({ BOX_W, BOX_H });
        box.setOrigin(BOX_W / 2.f, BOX_H / 2.f);
        box.setPosition(cX + BOX_W / 2.f, BOX_Y + BOX_H / 2.f);
        box.setScale(dynamicScale, dynamicScale);
        box.setFillColor(isFocus ? sf::Color(30, 40, 60, 200) : sf::Color(10, 15, 25, 150));
        box.setOutlineThickness(isFocus ? 3.f : 1.f);
        box.setOutlineColor(dynamicBorderCol);

        if (animatingCharIdx == i && p < 1.0f) {
            float pillarH = BOX_H + (H * easeP);
            sf::RectangleShape pillar({ BOX_W * (1.f - easeP), pillarH });
            pillar.setOrigin(pillar.getSize().x / 2.f, pillarH / 2.f);
            pillar.setPosition(box.getPosition());
            sf::Color pillarCol = charColors[i];
            pillarCol.a = static_cast<sf::Uint8>(150 * (1.f - easeP));
            pillar.setFillColor(pillarCol);
            window.draw(pillar);

            for (int k = 0; k < 12; ++k) {
                float kP = (std::fmod(p + k * 0.08f, 1.f));
                int dir = (k % 2 == 0) ? -1 : 1;
                float dataX = box.getPosition().x + (rand() % (int)BOX_W - BOX_W / 2.f);
                float dataY = box.getPosition().y + dir * (BOX_H / 2.f + 400.f * EaseOutQuart(kP));

                sf::RectangleShape laser({ (rand() % 4 + 2.f), (rand() % 60 + 20.f) });
                laser.setOrigin(laser.getSize().x / 2.f, laser.getSize().y / 2.f);
                laser.setPosition(dataX, dataY);
                sf::Color laserCol = (rand() % 3 == 0) ? Cyber::White : charColors[i];
                laserCol.a = static_cast<sf::Uint8>(255 * (1.f - kP));
                laser.setFillColor(laserCol);
                window.draw(laser);
            }
        }

        window.draw(box);

        sf::Color spriteTint = isFocus ? sf::Color(255, 255, 255, 255) : sf::Color(80, 80, 80, 150);
        charSprites[i].setColor(spriteTint);
        sf::FloatRect bounds = charSprites[i].getLocalBounds();
        float sW = (BOX_W - 10.f) / bounds.width;
        float sH = (BOX_H - 50.f) / bounds.height;
        charSprites[i].setScale(sW * dynamicScale, sH * dynamicScale);
        charSprites[i].setPosition(box.getPosition().x - (BOX_W / 2.f - 5.f) * dynamicScale, box.getPosition().y - (BOX_H / 2.f - 5.f) * dynamicScale);
        window.draw(charSprites[i]);
        charSprites[i].setColor(sf::Color::White);

        if (animatingCharIdx == i && p < 1.0f) {
            float imgTop = box.getPosition().y - (BOX_H / 2.f - 5.f) * dynamicScale;
            float imgBot = box.getPosition().y + (BOX_H / 2.f - 45.f) * dynamicScale;

            float scanY = imgTop + (imgBot - imgTop) * easeP;
            float scanW = (BOX_W - 10.f) * dynamicScale;

            for (int j = 0; j < 25; ++j) {
                float tailY = scanY - j * 2.f;
                if (tailY < imgTop) break;

                sf::RectangleShape tail({ scanW, 2.f });
                tail.setOrigin(scanW / 2.f, 1.f);
                tail.setPosition(box.getPosition().x, tailY);

                sf::Color tailCol = charColors[i];
                tailCol.a = static_cast<sf::Uint8>(std::max(0.f, 180.f * (1.f - j / 25.f) * (1.f - p)));
                tail.setFillColor(tailCol);
                window.draw(tail);
            }

            sf::RectangleShape scanLine({ scanW, 2.f });
            scanLine.setOrigin(scanW / 2.f, 1.f);
            scanLine.setPosition(box.getPosition().x, scanY);
            sf::Color lineCol = sf::Color::White;
            lineCol.a = static_cast<sf::Uint8>(255 * (1.f - p));
            scanLine.setFillColor(lineCol);
            window.draw(scanLine);

            DrawCornerBrackets(window, box.getPosition().x - scanW / 2.f - 4.f, scanY - 6.f, scanW + 8.f, 12.f, charColors[i], 8.f, 2.f);
        }

        sf::RectangleShape nameBg({ BOX_W, 40.f });
        nameBg.setOrigin(BOX_W / 2.f, 20.f);
        nameBg.setPosition(box.getPosition().x, box.getPosition().y + (BOX_H / 2.f - 20.f) * dynamicScale);
        nameBg.setScale(dynamicScale, dynamicScale);
        nameBg.setFillColor(sf::Color(0, 0, 0, 200)); nameBg.setOutlineThickness(1.f); nameBg.setOutlineColor(dynamicBorderCol);
        window.draw(nameBg);

        sf::Text txt(charNames[i], font, 24);
        txt.setFillColor(isFocus ? Cyber::White : dynamicBorderCol);
        sf::FloatRect tr = txt.getLocalBounds();
        txt.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
        txt.setPosition(nameBg.getPosition());
        txt.setScale(dynamicScale, dynamicScale);
        window.draw(txt);
    }

    auto drawArrow = [&](int charIdx, sf::Color col, float offsetX) {
        if (charIdx == -1) return;
        float cX = START_X + charIdx * (BOX_W + SPACING) + BOX_W / 2.f + offsetX;
        float cY = BOX_Y - 25.f + std::sin(blinkClk.getElapsedTime().asSeconds() * 6.f) * 5.f;

        sf::ConvexShape arrow(3);
        arrow.setPoint(0, sf::Vector2f(cX - 15.f, cY - 20.f));
        arrow.setPoint(1, sf::Vector2f(cX + 15.f, cY - 20.f));
        arrow.setPoint(2, sf::Vector2f(cX, cY));
        arrow.setFillColor(col);
        window.draw(arrow);

        DrawCentredText(window, font, col == Cyber::Cyan ? "P1" : "P2", 20, col, cX, cY - 35.f);
        };

    int drawP1 = (p1Char != -1) ? p1Char : ((selectionStep == 0 && hoveredChar != -1) ? hoveredChar : -1);
    int drawP2 = (p2Char != -1) ? p2Char : ((selectionStep == 2 && hoveredChar != -1) ? hoveredChar : -1);

    if (drawP1 != -1 && drawP1 == drawP2) {
        drawArrow(drawP1, Cyber::Cyan, -30.f);
        drawArrow(drawP2, Cyber::Magenta, 30.f);
    }
    else {
        drawArrow(drawP1, Cyber::Cyan, 0.f);
        if (isPVP) drawArrow(drawP2, Cyber::Magenta, 0.f);
    }

    // ── [SỬA] BỐ CỤC KHUNG NHẬP TÊN TÙY THEO CHẾ ĐỘ ──
    float NAME_Y = 520.f, NAME_W = 350.f, NAME_H = 60.f;

    if (!isPVP) {
        // PVE: Một khung tên đặt ở GIỮA, ẩn chữ Player 1
        float P1_X = W / 2.f - NAME_W / 2.f;
        bool typeP1 = (typingState == 1);
        DrawNeonRect(window, P1_X, NAME_Y, NAME_W, NAME_H, typeP1 ? sf::Color(20, 50, 70) : sf::Color::Black, Cyber::Cyan, typeP1 ? 3.f : 1.f);
        DrawCentredText(window, font, p1Name.empty() ? "Enter your name..." : p1Name, 26, typeP1 ? Cyber::White : Cyber::Gray, P1_X + NAME_W / 2.f, NAME_Y + 30.f);
    }
    else {
        // PVP: Hai khung chia 2 bên
        float P1_X = W / 2.f - NAME_W - 30.f;
        float P2_X = W / 2.f + 30.f;

        bool typeP1 = (typingState == 1);
        DrawNeonRect(window, P1_X, NAME_Y, NAME_W, NAME_H, typeP1 ? sf::Color(20, 50, 70) : sf::Color::Black, Cyber::Cyan, typeP1 ? 3.f : 1.f);
        DrawCentredText(window, font, "PLAYER 1", 20, Cyber::Cyan, P1_X + NAME_W / 2.f, NAME_Y - 15.f);
        DrawCentredText(window, font, p1Name.empty() ? "Enter P1 name..." : p1Name, 26, typeP1 ? Cyber::White : Cyber::Gray, P1_X + NAME_W / 2.f, NAME_Y + 30.f);

        bool typeP2 = (typingState == 2);
        DrawNeonRect(window, P2_X, NAME_Y, NAME_W, NAME_H, typeP2 ? sf::Color(70, 20, 50) : sf::Color::Black, Cyber::Magenta, typeP2 ? 3.f : 1.f);
        DrawCentredText(window, font, "PLAYER 2", 20, Cyber::Magenta, P2_X + NAME_W / 2.f, NAME_Y - 15.f);
        DrawCentredText(window, font, p2Name.empty() ? "Enter P2 name..." : p2Name, 26, typeP2 ? Cyber::White : Cyber::Gray, P2_X + NAME_W / 2.f, NAME_Y + 30.f);
    }

    // ── NÚT BẮT ĐẦU ──
    float btnX = W / 2.f - 150.f, btnY = 640.f;
    bool startHov = (mp.x >= btnX && mp.x <= btnX + 300.f && mp.y >= btnY && mp.y <= btnY + 70.f);
    sf::Color startCol = (selectionStep == 4) ? Cyber::Yellow : Cyber::Gray;
    DrawNeonRect(window, btnX, btnY, 300.f, 70.f, startHov && (selectionStep == 4) ? sf::Color(80, 70, 20) : Cyber::BgBtn, startCol, startHov && (selectionStep == 4) ? 3.f : 1.f);
    DrawCentredText(window, font, "START GAME", 28, startCol, W / 2.f, btnY + 35.f);
}
void DrawAbout(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);
    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    static sf::Clock fxClock;
    float t = fxClock.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 2.0f);
    float pulse2 = 0.5f + 0.5f * std::sin(t * 3.1f + 1.0f);

    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 18));

    // subtle HUD circles in background
    for (int i = 0; i < 3; ++i)
    {
        float r = 215.f + i * 48.f;
        sf::CircleShape ring(r, 100);
        ring.setOrigin(r, r);
        ring.setPosition(W / 2.f, 390.f);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(1.f);
        ring.setOutlineColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(12 - i * 3)));
        window.draw(ring);
    }

    // title + side marks
    sf::Text title("ABOUT US", font, 58);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(Cyber::Yellow);
    sf::FloatRect tr = title.getLocalBounds();
    title.setOrigin(tr.left + tr.width / 2.f, 0.f);
    title.setPosition(W / 2.f, 58.f);
    window.draw(title);

    sf::Text titleGlow = title;
    titleGlow.setFillColor(sf::Color(255, 230, 60, static_cast<sf::Uint8>(55 + pulse * 35)));
    titleGlow.setPosition(W / 2.f + 2.f, 60.f);
    window.draw(titleGlow);

    DrawCentredText(window, font, ">>", 28, sf::Color(0, 255, 255, static_cast<sf::Uint8>(170 + pulse * 70)), W / 2.f - 260.f, 80.f + std::sin(t * 3.f) * 2.f);
    DrawCentredText(window, font, "<<", 28, sf::Color(0, 255, 255, static_cast<sf::Uint8>(170 + pulse * 70)), W / 2.f + 260.f, 80.f - std::sin(t * 3.f) * 2.f);

    // main identity card panel
    float bW = 840.f, bH = 520.f;
    float bX = (W - bW) / 2.f;
    float bY = 160.f;

    DrawNeonRect(window, bX, bY, bW, bH, Cyber::BgPanel, Cyber::Cyan, 1.5f);
    DrawCornerBrackets(window, bX, bY, bW, bH, Cyber::Cyan, 18.f, 2.f);

    // subtle sweep line across panel
    float sweepY = bY + 40.f + std::fmod(t * 110.f, bH - 80.f);
    sf::RectangleShape sweep({ bW - 36.f, 1.5f });
    sweep.setPosition(bX + 18.f, sweepY);
    sweep.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(10 + pulse2 * 18)));
    window.draw(sweep);

    // header strip
    DrawSectionHeader(window, font, "[ PROJECT IDENTITY CARD ]", bX + 30.f, bY + 22.f, bW - 60.f, Cyber::Cyan);
    sf::CircleShape dot(4.f);
    dot.setOrigin(4.f, 4.f);
    dot.setPosition(bX + bW - 40.f, bY + 30.f);
    dot.setFillColor(sf::Color(80, 255, 170, static_cast<sf::Uint8>(150 + pulse * 105)));
    window.draw(dot);

    auto drawLabel = [&](const std::string& lbl, float x, float y)
        {
            sf::Text t(lbl, font, 14);
            t.setStyle(sf::Text::Bold);
            t.setFillColor(Cyber::Cyan);
            t.setPosition(x, y);
            window.draw(t);
        };
    auto drawValue = [&](const std::string& val, float x, float y, unsigned size = 22, sf::Color col = Cyber::White, bool bold = true)
        {
            sf::Text t(val, font, size);
            if (bold) t.setStyle(sf::Text::Bold);
            t.setFillColor(col);
            t.setPosition(x, y);
            window.draw(t);
        };

    // project info blocks
    float left = bX + 58.f;
    float top = bY + 74.f;
    float colGap = 310.f;

    drawLabel("PROJECT", left, top);
    drawValue("CARO MASTER", left, top + 22.f, 26, Cyber::Yellow);

    drawLabel("CLASS", left + colGap, top);
    drawValue("25CTT7 - HCMUS", left + colGap, top + 22.f, 24, Cyber::White);

    drawLabel("INSTRUCTOR", left, top + 86.f);
    drawValue("Mr. Truong Toan Thinh", left, top + 108.f, 22, Cyber::White);

    // development team section
    drawLabel("DEVELOPMENT TEAM", bX + 58.f, bY + 210.f);
    sf::RectangleShape divider({ bW - 116.f, 1.5f });
    divider.setPosition(bX + 58.f, bY + 232.f);
    divider.setFillColor(sf::Color(0, 255, 255, 85));
    window.draw(divider);

    // table header
    float tableX = bX + 58.f;
    float tableY = bY + 254.f;
    float tableW = bW - 116.f;
    float rowH = 34.f;

    sf::RectangleShape headBg({ tableW, 30.f });
    headBg.setPosition(tableX, tableY);
    headBg.setFillColor(sf::Color(10, 18, 34, 170));
    headBg.setOutlineThickness(1.f);
    headBg.setOutlineColor(sf::Color(0, 255, 255, 60));
    window.draw(headBg);

    drawLabel("NO.", tableX + 18.f, tableY + 5.f);
    drawLabel("STUDENT ID", tableX + 90.f, tableY + 5.f);
    drawLabel("FULL NAME", tableX + 290.f, tableY + 5.f);

    struct Member { const char* no; const char* id; const char* name; };
    Member members[] = {
        {"01", "24120164", "Nguyen The Anh"},
        {"02", "24120115", "Le Tan Phat"},
        {"03", "24120479", "Vu Duc Trung"},
        {"04", "24120311", "Nguyen Mai Tung Hieu"},
        {"05", "24120180", "Nguyen Dai Hieu"}
    };

    for (int i = 0; i < 5; ++i)
    {
        float y = tableY + 36.f + i * rowH;
        bool hov = (mp.x >= tableX && mp.x <= tableX + tableW && mp.y >= y && mp.y <= y + rowH - 2.f);

        sf::RectangleShape row({ tableW, rowH - 2.f });
        row.setPosition(tableX, y);
        row.setFillColor(hov ? sf::Color(12, 28, 48, 210) : sf::Color(8, 12, 22, 115));
        row.setOutlineThickness(1.f);
        row.setOutlineColor(hov ? sf::Color(0, 255, 255, 100) : sf::Color(60, 85, 120, 45));
        window.draw(row);

        sf::RectangleShape bar({ 3.f, rowH - 8.f });
        bar.setPosition(tableX + 10.f, y + 4.f);
        sf::Color barCol = (i % 2 == 0) ? Cyber::Cyan : Cyber::Magenta;
        if (i == 3) barCol = sf::Color(80, 255, 180);
        if (i == 4) barCol = sf::Color(170, 190, 255);
        bar.setFillColor(sf::Color(barCol.r, barCol.g, barCol.b, hov ? 255 : 170));
        window.draw(bar);

        drawValue(members[i].no, tableX + 30.f, y + 4.f, 18, Cyber::Yellow);
        drawValue(members[i].id, tableX + 90.f, y + 5.f, 16, Cyber::White, false);
        drawValue(members[i].name, tableX + 230.f, y + 5.f, 16, hov ? Cyber::White : sf::Color(220, 225, 240), true);
    }

    // footer area
    sf::RectangleShape footerLine({ bW - 116.f, 1.2f });
    footerLine.setPosition(bX + 58.f, bY + bH - 56.f);
    footerLine.setFillColor(sf::Color(0, 255, 255, 60));
    window.draw(footerLine);

    sf::Text footer("CARO MASTER // THANK YOU FOR PLAYING", font, 18);
    footer.setStyle(sf::Text::Bold);
    footer.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(165 + pulse * 70)));
    sf::FloatRect fb = footer.getLocalBounds();
    footer.setOrigin(fb.left + fb.width / 2.f, 0.f);
    footer.setPosition(W / 2.f, bY + bH - 44.f);
    window.draw(footer);

    sf::Text sub("Simple project information card for the development team", font, 14);
    sub.setFillColor(sf::Color(110, 135, 165));
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setOrigin(sb.left + sb.width / 2.f, 0.f);
    sub.setPosition(W / 2.f, bY + bH - 18.f);
    window.draw(sub);

    // Back button kept at original hitbox position
    const float BW = 250.f, BH = 55.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = bY + bH + 30.f;
    bool backHov = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);

    DrawNeonRect(window, BX, BY, BW, BH,
        backHov ? sf::Color(18, 30, 55) : Cyber::BgBtn,
        backHov ? Cyber::Magenta : Cyber::Grid,
        2.f);
    DrawCentredText(window, font, "BACK", 22, backHov ? Cyber::White : Cyber::Gray, BX + BW / 2.f, BY + BH / 2.f);
}

void DrawConfirmMainMenuOverlay(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    static sf::Clock confirmClock;
    float t = confirmClock.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 4.2f);

    sf::RectangleShape dim({ W, H });
    dim.setFillColor(sf::Color(0, 0, 0, 155));
    window.draw(dim);

    const float boxW = 560.f;
    const float boxH = 250.f;
    const float boxX = W / 2.f - boxW / 2.f;
    const float boxY = H / 2.f - boxH / 2.f;

    DrawNeonRect(window, boxX - 7.f, boxY - 7.f, boxW + 14.f, boxH + 14.f,
        sf::Color::Transparent,
        sf::Color(255, 80, 130, static_cast<sf::Uint8>(45 + pulse * 65)),
        3.f);

    DrawNeonRect(window, boxX, boxY, boxW, boxH,
        sf::Color(8, 13, 25, 238),
        sf::Color(255, 80, 130, 210),
        2.f);

    DrawCornerBrackets(window, boxX, boxY, boxW, boxH,
        sf::Color(255, 80, 130, 230), 18.f, 2.f);

    DrawSectionHeader(window, font, "[ CONFIRM ACTION ]", boxX + 22.f, boxY + 20.f, boxW - 44.f,
        sf::Color(255, 90, 140, 230));

    DrawCentredText(window, font, "BACK MAIN MENU?", 34,
        Cyber::White, W / 2.f, boxY + 78.f);

    DrawCentredText(window, font, "Unsaved progress will be lost.", 19,
        sf::Color(170, 190, 220), W / 2.f, boxY + 116.f);

    const float btnW = 190.f;
    const float btnH = 55.f;
    const float gap = 34.f;
    float yesX = W / 2.f - btnW - gap / 2.f;
    float noX = W / 2.f + gap / 2.f;
    float btnY = boxY + 160.f;

    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    bool yesHover = (mp.x >= yesX && mp.x <= yesX + btnW && mp.y >= btnY && mp.y <= btnY + btnH);
    bool noHover = (mp.x >= noX && mp.x <= noX + btnW && mp.y >= btnY && mp.y <= btnY + btnH);

    sf::Color yesCol = sf::Color(255, 80, 130);
    sf::Color noCol = sf::Color(0, 255, 255);

    Draw3DSciFiButton(window, yesX, btnY, btnW, btnH,
        yesHover ? sf::Color(60, 18, 32, 235) : sf::Color(22, 15, 25, 230),
        yesCol, yesHover ? 2.4f : 1.3f, yesHover, yesCol);

    Draw3DSciFiButton(window, noX, btnY, btnW, btnH,
        noHover ? sf::Color(10, 42, 52, 235) : sf::Color(14, 22, 32, 230),
        noCol, noHover ? 2.4f : 1.3f, noHover, noCol);

    DrawCentredText(window, font, "YES", 22,
        yesHover ? Cyber::White : sf::Color(255, 165, 195),
        yesX + btnW / 2.f, btnY + btnH / 2.f);

    DrawCentredText(window, font, "NO", 22,
        noHover ? Cyber::White : sf::Color(150, 240, 255),
        noX + btnW / 2.f, btnY + btnH / 2.f);
}


void DrawConfirmNewGameOverlay(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    static sf::Clock confirmClock;
    float t = confirmClock.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 4.2f);

    sf::RectangleShape dim({ W, H });
    dim.setFillColor(sf::Color(0, 0, 0, 155));
    window.draw(dim);

    const float boxW = 560.f;
    const float boxH = 250.f;
    const float boxX = W / 2.f - boxW / 2.f;
    const float boxY = H / 2.f - boxH / 2.f;

    DrawNeonRect(window, boxX - 7.f, boxY - 7.f, boxW + 14.f, boxH + 14.f,
        sf::Color::Transparent,
        sf::Color(255, 220, 0, static_cast<sf::Uint8>(45 + pulse * 65)),
        3.f);

    DrawNeonRect(window, boxX, boxY, boxW, boxH,
        sf::Color(8, 13, 25, 238),
        sf::Color(255, 220, 0, 210),
        2.f);

    DrawCornerBrackets(window, boxX, boxY, boxW, boxH,
        sf::Color(255, 220, 0, 230), 18.f, 2.f);

    DrawSectionHeader(window, font, "[ CONFIRM NEW GAME ]", boxX + 22.f, boxY + 20.f, boxW - 44.f,
        sf::Color(255, 220, 0, 230));

    DrawCentredText(window, font, "START NEW GAME?", 34,
        Cyber::White, W / 2.f, boxY + 78.f);

    DrawCentredText(window, font, "Current unsaved game will be lost.", 19,
        sf::Color(170, 190, 220), W / 2.f, boxY + 116.f);

    const float btnW = 190.f;
    const float btnH = 55.f;
    const float gap = 34.f;
    float yesX = W / 2.f - btnW - gap / 2.f;
    float noX = W / 2.f + gap / 2.f;
    float btnY = boxY + 160.f;

    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    bool yesHover = (mp.x >= yesX && mp.x <= yesX + btnW && mp.y >= btnY && mp.y <= btnY + btnH);
    bool noHover = (mp.x >= noX && mp.x <= noX + btnW && mp.y >= btnY && mp.y <= btnY + btnH);

    sf::Color yesCol = Cyber::Yellow;
    sf::Color noCol = Cyber::Cyan;

    Draw3DSciFiButton(window, yesX, btnY, btnW, btnH,
        yesHover ? sf::Color(62, 48, 10, 235) : sf::Color(25, 22, 14, 230),
        yesCol, yesHover ? 2.4f : 1.3f, yesHover, yesCol);

    Draw3DSciFiButton(window, noX, btnY, btnW, btnH,
        noHover ? sf::Color(10, 42, 52, 235) : sf::Color(14, 22, 32, 230),
        noCol, noHover ? 2.4f : 1.3f, noHover, noCol);

    DrawCentredText(window, font, "YES", 22,
        yesHover ? Cyber::White : sf::Color(255, 235, 150),
        yesX + btnW / 2.f, btnY + btnH / 2.f);

    DrawCentredText(window, font, "NO", 22,
        noHover ? Cyber::White : sf::Color(150, 240, 255),
        noX + btnW / 2.f, btnY + btnH / 2.f);
}

void DrawPauseOverlay(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    static sf::Clock clk;
    float t = clk.getElapsedTime().asSeconds();
    float pulse = 0.5f + 0.5f * std::sin(t * 3.2f);

    sf::RectangleShape dim({ W, H });
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);

    const float boxW = 470.f;
    const float boxH = 405.f;
    const float boxX = std::max(24.f, W / 2.f - boxW / 2.f);
    const float boxY = std::max(24.f, H / 2.f - boxH / 2.f - 4.f);

    DrawNeonRect(window, boxX - 8.f, boxY - 8.f, boxW + 16.f, boxH + 16.f,
        sf::Color::Transparent,
        sf::Color(0, 255, 255, static_cast<sf::Uint8>(45 + pulse * 70)),
        3.f);

    DrawNeonRect(window, boxX, boxY, boxW, boxH,
        sf::Color(7, 12, 24, 240),
        Cyber::Cyan,
        2.f);

    DrawCornerBrackets(window, boxX, boxY, boxW, boxH, Cyber::Cyan, 20.f, 2.2f);

    DrawCentredText(window, font, "GAME PAUSED", 40, Cyber::Yellow,
        boxX + boxW / 2.f, boxY + 50.f);

    DrawCentredText(window, font, "Press ESC / P to resume", 16, sf::Color(145, 170, 210),
        boxX + boxW / 2.f, boxY + 86.f);

    const char* labels[4] = {
        "RESUME",
        "NEW GAME",
        "SETTINGS",
        "MAIN MENU"
    };

    sf::Color accents[4] = {
        Cyber::Cyan,
        Cyber::Yellow,
        Cyber::Magenta,
        Cyber::NeonRed
    };

    const float btnW = 280.f;
    const float btnH = 50.f;
    const float gapY = 13.f;
    const float startX = boxX + boxW / 2.f - btnW / 2.f;
    const float startY = boxY + 118.f;

    sf::Vector2f mp = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    for (int i = 0; i < 4; ++i)
    {
        float y = startY + i * (btnH + gapY);
        bool hov = mp.x >= startX && mp.x <= startX + btnW && mp.y >= y && mp.y <= y + btnH;
        sf::Color accent = accents[i];

        if (hov)
        {
            DrawNeonRect(window, startX - 5.f, y - 5.f, btnW + 10.f, btnH + 10.f,
                sf::Color::Transparent,
                sf::Color(accent.r, accent.g, accent.b, 65),
                2.2f);
        }

        DrawNeonRect(window, startX, y, btnW, btnH,
            hov ? sf::Color(22, 34, 58, 240) : sf::Color(12, 18, 32, 230),
            hov ? accent : sf::Color(55, 75, 110, 190),
            hov ? 2.f : 1.2f);

        DrawCentredText(window, font, labels[i], 21,
            hov ? Cyber::White : sf::Color(170, 190, 220),
            startX + btnW / 2.f,
            y + btnH / 2.f);
    }
}
