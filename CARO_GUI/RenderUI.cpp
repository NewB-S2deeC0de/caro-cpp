#include "RenderUI.h"
#include "Constants.h"
#include "CaroAPI.h"
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdlib>
static int  s_lastMoveX  = -1;
static int  s_lastMoveY  = -1;
static int  s_lastPlayer = 0;    
static sf::Clock s_flashClock;
 
void SetLastMove(int x, int y, int player)
{
    s_lastMoveX  = x;
    s_lastMoveY  = y;
    s_lastPlayer = player;
    s_flashClock.restart();
}
 
void ResetLastMove()
{
    s_lastMoveX  = -1;
    s_lastMoveY  = -1;
    s_lastPlayer = 0;
}

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
    for (int i = 0; i < 3; ++i) {
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
        "PVP - 2 Nguoi Choi",
        "PVE - Choi voi May",
        "Cai Dat",
        "Tai Game (Load)",
        "Thong Tin (About)",
        "Thoat"
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

void DrawInGamePanel(sf::RenderWindow& window, const sf::Font& font, float timeRemaining, bool isPlayerTurn, int gameStatus, int boardSize, GameMode gameMode, int undoLeft[2], float saveNotifTimer, int p1Char, int p2Char, const std::string& p1Name, const std::string& p2Name, sf::Sprite charSprites[4])
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);
    int cellSz = GetDynCellSize(boardSize);
    float boardLeft = static_cast<float>(Config::OFFSET_X);
    float boardW = boardSize * cellSz;

    // Giữ nguyên tọa độ lấy theo Config để không làm hỏng click chuột của InputUI
    const float CARD_W = static_cast<float>(Config::PANEL_W);
    const float CARD_H = 340.f;
    const float pX_right = PanelX(boardSize);

    // Ép bảng trái nằm giữa lề trái và bàn cờ
    float pX_left = (boardLeft - CARD_W) / 2.f;
    if (pX_left < 10.f) pX_left = 10.f; // Chống tràn màn hình

    const float pY = 40.f; // Đẩy lên trên một chút để chừa chỗ cho nút bấm

    // ==========================================
    // ── ĐỒNG HỒ & KẾT QUẢ Ở GIỮA TRÊN CÙNG ──
    // ==========================================
    float boardCenterX = boardLeft + boardW / 2.f;
    sf::Color timerColor = (timeRemaining > 20.f) ? Cyber::Cyan : (timeRemaining > 10.f) ? Cyber::Yellow : Cyber::NeonRed;
    DrawCentredText(window, font, "THOI GIAN: " + std::to_string((int)timeRemaining) + "s", 32, timerColor, boardCenterX, 30.f);

    if (gameStatus != 0) {
        std::string s;
        if (gameMode == GameMode::PVP) s = (gameStatus == 1) ? p1Name + " THANG!" : ((gameStatus == 2) ? p2Name + " THANG!" : "HOA!");
        else s = (gameStatus == 1) ? p1Name + " THANG!" : ((gameStatus == 2) ? "MAY THANG!" : "HOA!");

        DrawNeonRect(window, boardCenterX - 180.f, H / 2.f - 50.f, 360.f, 100.f, sf::Color(40, 40, 10, 230), Cyber::Yellow, 3.f);
        DrawCornerBrackets(window, boardCenterX - 180.f, H / 2.f - 50.f, 360.f, 100.f, Cyber::Yellow, 20.f, 3.f);
        DrawCentredText(window, font, s, 36, Cyber::Yellow, boardCenterX, H / 2.f);
    }

    if (saveNotifTimer > 0.f) {
        int alpha = saveNotifTimer < 0.5f ? static_cast<int>((saveNotifTimer / 0.5f) * 255.f) : 255;
        DrawNeonRect(window, boardCenterX - 200.f, H / 2.f - 40.f, 400.f, 80.f, sf::Color(10, 10, 10, alpha), sf::Color(50, 255, 50, alpha), 3.f);
        sf::Text notifTxt("DA LUU VAN GAME!", font, 36);
        notifTxt.setFillColor(sf::Color(255, 255, 255, alpha));
        notifTxt.setStyle(sf::Text::Bold);
        sf::FloatRect nr = notifTxt.getLocalBounds();
        notifTxt.setOrigin(nr.left + nr.width / 2.f, nr.top + nr.height / 2.f);
        notifTxt.setPosition(boardCenterX, H / 2.f);
        window.draw(notifTxt);
    }

    // ==========================================
    // ── BẢNG BÊN TRÁI (PLAYER 1 - CẦM X) ──
    // ==========================================
    bool p1Active = (gameStatus == 0) && isPlayerTurn;
    sf::Color p1Col = p1Active ? Cyber::Cyan : sf::Color(50, 60, 70, 150);

    DrawNeonRect(window, pX_left, pY, CARD_W, CARD_H, sf::Color(10, 15, 25, 200), p1Col, p1Active ? 3.f : 1.f);
    if (p1Active) DrawCornerBrackets(window, pX_left, pY, CARD_W, CARD_H, Cyber::Cyan, 15.f, 2.f);

    if (p1Char != -1) {
        float avSz = 140.f;
        float avX = pX_left + (CARD_W - avSz) / 2.f;
        float avY = pY + 20.f;

        sf::Color tint = p1Active ? sf::Color::White : sf::Color(60, 60, 60, 150);
        charSprites[p1Char].setColor(tint);
        sf::FloatRect bounds = charSprites[p1Char].getLocalBounds();
        charSprites[p1Char].setScale(avSz / bounds.width, avSz / bounds.height);
        charSprites[p1Char].setPosition(avX, avY);
        window.draw(charSprites[p1Char]);
        charSprites[p1Char].setColor(sf::Color::White);

        DrawNeonRect(window, avX, avY, avSz, avSz, sf::Color::Transparent, p1Col, 2.f);
    }

    std::string name1 = p1Name.empty() ? "PLAYER 1" : p1Name;
    DrawCentredText(window, font, name1, 24, p1Active ? Cyber::White : Cyber::Gray, pX_left + CARD_W / 2.f, pY + 185.f);

    DrawNeonRect(window, pX_left + 40.f, pY + 210.f, CARD_W - 80.f, 30.f, sf::Color(0, 0, 0, 150), p1Col, 1.f);
    DrawCentredText(window, font, "PHE: X", 20, p1Col, pX_left + CARD_W / 2.f, pY + 225.f);

    DrawCentredText(window, font, "So luot di: --", 18, p1Active ? Cyber::White : Cyber::Gray, pX_left + CARD_W / 2.f, pY + 270.f);

    DrawCentredText(window, font, "LUOT UNDO:", 16, Cyber::Gray, pX_left + CARD_W / 2.f, pY + 305.f);
    float dotsX1 = pX_left + CARD_W / 2.f - (Config::UNDO_MAX * 20.f) / 2.f;
    for (int k = 0; k < Config::UNDO_MAX; ++k) {
        sf::CircleShape dot(6.f);
        dot.setOrigin(6.f, 6.f);
        dot.setPosition(dotsX1 + k * 20.f + 10.f, pY + 325.f);
        if (k < undoLeft[0]) {
            dot.setFillColor(p1Col);
            dot.setOutlineThickness(0);
        }
        else {
            dot.setFillColor(sf::Color(20, 25, 40));
            dot.setOutlineThickness(1.5f);
            dot.setOutlineColor(sf::Color(50, 60, 90));
        }
        window.draw(dot);
    }

    // ==========================================
    // ── BẢNG BÊN PHẢI (PLAYER 2 / AI - CẦM O) ──
    // ==========================================
    bool p2Active = (gameStatus == 0) && !isPlayerTurn;
    sf::Color p2Col = p2Active ? Cyber::Magenta : sf::Color(70, 50, 60, 150);

    DrawNeonRect(window, pX_right, pY, CARD_W, CARD_H, sf::Color(10, 15, 25, 200), p2Col, p2Active ? 3.f : 1.f);
    if (p2Active) DrawCornerBrackets(window, pX_right, pY, CARD_W, CARD_H, Cyber::Magenta, 15.f, 2.f);

    int drawP2Char = (gameMode == GameMode::PVE && p2Char == -1) ? 1 : p2Char;
    if (drawP2Char != -1) {
        float avSz = 140.f;
        float avX = pX_right + (CARD_W - avSz) / 2.f;
        float avY = pY + 20.f;

        sf::Color tint = p2Active ? sf::Color::White : sf::Color(80, 80, 80, 150);
        charSprites[drawP2Char].setColor(tint);
        sf::FloatRect bounds = charSprites[drawP2Char].getLocalBounds();
        charSprites[drawP2Char].setScale(avSz / bounds.width, avSz / bounds.height);
        charSprites[drawP2Char].setPosition(avX, avY);
        window.draw(charSprites[drawP2Char]);
        charSprites[drawP2Char].setColor(sf::Color::White);

        DrawNeonRect(window, avX, avY, avSz, avSz, sf::Color::Transparent, p2Col, 2.f);
    }

    std::string name2 = (gameMode == GameMode::PVE) ? "MAY (AI)" : (p2Name.empty() ? "PLAYER 2" : p2Name);
    DrawCentredText(window, font, name2, 24, p2Active ? Cyber::White : Cyber::Gray, pX_right + CARD_W / 2.f, pY + 185.f);

    DrawNeonRect(window, pX_right + 40.f, pY + 210.f, CARD_W - 80.f, 30.f, sf::Color(0, 0, 0, 150), p2Col, 1.f);
    DrawCentredText(window, font, "PHE: O", 20, p2Col, pX_right + CARD_W / 2.f, pY + 225.f);

    DrawCentredText(window, font, "So luot di: --", 18, p2Active ? Cyber::White : Cyber::Gray, pX_right + CARD_W / 2.f, pY + 270.f);

    DrawCentredText(window, font, "LUOT UNDO:", 16, Cyber::Gray, pX_right + CARD_W / 2.f, pY + 305.f);
    float dotsX2 = pX_right + CARD_W / 2.f - (Config::UNDO_MAX * 20.f) / 2.f;
    for (int k = 0; k < Config::UNDO_MAX; ++k) {
        sf::CircleShape dot(6.f);
        dot.setOrigin(6.f, 6.f);
        dot.setPosition(dotsX2 + k * 20.f + 10.f, pY + 325.f);
        if (k < undoLeft[1]) {
            dot.setFillColor(p2Col);
            dot.setOutlineThickness(0);
        }
        else {
            dot.setFillColor(sf::Color(20, 25, 40));
            dot.setOutlineThickness(1.5f);
            dot.setOutlineColor(sf::Color(50, 60, 90));
        }
        window.draw(dot);
    }

    // ==========================================
    // ── CÁC NÚT BẤM (Nằm bên dưới bảng phải) ──
    // ==========================================
    const float BTN_START_Y = (gameMode == GameMode::PVP) ? 395.f : 420.f;
    const char* gameBtns[] = { "Undo", "Save Game", "Main Menu" };
    const sf::Color btnAccent[] = { Cyber::Cyan, Cyber::CyanDim, Cyber::MagDim };
    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));
    const float BTN_H = 52.0f;

    for (int i = 0; i < 3; ++i) {
        float bX = pX_right;
        float bY = BTN_START_Y + i * 80.f;
        bool hov = (mp.x >= bX && mp.x <= bX + CARD_W && mp.y >= bY && mp.y <= bY + BTN_H);

        DrawNeonRect(window, bX, bY, CARD_W, BTN_H, hov ? sf::Color(20, 35, 65, 240) : Cyber::BgBtn, hov ? btnAccent[i] : sf::Color(btnAccent[i].r, btnAccent[i].g, btnAccent[i].b, 70), hov ? 2.f : 1.f);
        if (hov) DrawCornerBrackets(window, bX, bY, CARD_W, BTN_H, btnAccent[i], 8.f, 1.5f);

        DrawCentredText(window, font, gameBtns[i], 22, hov ? Cyber::White : sf::Color(160, 175, 200), bX + CARD_W / 2.f, bY + BTN_H / 2.f);
    }
}

void DrawBoard(sf::RenderWindow& window, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float ox = static_cast<float>(Config::OFFSET_X);
    float oy = static_cast<float>(Config::OFFSET_Y);
    float bW = static_cast<float>(boardSize * cellSz);
    float bH = static_cast<float>(boardSize * cellSz);

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
    float ox = static_cast<float>(Config::OFFSET_X);
    float oy = static_cast<float>(Config::OFFSET_Y);

    for (int x = 0; x < boardSize; ++x) {
        for (int y = 0; y < boardSize; ++y) {
            int cell = GetCell(x, y);
            if (!cell) continue;

            float cx = ox + x * cellSz + cellSz / 2.f;
            float cy = oy + y * cellSz + cellSz / 2.f;
            float arm = cellSz / 2.f - 6.f; // Thu nhỏ lại chút cho thanh thoát

            if (cell == 1) {
                // ── QUÂN X (CYAN) ──
                // Lớp Glow (Nhòe sáng)
                for (int rot : {45, -45}) {
                    sf::RectangleShape gl({ arm * 2.f, 6.f });
                    gl.setFillColor(sf::Color(0, 255, 255, 60));
                    gl.setOrigin(arm, 3.f);
                    gl.setPosition(cx, cy);
                    gl.setRotation(static_cast<float>(rot));
                    window.draw(gl);
                }
                // Lõi sáng nét căng
                for (int rot : {45, -45}) {
                    sf::RectangleShape ln({ arm * 2.f, 3.f });
                    ln.setFillColor(Cyber::Cyan);
                    ln.setOrigin(arm, 1.5f);
                    ln.setPosition(cx, cy);
                    ln.setRotation(static_cast<float>(rot));
                    window.draw(ln);
                }
                // Họa tiết kẹp góc
                float br = arm + 2.f;
                DrawCornerBrackets(window, cx - br, cy - br, br * 2.f, br * 2.f, sf::Color(0, 200, 220, 100), 6.f, 1.5f);
            }
            else if (cell == 2) {
                // ── QUÂN O (MAGENTA) ──
                // Lớp Glow (Nhòe sáng)
                sf::CircleShape glow(arm + 1.f);
                glow.setOrigin(arm + 1.f, arm + 1.f);
                glow.setPosition(cx, cy);
                glow.setFillColor(sf::Color::Transparent);
                glow.setOutlineThickness(5.f);
                glow.setOutlineColor(sf::Color(255, 0, 200, 60));
                window.draw(glow);

                // Lõi sáng nét căng
                sf::CircleShape ring(arm);
                ring.setOrigin(arm, arm);
                ring.setPosition(cx, cy);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(2.5f);
                ring.setOutlineColor(Cyber::Magenta);
                window.draw(ring);

                // Chấm năng lượng ở giữa
                sf::CircleShape dot(3.f);
                dot.setOrigin(3.f, 3.f);
                dot.setPosition(cx, cy);
                dot.setFillColor(sf::Color(255, 100, 200, 255));
                window.draw(dot);
            }
        }
    }
    if (s_lastMoveX >= 0 && s_lastMoveX < boardSize &&
        s_lastMoveY >= 0 && s_lastMoveY < boardSize)
    {
        float elapsed = s_flashClock.getElapsedTime().asSeconds();
        const float FLASH_DURATION = 2.5f;  // Tổng thời gian hiệu ứng (giây)
        const float PULSE_SPEED    = 6.5f;  // Tốc độ nhấp nháy (Hz)
 
        if (elapsed < FLASH_DURATION)
        {
            // Fade dần về 0 theo thời gian
            float fadeRatio = 1.f - (elapsed / FLASH_DURATION);
            // Xung sin để tạo nhịp sáng/tối
            float pulse     = (std::sin(elapsed * PULSE_SPEED) + 1.f) * 0.5f;
            float intensity = pulse * fadeRatio;
 
            // Chọn màu theo người vừa đánh
            sf::Color glowCol = (s_lastPlayer == 1) ? Cyber::Cyan : Cyber::Magenta;
 
            float rx = ox + s_lastMoveX * cellSz;
            float ry = oy + s_lastMoveY * cellSz;
            float sz = static_cast<float>(cellSz);
 
            // Lớp 1: Nền glow mờ phủ toàn ô
            sf::RectangleShape glowBg({ sz, sz });
            glowBg.setPosition(rx, ry);
            glowBg.setFillColor(sf::Color(
                glowCol.r, glowCol.g, glowCol.b,
                static_cast<sf::Uint8>(60.f * intensity)));
            window.draw(glowBg);
 
            // Lớp 2: Viền neon nhấp nháy
            sf::RectangleShape flashBorder({ sz, sz });
            flashBorder.setPosition(rx, ry);
            flashBorder.setFillColor(sf::Color::Transparent);
            flashBorder.setOutlineThickness(2.5f);
            flashBorder.setOutlineColor(sf::Color(
                glowCol.r, glowCol.g, glowCol.b,
                static_cast<sf::Uint8>(230.f * intensity)));
            window.draw(flashBorder);
 
            // Lớp 3: Dấu góc (corner brackets) sáng nhấp nháy
            DrawCornerBrackets(window,
                rx + 3.f, ry + 3.f, sz - 6.f, sz - 6.f,
                sf::Color(glowCol.r, glowCol.g, glowCol.b,
                    static_cast<sf::Uint8>(255.f * intensity)),
                7.f, 2.5f);
 
            // Lớp 4: Chấm nhỏ ở trung tâm ô (điểm đánh dấu)
            float cx2 = rx + sz / 2.f;
            float cy2 = ry + sz / 2.f;
            float dotR = 3.5f;
            sf::CircleShape dot(dotR);
            dot.setOrigin(dotR, dotR);
            dot.setPosition(cx2, cy2);
            dot.setFillColor(sf::Color(
                glowCol.r, glowCol.g, glowCol.b,
                static_cast<sf::Uint8>(255.f * intensity)));
            window.draw(dot);
        }
    }
}

// ============================================================
//  DrawHoverEffect
// ============================================================
void DrawHoverEffect(sf::RenderWindow& window, int gX, int gY, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);

    if (gX >= 0 && gX < boardSize && gY >= 0 && gY < boardSize) {
        float rx = static_cast<float>(Config::OFFSET_X + gX * cellSz);
        float ry = static_cast<float>(Config::OFFSET_Y + gY * cellSz);

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

void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY) { DrawWinLine(window, sX, sY, eX, eY, 15); }

void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY, int boardSize)
{
    if (sX == -1) return;
    int cs = GetDynCellSize(boardSize);
    float x1 = Config::OFFSET_X + sX * cs + cs / 2.f;
    float y1 = Config::OFFSET_Y + sY * cs + cs / 2.f;
    float x2 = Config::OFFSET_X + eX * cs + cs / 2.f;
    float y2 = Config::OFFSET_Y + eY * cs + cs / 2.f;

    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    float ang = std::atan2(dy, dx) * 180.f / 3.14159265f;

    sf::RectangleShape glow({ len, 10.f });
    glow.setFillColor(sf::Color(255, 220, 0, 40));
    glow.setOrigin(0, 5.f); glow.setPosition(x1, y1); glow.setRotation(ang);
    window.draw(glow);

    sf::RectangleShape line({ len, 3.5f });
    line.setFillColor(Cyber::Yellow);
    line.setOrigin(0, 1.75f); line.setPosition(x1, y1); line.setRotation(ang);
    window.draw(line);

    for (float ex : { x1, x2 }) {
        float ey = (ex == x1) ? y1 : y2;
        sf::CircleShape cap(5.f); cap.setOrigin(5.f, 5.f); cap.setPosition(ex, ey); cap.setFillColor(Cyber::Yellow);
        window.draw(cap);
    }
}

void DrawSettings(sf::RenderWindow& window, const sf::Font& font, int boardSize, bool ruleBlock2, int aiLevel, float sfxVolume, bool bgmEnabled)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    for (float gx = 0; gx < W; gx += 60.f) { sf::RectangleShape vl({ 1.f, H }); vl.setPosition(gx, 0); vl.setFillColor(sf::Color(0, 255, 255, 6)); window.draw(vl); }
    for (float gy = 0; gy < H; gy += 60.f) { sf::RectangleShape hl({ W, 1.f }); hl.setPosition(0, gy); hl.setFillColor(sf::Color(0, 255, 255, 6)); window.draw(hl); }
    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 14));

    DrawCentredText(window, font, "// CAI DAT HE THONG", 54, Cyber::Yellow, W / 2.f, 80.f);

    const char* labels[] = { "Kich thuoc ban co (10-30):", "Luat chan 2 dau:", "Do kho AI (1-3):", "Am luong SFX (0-100):", "Nhac nen BGM:" };
    std::string values[] = { std::to_string(boardSize), ruleBlock2 ? "BAT (ON)" : "TAT (OFF)", std::to_string(aiLevel), std::to_string((int)sfxVolume), bgmEnabled ? "BAT (ON)" : "TAT (OFF)" };

    const float SY = 200.f, LX = 250.f, CX = 650.f, RG = 80.f;

    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));

    for (int i = 0; i < 5; ++i) {
        float cy = SY + i * RG;
        sf::Text lbl(labels[i], font, 28); lbl.setFillColor(Cyber::White); lbl.setPosition(LX, cy); window.draw(lbl);

        if (i == 0 || i == 2 || i == 3) {
            bool hovM = (mp.x >= CX && mp.x <= CX + 50 && mp.y >= cy && mp.y <= cy + 50);
            DrawNeonRect(window, CX, cy, 50, 50, hovM ? sf::Color(80, 20, 20) : Cyber::BgBtn, Cyber::NeonRed, 1.5f);
            DrawCentredText(window, font, "-", 40, Cyber::NeonRed, CX + 25.f, cy + 20.f);

            DrawCentredText(window, font, values[i], 28, Cyber::Cyan, CX + 125.f, cy + 25.f);

            float pX2 = CX + 200.f;
            bool hovP = (mp.x >= pX2 && mp.x <= pX2 + 50 && mp.y >= cy && mp.y <= cy + 50);
            DrawNeonRect(window, pX2, cy, 50, 50, hovP ? sf::Color(20, 60, 20) : Cyber::BgBtn, sf::Color(50, 200, 100), 1.5f);
            DrawCentredText(window, font, "+", 40, sf::Color(50, 200, 100), pX2 + 25.f, cy + 20.f);
        }
        else {
            bool hovT = (mp.x >= CX && mp.x <= CX + 250 && mp.y >= cy && mp.y <= cy + 50);
            sf::Color tCol = (values[i] == "BAT (ON)") ? Cyber::Cyan : Cyber::NeonRed;
            DrawNeonRect(window, CX, cy, 250, 50, sf::Color(tCol.r, tCol.g, tCol.b, hovT ? 40u : 15u), tCol, 1.5f);
            if (hovT) DrawCornerBrackets(window, CX, cy, 250, 50, tCol, 8.f, 1.5f);
            DrawCentredText(window, font, values[i], 28, tCol, CX + 125.f, cy + 25.f);
        }
    }

    const float BW = 300.f, BH = 60.f;
    const float BX = Config::WIN_WIDTH / 2.f - BW / 2.f, BY = 650.f;
    bool bh = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);
    DrawNeonRect(window, BX, BY, BW, BH, bh ? sf::Color(20, 40, 70) : Cyber::BgBtn, bh ? Cyber::Magenta : sf::Color(180, 0, 150, 100), bh ? 2.f : 1.f);
    if (bh) DrawCornerBrackets(window, BX, BY, BW, BH, Cyber::Magenta, 10.f, 2.f);
    DrawCentredText(window, font, "QUAY LAI MENU", 24, bh ? Cyber::White : sf::Color(160, 175, 200), BX + BW / 2.f, BY + BH / 2.f);
}

void DrawLoadScreen(sf::RenderWindow& window, const sf::Font& font)
{
    DrawScanlines(window, 0, 0, Config::WIN_WIDTH, Config::WIN_HEIGHT, sf::Color(0, 0, 0, 14));
    sf::Text title("DANH SACH DIEM LUU", font, 45);
    title.setFillColor(Cyber::Yellow); title.setStyle(sf::Text::Bold); title.setPosition(80.f, 50.f); window.draw(title);

    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));

    int hoveredSlot = -1;
    const float BTN_W = 400.f, BTN_H = 75.f, START_X = 80.f, START_Y = 150.f, DEL_W = 70.f;

    for (int i = 1; i <= 5; ++i) {
        float bY = START_Y + (i - 1) * 95.f;
        bool hov = (mp.x >= START_X && mp.x <= START_X + BTN_W && mp.y >= bY && mp.y <= bY + BTN_H);
        if (hov) hoveredSlot = i;

        int size = 0, moves = 0, turn = 0; char gName[64] = "";
        bool hasData = PeekGameSlot(i, &size, &moves, &turn, gName);

        DrawNeonRect(window, START_X, bY, BTN_W, BTN_H, hov ? sf::Color(20, 40, 70, 230) : Cyber::BgBtn, hov ? Cyber::Cyan : Cyber::Grid, hov ? 3.f : 1.f);

        std::string slotText = (hasData && std::string(gName) != "") ? std::string(gName) : "Slot " + std::to_string(i);
        if (!hasData) slotText += " - [ TRONG ]";

        sf::Text txt(slotText, font, 22);
        txt.setFillColor(hasData ? Cyber::White : Cyber::Gray);
        txt.setPosition(START_X + 25.f, bY + BTN_H / 2.f - 15.f);
        window.draw(txt);

        if (hasData) {
            float delX = START_X + BTN_W + 10.f;
            bool delHov = (mp.x >= delX && mp.x <= delX + DEL_W && mp.y >= bY && mp.y <= bY + BTN_H);
            DrawNeonRect(window, delX, bY, DEL_W, BTN_H, delHov ? sf::Color(150, 30, 30) : sf::Color(80, 20, 20), Cyber::NeonRed, 1.5f);
            DrawCentredText(window, font, "X", 28, Cyber::White, delX + DEL_W / 2.f, bY + BTN_H / 2.f - 5.f);
        }
    }

    float backX = 80.f, backY = 680.f;
    bool backHov = (mp.x >= backX && mp.x <= backX + 200.f && mp.y >= backY && mp.y <= backY + 60.f);
    DrawNeonRect(window, backX, backY, 200.f, 60.f, backHov ? sf::Color(80, 30, 30) : Cyber::BgBtn, Cyber::Grid, 2.f);
    DrawCentredText(window, font, "<-- QUAY LAI", 22, Cyber::White, backX + 100.f, backY + 30.f);

    const float PREV_X = 620.f, PREV_Y = 150.f, PREV_W = 480.f, PREV_H = 550.f;
    DrawNeonRect(window, PREV_X, PREV_Y, PREV_W, PREV_H, Cyber::BgPanel, Cyber::Grid, 2.f);

    if (hoveredSlot != -1) {
        int tempBoard[30][30]; int bSize, moves, turn; char sDate[32], gName[64];
        if (GetSlotPreview(hoveredSlot, &bSize, &moves, &turn, sDate, gName, tempBoard)) {
            const float BOARD_AREA = 260.f;
            float mX = PREV_X + (PREV_W - BOARD_AREA) / 2.f, mY = PREV_Y + 30.f, mCellSz = BOARD_AREA / bSize;

            DrawNeonRect(window, mX, mY, BOARD_AREA, BOARD_AREA, sf::Color(6, 8, 16), Cyber::White, 2.f);

            for (int i = 1; i < bSize; ++i) {
                sf::RectangleShape vLine({ 1.f, BOARD_AREA }); vLine.setPosition(mX + i * mCellSz, mY); vLine.setFillColor(Cyber::Grid); window.draw(vLine);
                sf::RectangleShape hLine({ BOARD_AREA, 1.f }); hLine.setPosition(mX, mY + i * mCellSz); hLine.setFillColor(Cyber::Grid); window.draw(hLine);
            }

            float thickness = std::max(1.f, mCellSz / 6.f);
            for (int x = 0; x < bSize; ++x) {
                for (int y = 0; y < bSize; ++y) {
                    float cX = mX + x * mCellSz + mCellSz / 2.f, cY = mY + y * mCellSz + mCellSz / 2.f;
                    float sz = mCellSz / 2.f - mCellSz / 6.f;
                    if (tempBoard[x][y] == 1) {
                        for (int rot : {45, -45}) {
                            sf::RectangleShape l({ mCellSz * 0.7f, thickness }); l.setOrigin(mCellSz * 0.7f / 2.f, thickness / 2.f); l.setPosition(cX, cY); l.setFillColor(Cyber::Cyan); l.rotate(static_cast<float>(rot)); window.draw(l);
                        }
                    }
                    else if (tempBoard[x][y] == 2) {
                        sf::CircleShape circle(sz); circle.setOrigin(sz, sz); circle.setPosition(cX, cY); circle.setFillColor(sf::Color::Transparent); circle.setOutlineThickness(thickness); circle.setOutlineColor(Cyber::Magenta); window.draw(circle);
                    }
                }
            }

            float tY = mY + BOARD_AREA + 25.f, tPadding = 25.f;
            sf::Text infoTitle("THONG TIN VAN GAME", font, 24);
            infoTitle.setFillColor(Cyber::Yellow); infoTitle.setStyle(sf::Text::Bold | sf::Text::Underlined); infoTitle.setPosition(PREV_X + tPadding, tY); window.draw(infoTitle);

            std::string detailStr = " - Ten luu: " + std::string(gName) + "\n - Ngay luu: " + std::string(sDate) + "\n - Ban co: " + std::to_string(bSize) + "x" + std::to_string(bSize) + "\n - Da danh: " + std::to_string(moves) + " nuoc\n - Luot ke: " + (turn == 1 ? "Nguoi 1 (X)" : "Nguoi 2/May (O)");
            sf::Text infoTxt(detailStr, font, 20); infoTxt.setFillColor(Cyber::White); infoTxt.setPosition(PREV_X + tPadding, tY + 45.f); infoTxt.setLineSpacing(1.4f); window.draw(infoTxt);
        }
        else {
            DrawCentredText(window, font, "DIEM LUU TRONG", 28, Cyber::Gray, PREV_X + PREV_W / 2.f, PREV_Y + PREV_H / 2.f);
        }
    }
    else {
        DrawCentredText(window, font, "<- DI CHUOT VAO SLOT\n   DE XEM TRUOC", 24, Cyber::Gray, PREV_X + PREV_W / 2.f, PREV_Y + PREV_H / 2.f);
    }
}

void DrawSaveScreen(sf::RenderWindow& window, const sf::Font& font, bool isNaming, const std::string& inputName, sf::Clock& clock, bool isConfirmOverwrite)
{
    DrawScanlines(window, 0, 0, Config::WIN_WIDTH, Config::WIN_HEIGHT, sf::Color(0, 0, 0, 14));
    sf::Text title("CHON O DE LUU VAN GAME", font, 45);
    title.setFillColor(Cyber::Yellow); title.setStyle(sf::Text::Bold); title.setPosition(80.f, 50.f); window.draw(title);

    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));

    int hoveredSlot = -1;
    const float BTN_W = 480.f, BTN_H = 75.f, START_X = 80.f, START_Y = 150.f;

    for (int i = 1; i <= 5; ++i) {
        float bY = START_Y + (i - 1) * 95.f;
        bool hov = (mp.x >= START_X && mp.x <= START_X + BTN_W && mp.y >= bY && mp.y <= bY + BTN_H);
        if (hov && !isNaming) hoveredSlot = i;

        int size = 0, moves = 0, turn = 0; char gName[64] = "";
        bool hasData = PeekGameSlot(i, &size, &moves, &turn, gName);

        DrawNeonRect(window, START_X, bY, BTN_W, BTN_H, hov ? sf::Color(20, 40, 70, 230) : Cyber::BgBtn, hov ? Cyber::Cyan : Cyber::Grid, hov ? 3.f : 1.f);

        std::string slotText = (hasData && std::string(gName) != "") ? std::string(gName) : "Slot " + std::to_string(i);
        slotText += hasData ? " - [ GHI DE ]" : " - [ TRONG ]";

        sf::Text txt(slotText, font, 22);
        txt.setFillColor(hasData ? Cyber::Yellow : Cyber::Gray);
        txt.setPosition(START_X + 25.f, bY + BTN_H / 2.f - 15.f);
        window.draw(txt);
    }

    float backX = 80.f, backY = 680.f;
    bool backHov = (mp.x >= backX && mp.x <= backX + 200.f && mp.y >= backY && mp.y <= backY + 60.f);
    DrawNeonRect(window, backX, backY, 200.f, 60.f, backHov ? sf::Color(80, 30, 30) : Cyber::BgBtn, Cyber::Grid, 2.f);
    DrawCentredText(window, font, "<-- QUAY LAI", 22, Cyber::White, backX + 100.f, backY + 30.f);

    const float PREV_X = 620.f, PREV_Y = 150.f, PREV_W = 480.f, PREV_H = 550.f;
    DrawNeonRect(window, PREV_X, PREV_Y, PREV_W, PREV_H, Cyber::BgPanel, Cyber::Grid, 2.f);

    if (hoveredSlot != -1 && !isNaming) {
        int tempBoard[30][30]; int bSize, moves, turn; char sDate[32], gName[64];
        if (GetSlotPreview(hoveredSlot, &bSize, &moves, &turn, sDate, gName, tempBoard)) {
            const float BOARD_AREA = 260.f;
            float mX = PREV_X + (PREV_W - BOARD_AREA) / 2.f, mY = PREV_Y + 30.f, mCellSz = BOARD_AREA / bSize;

            DrawNeonRect(window, mX, mY, BOARD_AREA, BOARD_AREA, sf::Color(6, 8, 16), Cyber::White, 2.f);

            for (int i = 1; i < bSize; ++i) {
                sf::RectangleShape vLine({ 1.f, BOARD_AREA }); vLine.setPosition(mX + i * mCellSz, mY); vLine.setFillColor(Cyber::Grid); window.draw(vLine);
                sf::RectangleShape hLine({ BOARD_AREA, 1.f }); hLine.setPosition(mX, mY + i * mCellSz); hLine.setFillColor(Cyber::Grid); window.draw(hLine);
            }

            float thickness = std::max(1.f, mCellSz / 6.f);
            for (int x = 0; x < bSize; ++x) {
                for (int y = 0; y < bSize; ++y) {
                    float cX = mX + x * mCellSz + mCellSz / 2.f, cY = mY + y * mCellSz + mCellSz / 2.f;
                    float sz = mCellSz / 2.f - mCellSz / 6.f;
                    if (tempBoard[x][y] == 1) {
                        for (int rot : {45, -45}) { sf::RectangleShape l({ mCellSz * 0.7f, thickness }); l.setOrigin(mCellSz * 0.7f / 2.f, thickness / 2.f); l.setPosition(cX, cY); l.setFillColor(Cyber::Cyan); l.rotate(static_cast<float>(rot)); window.draw(l); }
                    }
                    else if (tempBoard[x][y] == 2) {
                        sf::CircleShape circle(sz); circle.setOrigin(sz, sz); circle.setPosition(cX, cY); circle.setFillColor(sf::Color::Transparent); circle.setOutlineThickness(thickness); circle.setOutlineColor(Cyber::Magenta); window.draw(circle);
                    }
                }
            }

            float tY = mY + BOARD_AREA + 25.f, tPadding = 25.f;
            sf::Text infoTitle("THONG TIN VAN GAME CU:", font, 24);
            infoTitle.setFillColor(Cyber::NeonRed); infoTitle.setStyle(sf::Text::Bold | sf::Text::Underlined); infoTitle.setPosition(PREV_X + tPadding, tY); window.draw(infoTitle);

            std::string detailStr = " - Ngay luu: " + std::string(sDate) + "\n - Ban co: " + std::to_string(bSize) + "x" + std::to_string(bSize) + "\n - Da danh: " + std::to_string(moves) + " nuoc\n\n CHU Y: LUU VAO DAY SE GHI DE!";
            sf::Text infoTxt(detailStr, font, 20); infoTxt.setFillColor(Cyber::White); infoTxt.setPosition(PREV_X + tPadding, tY + 45.f); infoTxt.setLineSpacing(1.4f); window.draw(infoTxt);
        }
        else {
            DrawCentredText(window, font, "O LUU TRONG\n\nSAN SANG LUU GAME!", 26, Cyber::Cyan, PREV_X + PREV_W / 2.f, PREV_Y + PREV_H / 2.f);
        }
    }
    else if (!isNaming) {
        DrawCentredText(window, font, "<- CHON 1 O DE LUU VAN GAME", 24, Cyber::Gray, PREV_X + PREV_W / 2.f, PREV_Y + PREV_H / 2.f);
    }

    if (isNaming) {
        sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(Config::WIN_WIDTH), static_cast<float>(Config::WIN_HEIGHT)));
        overlay.setFillColor(sf::Color(0, 0, 0, 200)); window.draw(overlay);

        float bX = Config::WIN_WIDTH / 2.f - 250.f, bY = Config::WIN_HEIGHT / 2.f - 125.f;
        DrawNeonRect(window, bX, bY, 500.f, 250.f, Cyber::BgPanel, Cyber::Cyan, 3.f);
        DrawCornerBrackets(window, bX, bY, 500.f, 250.f, Cyber::Cyan, 20.f, 3.f);

        DrawCentredText(window, font, "NHAP TEN VAN GAME", 24, Cyber::White, Config::WIN_WIDTH / 2.f, Config::WIN_HEIGHT / 2.f - 80.f);

        DrawNeonRect(window, Config::WIN_WIDTH / 2.f - 200.f, Config::WIN_HEIGHT / 2.f - 35.f, 400.f, 50.f, sf::Color::Black, Cyber::CyanDim, 1.5f);

        sf::Text inputText(inputName, font, 22);
        inputText.setFillColor(Cyber::Cyan);
        sf::FloatRect textBounds = inputText.getLocalBounds();
        inputText.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
        inputText.setPosition(Config::WIN_WIDTH / 2.f, Config::WIN_HEIGHT / 2.f - 10.f);
        window.draw(inputText);

        static sf::Clock blinkClock;
        static size_t lastLength = inputName.length();
        if (inputName.length() != lastLength) {
            blinkClock.restart();
            lastLength = inputName.length();
        }

        if (blinkClock.getElapsedTime().asMilliseconds() % 1000 < 500) {
            sf::RectangleShape cursor(sf::Vector2f(2.f, 24.f));
            cursor.setFillColor(Cyber::Yellow);
            float cursorX = (Config::WIN_WIDTH / 2.f) + (inputName.empty() ? 0 : textBounds.width / 2.f + 4.f);
            cursor.setOrigin(1.f, 12.f);
            cursor.setPosition(cursorX, Config::WIN_HEIGHT / 2.f - 10.f);
            window.draw(cursor);
        }

        DrawNeonRect(window, Config::WIN_WIDTH / 2.f - 100.f, Config::WIN_HEIGHT / 2.f + 35.f, 200.f, 50.f, sf::Color(20, 60, 20), sf::Color(50, 200, 100), 2.f);
        DrawCentredText(window, font, "CHAP NHAN", 20, Cyber::White, Config::WIN_WIDTH / 2.f, Config::WIN_HEIGHT / 2.f + 60.f);

        if (isConfirmOverwrite) {
            sf::RectangleShape overlay2(sf::Vector2f(static_cast<float>(Config::WIN_WIDTH), static_cast<float>(Config::WIN_HEIGHT)));
            overlay2.setFillColor(sf::Color(0, 0, 0, 200)); window.draw(overlay2);

            float cw = 500.f, ch = 200.f;
            float cx = Config::WIN_WIDTH / 2.f - cw / 2.f;
            float cy = Config::WIN_HEIGHT / 2.f - ch / 2.f;

            DrawNeonRect(window, cx, cy, cw, ch, Cyber::BgPanel, Cyber::Yellow, 3.f);
            DrawCornerBrackets(window, cx, cy, cw, ch, Cyber::Yellow, 15.f, 2.f);

            DrawCentredText(window, font, "TEN DA TON TAI!", 26, Cyber::NeonRed, Config::WIN_WIDTH / 2.f, cy + 30.f);
            DrawCentredText(window, font, "Ban co muon ghi de len file", 20, Cyber::White, Config::WIN_WIDTH / 2.f, cy + 70.f);
            DrawCentredText(window, font, "game mang ten nay khong?", 20, Cyber::White, Config::WIN_WIDTH / 2.f, cy + 100.f);

            float btnW = 120.f, btnH = 50.f;
            float yesX = Config::WIN_WIDTH / 2.f - 140.f, yesY = Config::WIN_HEIGHT / 2.f + 30.f;
            float noX = Config::WIN_WIDTH / 2.f + 20.f, noY = Config::WIN_HEIGHT / 2.f + 30.f;

            bool hovYes = (mp.x >= yesX && mp.x <= yesX + btnW && mp.y >= yesY && mp.y <= yesY + btnH);
            DrawNeonRect(window, yesX, yesY, btnW, btnH, hovYes ? sf::Color(80, 20, 20) : sf::Color(30, 10, 10), Cyber::NeonRed, 2.f);
            DrawCentredText(window, font, "CO", 20, Cyber::White, yesX + btnW / 2.f, yesY + btnH / 2.f);

            bool hovNo = (mp.x >= noX && mp.x <= noX + btnW && mp.y >= noY && mp.y <= noY + btnH);
            DrawNeonRect(window, noX, noY, btnW, btnH, hovNo ? sf::Color(20, 60, 20) : sf::Color(10, 30, 10), sf::Color(50, 200, 100), 2.f);
            DrawCentredText(window, font, "KHONG", 20, Cyber::White, noX + btnW / 2.f, noY + btnH / 2.f);
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

    DrawCentredText(window, font, isPVP ? "CHON NHAN VAT & DAT TEN" : "CHON NHAN VAT", 46, Cyber::Yellow, W / 2.f, 50.f);

    std::string stepStr = "";
    if (selectionStep == 0) stepStr = isPVP ? ">> PLAYER 1: CLICK CHON ROI NHAN [ENTER] <<" : ">> CLICK CHON NHAN VAT ROI NHAN [ENTER] <<";
    else if (selectionStep == 1) stepStr = isPVP ? ">> PLAYER 1: NHAP TEN ROI NHAN [ENTER] <<" : ">> NHAP TEN ROI NHAN [ENTER] <<";
    else if (selectionStep == 2) stepStr = ">> PLAYER 2: CLICK CHON ROI NHAN [ENTER] <<";
    else if (selectionStep == 3) stepStr = ">> PLAYER 2: NHAP TEN ROI NHAN [ENTER] <<";
    else if (selectionStep == 4) stepStr = ">> NHAN [ENTER] DE BAT DAU <<";

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
        DrawCentredText(window, font, p1Name.empty() ? "Nhap ten cua ban..." : p1Name, 26, typeP1 ? Cyber::White : Cyber::Gray, P1_X + NAME_W / 2.f, NAME_Y + 30.f);
    }
    else {
        // PVP: Hai khung chia 2 bên
        float P1_X = W / 2.f - NAME_W - 30.f;
        float P2_X = W / 2.f + 30.f;

        bool typeP1 = (typingState == 1);
        DrawNeonRect(window, P1_X, NAME_Y, NAME_W, NAME_H, typeP1 ? sf::Color(20, 50, 70) : sf::Color::Black, Cyber::Cyan, typeP1 ? 3.f : 1.f);
        DrawCentredText(window, font, "PLAYER 1", 20, Cyber::Cyan, P1_X + NAME_W / 2.f, NAME_Y - 15.f);
        DrawCentredText(window, font, p1Name.empty() ? "Nhap ten P1..." : p1Name, 26, typeP1 ? Cyber::White : Cyber::Gray, P1_X + NAME_W / 2.f, NAME_Y + 30.f);

        bool typeP2 = (typingState == 2);
        DrawNeonRect(window, P2_X, NAME_Y, NAME_W, NAME_H, typeP2 ? sf::Color(70, 20, 50) : sf::Color::Black, Cyber::Magenta, typeP2 ? 3.f : 1.f);
        DrawCentredText(window, font, "PLAYER 2", 20, Cyber::Magenta, P2_X + NAME_W / 2.f, NAME_Y - 15.f);
        DrawCentredText(window, font, p2Name.empty() ? "Nhap ten P2..." : p2Name, 26, typeP2 ? Cyber::White : Cyber::Gray, P2_X + NAME_W / 2.f, NAME_Y + 30.f);
    }

    // ── NÚT BẮT ĐẦU ──
    float btnX = W / 2.f - 150.f, btnY = 640.f;
    bool startHov = (mp.x >= btnX && mp.x <= btnX + 300.f && mp.y >= btnY && mp.y <= btnY + 70.f);
    sf::Color startCol = (selectionStep == 4) ? Cyber::Yellow : Cyber::Gray;
    DrawNeonRect(window, btnX, btnY, 300.f, 70.f, startHov && (selectionStep == 4) ? sf::Color(80, 70, 20) : Cyber::BgBtn, startCol, startHov && (selectionStep == 4) ? 3.f : 1.f);
    DrawCentredText(window, font, "BAT DAU GAME", 28, startCol, W / 2.f, btnY + 35.f);
}
void DrawAbout(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);
    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 18));

    // 1. Tiêu đề chính
    DrawCentredText(window, font, "ABOUT US", 60, Cyber::Yellow, W / 2.f, 80.f);

    // 2. Khung nội dung chính
    float bW = 850.f, bH = 520.f;
    float bX = (W - bW) / 2.f;
    float bY = 160.f;

    DrawNeonRect(window, bX, bY, bW, bH, Cyber::BgPanel, Cyber::Cyan, 1.5f);
    DrawCornerBrackets(window, bX, bY, bW, bH, Cyber::Cyan, 20.f, 2.f);

    // 3. Nội dung
    float startX = bX + 60.f;
    float currY = bY + 45.f;
    float lineGap = 38.f;

    // --- Dòng PROJECT: CARO MASTER căn giữa ---
    DrawCentredText(window, font, "PROJECT: CARO MASTER", 26, Cyber::White, W / 2.f, currY + 10.f);
    currY += lineGap + 15.f; // Nhích xuống để phần danh sách bên dưới thoáng hơn

    // Lambda vẽ các dòng căn lề trái
    auto DrawRow = [&](const std::string& text, sf::Color color, bool bold = false) {
        sf::Text t(text, font, 22);
        t.setFillColor(color);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setPosition(startX, currY);
        window.draw(t);
        currY += lineGap;
        };

    DrawRow("Class: 25CTT7 - HCMUS", Cyber::White);
    DrawRow("Instructor: Mr. Truong Toan Thinh", Cyber::White);

    currY += 10.f;
    DrawRow("---------------- DEVELOPMENT TEAM ----------------", Cyber::CyanDim);
    currY += 15.f;

    // Danh sách thành viên (Căn lề trái)
    DrawRow("24120164 - Nguyen The Anh", Cyber::White);
    DrawRow("24120115 - Le Tan Phat", Cyber::White);
    DrawRow("24120479 - Vu Duc Trung", Cyber::White);
    DrawRow("24120311 - Nguyen Mai Tung Hieu", Cyber::White);
    DrawRow("24120180 - Nguyen Dai Hieu", Cyber::White);

    // 4. Nút QUAY LAI nằm DƯỚI khung
    const float BW = 250.f, BH = 55.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = bY + bH + 30.f;

    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2i mp(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));
    bool hov = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);

    DrawNeonRect(window, BX, BY, BW, BH, hov ? sf::Color(30, 40, 70) : Cyber::BgBtn, hov ? Cyber::Magenta : Cyber::Grid, 2.f);
    DrawCentredText(window, font, "QUAY LAI", 22, hov ? Cyber::White : Cyber::Gray, BX + BW / 2.f, BY + BH / 2.f);
}