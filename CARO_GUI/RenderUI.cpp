#include "RenderUI.h"
#include "Constants.h"
#include "CaroAPI.h"
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

namespace Cyber {
    // Primary accent – electric cyan
    const sf::Color Cyan{ 0,  255, 255, 255 };
    const sf::Color CyanDim{ 0,  180, 200, 180 };
    const sf::Color CyanGlow{ 0,  255, 255,  40 };

    // Secondary accent – hot magenta
    const sf::Color Magenta{ 255,  0, 200, 255 };
    const sf::Color MagDim{ 200,  0, 160, 180 };

    // Warning / win – electric yellow
    const sf::Color Yellow{ 255, 220,  0, 255 };
    const sf::Color YellowD{ 200, 170,  0, 200 };

    // Danger / X-piece – neon red-orange
    const sf::Color NeonRed{ 255,  50,  80, 255 };

    // Background layers
    const sf::Color BgDeep{ 8,   8,  14, 255 };
    const sf::Color BgPanel{ 12,  16,  28, 230 };
    const sf::Color BgBtn{ 18,  22,  38, 255 };
    const sf::Color BgHover{ 30,  40,  70, 255 };

    // Grid / border
    const sf::Color Grid{ 40,  55,  80, 255 };
    const sf::Color GridDim{ 25,  35,  55, 255 };
    const sf::Color White{ 220, 230, 255, 255 };
    const sf::Color Gray{ 100, 115, 145, 255 };
}

// ============================================================
//  Helper Functions
// ============================================================
static float PanelX(int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float boardRight = static_cast<float>(Config::OFFSET_X + boardSize * cellSz);
    float gapWidth = static_cast<float>(Config::WIN_WIDTH) - boardRight;
    return boardRight + (gapWidth - static_cast<float>(Config::PANEL_W)) / 2.0f;
}

// ── Corner bracket decoration ─────
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

// ── Neon-bordered rectangle ──────────────────────────────────
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

// ── Centred text helper ──────────────────────────────────────
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

// ── Scanline overlay ─────────────────────────────────────────
static void DrawScanlines(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color lineCol)
{
    for (float ly = y; ly < y + h; ly += 4.f) {
        sf::RectangleShape sl({ w, 1.f });
        sl.setPosition(x, ly);
        sl.setFillColor(lineCol);
        window.draw(sl);
    }
}

// ── Glitch accent stripe ──────────
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

// ── Section header with horizontal rule ─────────────────────
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

// ============================================================
//  DrawMenu (Tọa độ của RenderUI_final, đồ họa Cyberpunk)
// ============================================================
void DrawMenu(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    // ── Background grid & scanlines ──
    for (float gx = 0; gx < W; gx += 60.f) {
        sf::RectangleShape vl({ 1.f, H }); vl.setPosition(gx, 0); vl.setFillColor(sf::Color(0, 255, 255, 8)); window.draw(vl);
    }
    for (float gy = 0; gy < H; gy += 60.f) {
        sf::RectangleShape hl({ W, 1.f }); hl.setPosition(0, gy); hl.setFillColor(sf::Color(0, 255, 255, 8)); window.draw(hl);
    }
    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 18));

    // ── Title ──
    sf::Text shadow("CARO MASTER", font, 72);
    shadow.setFillColor(sf::Color(0, 255, 255, 30));
    sf::FloatRect sr = shadow.getLocalBounds();
    shadow.setOrigin(sr.left + sr.width / 2.0f, sr.top + sr.height / 2.0f);
    shadow.setPosition(W / 2.0f + 3.f, 150.0f + 3.f);
    window.draw(shadow);

    sf::Text title("CARO MASTER", font, 70);
    title.setFillColor(Cyber::Cyan);
    title.setStyle(sf::Text::Bold);
    sf::FloatRect r = title.getLocalBounds();
    title.setOrigin(r.left + r.width / 2.0f, r.top + r.height / 2.0f);
    title.setPosition(W / 2.0f, 150.0f);
    window.draw(title);

    // ── Menu Buttons (Giữ nguyên kích thước/tọa độ final) ──
    const char* menuItems[] = {
        "PVP - 2 Nguoi Choi",
        "PVE - Choi voi May",
        "Cai Dat",
        "Tai Game (Load)",
        "Thong Tin (About)",
        "Thoat"
    };
    const sf::Color btnBorder[] = { Cyber::Cyan, Cyber::Magenta, Cyber::CyanDim, Cyber::CyanDim, Cyber::Yellow, sf::Color(180, 30, 60, 200) };

    const float BTN_W = 350.0f;
    const float BTN_H = 60.0f;
    const float START_Y = 300.0f;
    sf::Vector2i mp = sf::Mouse::getPosition(window);

    for (int i = 0; i < 6; ++i) {
        float bX = W / 2.0f - BTN_W / 2.0f;
        float bY = START_Y + i * 80.0f;
        bool hov = (mp.x >= bX && mp.x <= bX + BTN_W && mp.y >= bY && mp.y <= bY + BTN_H);

        DrawNeonRect(window, bX, bY, BTN_W, BTN_H,
            hov ? sf::Color(20, 40, 70, 230) : Cyber::BgBtn,
            hov ? btnBorder[i] : sf::Color(btnBorder[i].r, btnBorder[i].g, btnBorder[i].b, 80),
            hov ? 2.f : 1.f);

        if (hov) DrawCornerBrackets(window, bX, bY, BTN_W, BTN_H, btnBorder[i], 10.f, 2.f);

        sf::Text txt(menuItems[i], font, 24);
        txt.setFillColor(hov ? Cyber::White : sf::Color(160, 175, 200));
        sf::FloatRect tr = txt.getLocalBounds();
        txt.setOrigin(tr.left + tr.width / 2.f, tr.top + tr.height / 2.f);
        txt.setPosition(bX + BTN_W / 2.f, bY + BTN_H / 2.f);
        window.draw(txt);
    }
}

// ============================================================
//  DrawInGamePanel
// ============================================================
void DrawInGamePanel(sf::RenderWindow& window, const sf::Font& font, float timeRemaining, bool isPlayerTurn, int gameStatus, int boardSize, GameMode gameMode, int undoLeft[2], float saveNotifTimer)
{
    const float pX = PanelX(boardSize);
    const float BTN_W = static_cast<float>(Config::PANEL_W);
    const float BTN_H = 52.0f;

    int cellSz = GetDynCellSize(boardSize);
    float boardRight = static_cast<float>(Config::OFFSET_X + boardSize * cellSz);

    // ── Panel Background ──
    float pY = static_cast<float>(Config::OFFSET_Y);
    float pH = static_cast<float>(Config::WIN_HEIGHT - 2 * Config::OFFSET_Y);
    DrawNeonRect(window, pX - 12.f, pY, BTN_W + 24.f, pH, Cyber::BgPanel, Cyber::Grid, 1.f);
    DrawCornerBrackets(window, pX - 12.f, pY, BTN_W + 24.f, pH, Cyber::CyanDim, 16.f, 1.5f);
    DrawScanlines(window, pX - 12.f, pY, BTN_W + 24.f, pH, sf::Color(0, 0, 0, 12));

    sf::RectangleShape divider(sf::Vector2f(2.f, pH));
    divider.setPosition(boardRight + Config::PANEL_GAP / 2.f, pY);
    divider.setFillColor(Cyber::Grid);
    window.draw(divider);

    // ── Turn Label ──
    DrawSectionHeader(window, font, "// LUOT DI", pX, 60.f, BTN_W, Cyber::Gray);

    std::string turnStr;
    sf::Color turnColor;
    if (gameMode == GameMode::PVP) {
        turnStr = isPlayerTurn ? "NGUOI 1 (X)" : "NGUOI 2 (O)";
        turnColor = isPlayerTurn ? Cyber::Cyan : Cyber::Magenta;
    }
    else {
        turnStr = isPlayerTurn ? "NGUOI (X)" : "MAY (O)";
        turnColor = isPlayerTurn ? Cyber::Cyan : Cyber::Magenta;
    }

    DrawNeonRect(window, pX, 85.f - 10.f, BTN_W, 44.f, sf::Color(turnColor.r, turnColor.g, turnColor.b, 18), sf::Color(turnColor.r, turnColor.g, turnColor.b, 120), 1.5f);
    DrawGlitchStripes(window, pX, 85.f - 10.f, BTN_W, 44.f, sf::Color(turnColor.r, turnColor.g, turnColor.b, 35));

    sf::Text turnText(turnStr, font, 24);
    turnText.setFillColor(turnColor);
    turnText.setStyle(sf::Text::Bold);
    turnText.setOrigin(turnText.getLocalBounds().left + turnText.getLocalBounds().width / 2.f, turnText.getLocalBounds().top + turnText.getLocalBounds().height / 2.f);
    turnText.setPosition(pX + BTN_W / 2.f, 85.f + 12.f);
    window.draw(turnText);

    // ── Timer ──
    DrawSectionHeader(window, font, "// THOI GIAN CON LAI", pX, 135.f, BTN_W, Cyber::Gray);
    sf::Color timerColor = (timeRemaining > 20.f) ? Cyber::Cyan : (timeRemaining > 10.f) ? Cyber::Yellow : Cyber::NeonRed;

    sf::Text timerText(std::to_string((int)timeRemaining) + " giay", font, 34);
    timerText.setFillColor(timerColor);
    timerText.setStyle(sf::Text::Bold);
    timerText.setPosition(pX, 158.f);
    window.draw(timerText);

    float ratio = std::max(0.f, std::min(1.f, timeRemaining / 60.f));
    DrawNeonRect(window, pX, 200.f, BTN_W, 8.f, sf::Color(15, 20, 35), Cyber::Grid, 1.f);
    if (ratio > 0.f) {
        DrawNeonRect(window, pX, 200.f, BTN_W * ratio, 8.f, sf::Color(timerColor.r, timerColor.g, timerColor.b, 200), sf::Color::Transparent, 0);
        DrawNeonRect(window, pX + BTN_W * ratio - 3.f, 200.f, 3.f, 8.f, timerColor, sf::Color::Transparent, 0);
    }

    // ── Status/Hints ──
    if (gameStatus != 0) {
        DrawNeonRect(window, pX, 220.f, BTN_W, 60.f, sf::Color(40, 40, 10, 200), Cyber::Yellow, 2.f);
        DrawCornerBrackets(window, pX, 220.f, BTN_W, 60.f, Cyber::Yellow, 10.f, 2.f);

        std::string s;
        if (gameMode == GameMode::PVP) s = (gameStatus == 1) ? "NGUOI 1 THANG!" : ((gameStatus == 2) ? "NGUOI 2 THANG!" : "HOA!");
        else s = (gameStatus == 1) ? "NGUOI THANG!" : ((gameStatus == 2) ? "MAY THANG!" : "HOA!");

        DrawCentredText(window, font, s, 26, Cyber::Yellow, pX + BTN_W / 2.f, 250.f);
    }
    else {
        std::string hintStr;
        if (gameMode == GameMode::PVP) hintStr = isPlayerTurn ? ">> Nguoi 1 - Bam ban co" : ">> Nguoi 2 - Bam ban co";
        else hintStr = isPlayerTurn ? ">> Click vao ban co de di" : ">> Dang doi AI suy nghi...";

        sf::Text hint(hintStr, font, 14);
        hint.setFillColor(Cyber::CyanDim);
        hint.setPosition(pX, 220.f);
        window.draw(hint);
    }

    // ── Undo ──
    if (gameMode == GameMode::PVP) {
        DrawSectionHeader(window, font, "// LUOT UNDO CON LAI", pX, 295.f, BTN_W, Cyber::Gray);

        struct PUI { std::string label; int left; sf::Color color; float posY; } info[2] = {
            { "Nguoi 1 (X):", undoLeft[0], Cyber::Cyan, 320.f },
            { "Nguoi 2 (O):", undoLeft[1], Cyber::Magenta, 350.f }
        };

        for (int p = 0; p < 2; ++p) {
            sf::Text lblTxt(info[p].label, font, 18);
            lblTxt.setFillColor(info[p].color);
            lblTxt.setPosition(pX, info[p].posY);
            window.draw(lblTxt);

            float startX = pX + 130.f;
            for (int k = 0; k < Config::UNDO_MAX; ++k) {
                sf::CircleShape dot(7.f);
                dot.setOrigin(7.f, 7.f);
                dot.setPosition(startX + k * 22.f + 7.f, info[p].posY + 8.f);
                if (k < info[p].left) {
                    dot.setFillColor(info[p].color);
                    dot.setOutlineThickness(0);
                }
                else {
                    dot.setFillColor(sf::Color(20, 25, 40));
                    dot.setOutlineThickness(1.5f);
                    dot.setOutlineColor(sf::Color(50, 60, 90));
                }
                window.draw(dot);
            }
        }
        sf::RectangleShape sep({ BTN_W, 1.f });
        sep.setPosition(pX, 382.f); sep.setFillColor(sf::Color(50, 60, 90)); window.draw(sep);
    }

    // ── Buttons ──
    const float BTN_START_Y = (gameMode == GameMode::PVP) ? 395.f : 420.f;
    const char* gameBtns[] = { "Undo", "Save Game", "Main Menu" };
    const sf::Color btnAccent[] = { Cyber::Cyan, Cyber::CyanDim, Cyber::MagDim };
    sf::Vector2i mp = sf::Mouse::getPosition(window);

    for (int i = 0; i < 3; ++i) {
        float bX = pX;
        float bY = BTN_START_Y + i * 80.f;
        bool hov = (mp.x >= bX && mp.x <= bX + BTN_W && mp.y >= bY && mp.y <= bY + BTN_H);

        DrawNeonRect(window, bX, bY, BTN_W, BTN_H, hov ? sf::Color(20, 35, 65, 240) : Cyber::BgBtn, hov ? btnAccent[i] : sf::Color(btnAccent[i].r, btnAccent[i].g, btnAccent[i].b, 70), hov ? 2.f : 1.f);
        if (hov) DrawCornerBrackets(window, bX, bY, BTN_W, BTN_H, btnAccent[i], 8.f, 1.5f);

        DrawCentredText(window, font, gameBtns[i], 22, hov ? Cyber::White : sf::Color(160, 175, 200), bX + BTN_W / 2.f, bY + BTN_H / 2.f);
    }

    // ── Notification ──
    if (saveNotifTimer > 0.f) {
        int alpha = saveNotifTimer < 0.5f ? static_cast<int>((saveNotifTimer / 0.5f) * 255.f) : 255;
        float bCx = static_cast<float>(Config::OFFSET_X) + (boardSize * cellSz) / 2.0f;
        float bCy = static_cast<float>(Config::OFFSET_Y) + (boardSize * cellSz) / 2.0f;

        DrawNeonRect(window, bCx - 200.f, bCy - 40.f, 400.f, 80.f, sf::Color(10, 10, 10, alpha), sf::Color(50, 255, 50, alpha), 3.f);

        sf::Text notifTxt("DA LUU VAN GAME!", font, 36);
        notifTxt.setFillColor(sf::Color(255, 255, 255, alpha));
        notifTxt.setStyle(sf::Text::Bold);
        sf::FloatRect nr = notifTxt.getLocalBounds();
        notifTxt.setOrigin(nr.left + nr.width / 2.f, nr.top + nr.height / 2.f);
        notifTxt.setPosition(bCx, bCy);
        window.draw(notifTxt);
    }
}

// ============================================================
//  DrawBoard
// ============================================================
void DrawBoard(sf::RenderWindow& window, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    float ox = static_cast<float>(Config::OFFSET_X);
    float oy = static_cast<float>(Config::OFFSET_Y);
    float bW = static_cast<float>(boardSize * cellSz);
    float bH = static_cast<float>(boardSize * cellSz);

    DrawNeonRect(window, ox - 2.f, oy - 2.f, bW + 4.f, bH + 4.f, sf::Color(6, 8, 16), Cyber::Grid, 1.f);
    DrawScanlines(window, ox, oy, bW, bH, sf::Color(0, 0, 0, 10));

    for (int i = 0; i <= boardSize; ++i) {
        bool edge = (i == 0 || i == boardSize);
        sf::Color lc = edge ? Cyber::Grid : Cyber::GridDim;
        float lw = edge ? 1.5f : 0.5f;

        sf::RectangleShape h(sf::Vector2f(bW, lw));
        h.setPosition(ox, oy + i * cellSz - lw / 2.f);
        h.setFillColor(lc);
        window.draw(h);

        sf::RectangleShape v(sf::Vector2f(lw, bH));
        v.setPosition(ox + i * cellSz - lw / 2.f, oy);
        v.setFillColor(lc);
        window.draw(v);
    }

    auto drawDot = [&](int gx, int gy, float r, sf::Color c) {
        sf::CircleShape dot(r); dot.setOrigin(r, r);
        dot.setPosition(ox + gx * cellSz + cellSz / 2.f, oy + gy * cellSz + cellSz / 2.f);
        dot.setFillColor(c); window.draw(dot);
        };

    int mid = boardSize / 2;
    drawDot(mid, mid, 4.f, Cyber::CyanDim);
    if (boardSize >= 15) {
        int q = boardSize / 4;
        for (int dx : {q, boardSize - 1 - q})
            for (int dy : {q, boardSize - 1 - q})
                drawDot(dx, dy, 3.f, Cyber::GridDim);
    }

    DrawCornerBrackets(window, ox - 2.f, oy - 2.f, bW + 4.f, bH + 4.f, Cyber::CyanDim, 18.f, 1.5f);
}

// ============================================================
//  DrawPieces
// ============================================================
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
            float arm = cellSz / 2.f - 5.f;

            if (cell == 1) { // X (Cyan)
                float br = arm + 3.f;
                DrawCornerBrackets(window, cx - br - 1.f, cy - br - 1.f, (br + 1.f) * 2.f, (br + 1.f) * 2.f, sf::Color(0, 200, 220, 80), 5.f, 1.f);
                for (int rot : {45, -45}) {
                    sf::RectangleShape ln({ arm * 2.f, 2.5f });
                    ln.setFillColor(Cyber::Cyan);
                    ln.setOrigin(arm, 1.25f);
                    ln.setPosition(cx, cy);
                    ln.setRotation(static_cast<float>(rot));
                    window.draw(ln);
                }
            }
            else if (cell == 2) { // O (Magenta)
                sf::CircleShape outer(arm + 2.f); outer.setOrigin(arm + 2.f, arm + 2.f); outer.setPosition(cx, cy);
                outer.setFillColor(sf::Color::Transparent); outer.setOutlineThickness(1.f); outer.setOutlineColor(sf::Color(200, 0, 160, 60));
                window.draw(outer);

                sf::CircleShape ring(arm); ring.setOrigin(arm, arm); ring.setPosition(cx, cy);
                ring.setFillColor(sf::Color::Transparent); ring.setOutlineThickness(2.5f); ring.setOutlineColor(Cyber::Magenta);
                window.draw(ring);

                sf::CircleShape dot(2.5f); dot.setOrigin(2.5f, 2.5f); dot.setPosition(cx, cy); dot.setFillColor(Cyber::Magenta);
                window.draw(dot);
            }
        }
    }
}

// ============================================================
//  DrawHoverEffect
// ============================================================
void DrawHoverEffect(sf::RenderWindow& window, int mouseX, int mouseY, int boardSize)
{
    int cellSz = GetDynCellSize(boardSize);
    int gX = (mouseX - Config::OFFSET_X) / cellSz;
    int gY = (mouseY - Config::OFFSET_Y) / cellSz;

    if (gX >= 0 && gX < boardSize && gY >= 0 && gY < boardSize && GetCell(gX, gY) == 0) {
        float rx = static_cast<float>(Config::OFFSET_X + gX * cellSz);
        float ry = static_cast<float>(Config::OFFSET_Y + gY * cellSz);

        sf::RectangleShape hr({ static_cast<float>(cellSz), static_cast<float>(cellSz) });
        hr.setPosition(rx, ry);
        hr.setFillColor(sf::Color(0, 200, 255, 22));
        window.draw(hr);

        DrawCornerBrackets(window, rx + 2.f, ry + 2.f, static_cast<float>(cellSz) - 4.f, static_cast<float>(cellSz) - 4.f, sf::Color(0, 255, 255, 180), 6.f, 1.5f);
    }
}

// ============================================================
//  DrawWinLine
// ============================================================
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

// ============================================================
//  DrawSettings
// ============================================================
void DrawSettings(sf::RenderWindow& window, const sf::Font& font, int boardSize, bool ruleBlock2, int aiLevel, float sfxVolume, bool bgmEnabled)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    // BG Grid
    for (float gx = 0; gx < W; gx += 60.f) { sf::RectangleShape vl({ 1.f, H }); vl.setPosition(gx, 0); vl.setFillColor(sf::Color(0, 255, 255, 6)); window.draw(vl); }
    for (float gy = 0; gy < H; gy += 60.f) { sf::RectangleShape hl({ W, 1.f }); hl.setPosition(0, gy); hl.setFillColor(sf::Color(0, 255, 255, 6)); window.draw(hl); }
    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 14));

    DrawCentredText(window, font, "// CAI DAT HE THONG", 54, Cyber::Yellow, W / 2.f, 80.f);

    const char* labels[] = { "Kich thuoc ban co (10-30):", "Luat chan 2 dau:", "Do kho AI (1-3):", "Am luong SFX (0-100):", "Nhac nen BGM:" };
    std::string values[] = { std::to_string(boardSize), ruleBlock2 ? "BAT (ON)" : "TAT (OFF)", std::to_string(aiLevel), std::to_string((int)sfxVolume), bgmEnabled ? "BAT (ON)" : "TAT (OFF)" };

    const float SY = 200.f, LX = 250.f, CX = 650.f, RG = 80.f;
    sf::Vector2i mp = sf::Mouse::getPosition(window);

    for (int i = 0; i < 5; ++i) {
        float cy = SY + i * RG;
        sf::Text lbl(labels[i], font, 28); lbl.setFillColor(Cyber::White); lbl.setPosition(LX, cy); window.draw(lbl);

        if (i == 0 || i == 2 || i == 3) { // [-] value [+]
            bool hovM = (mp.x >= CX && mp.x <= CX + 50 && mp.y >= cy && mp.y <= cy + 50);
            DrawNeonRect(window, CX, cy, 50, 50, hovM ? sf::Color(80, 20, 20) : Cyber::BgBtn, Cyber::NeonRed, 1.5f);
            DrawCentredText(window, font, "-", 40, Cyber::NeonRed, CX + 25.f, cy + 20.f);

            DrawCentredText(window, font, values[i], 28, Cyber::Cyan, CX + 125.f, cy + 25.f);

            float pX2 = CX + 200.f;
            bool hovP = (mp.x >= pX2 && mp.x <= pX2 + 50 && mp.y >= cy && mp.y <= cy + 50);
            DrawNeonRect(window, pX2, cy, 50, 50, hovP ? sf::Color(20, 60, 20) : Cyber::BgBtn, sf::Color(50, 200, 100), 1.5f);
            DrawCentredText(window, font, "+", 40, sf::Color(50, 200, 100), pX2 + 25.f, cy + 20.f);
        }
        else { // Toggle
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

// ============================================================
//  DrawLoadScreen
// ============================================================
void DrawLoadScreen(sf::RenderWindow& window, const sf::Font& font)
{
    DrawScanlines(window, 0, 0, Config::WIN_WIDTH, Config::WIN_HEIGHT, sf::Color(0, 0, 0, 14));
    sf::Text title("DANH SACH DIEM LUU", font, 45);
    title.setFillColor(Cyber::Yellow); title.setStyle(sf::Text::Bold); title.setPosition(80.f, 50.f); window.draw(title);

    sf::Vector2i mp = sf::Mouse::getPosition(window);
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
                    if (tempBoard[x][y] == 1) { // Cyber X mini
                        for (int rot : {45, -45}) {
                            sf::RectangleShape l({ mCellSz * 0.7f, thickness }); l.setOrigin(mCellSz * 0.7f / 2.f, thickness / 2.f); l.setPosition(cX, cY); l.setFillColor(Cyber::Cyan); l.rotate(rot); window.draw(l);
                        }
                    }
                    else if (tempBoard[x][y] == 2) { // Cyber O mini
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

// ============================================================
//  DrawSaveScreen
// ============================================================
void DrawSaveScreen(sf::RenderWindow& window, const sf::Font& font, bool isNaming, const std::string& inputName, sf::Clock& clock)
{
    DrawScanlines(window, 0, 0, Config::WIN_WIDTH, Config::WIN_HEIGHT, sf::Color(0, 0, 0, 14));
    sf::Text title("CHON O DE LUU VAN GAME", font, 45);
    title.setFillColor(Cyber::Yellow); title.setStyle(sf::Text::Bold); title.setPosition(80.f, 50.f); window.draw(title);

    sf::Vector2i mp = sf::Mouse::getPosition(window);
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
                    if (tempBoard[x][y] == 1) { // Cyber X
                        for (int rot : {45, -45}) { sf::RectangleShape l({ mCellSz * 0.7f, thickness }); l.setOrigin(mCellSz * 0.7f / 2.f, thickness / 2.f); l.setPosition(cX, cY); l.setFillColor(Cyber::Cyan); l.rotate(rot); window.draw(l); }
                    }
                    else if (tempBoard[x][y] == 2) { // Cyber O
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
        sf::RectangleShape overlay(sf::Vector2f(Config::WIN_WIDTH, Config::WIN_HEIGHT));
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

        // Logic con trỏ nhấp nháy từ bản final
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
    }
}

//
void DrawAbout(sf::RenderWindow& window, const sf::Font& font)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    DrawScanlines(window, 0, 0, W, H, sf::Color(0, 0, 0, 18));

    // 1. Tiêu đề chính nhích lên cao hơn
    DrawCentredText(window, font, "ABOUT US", 60, Cyber::Yellow, W / 2.f, 50.f);

    // 2. Khung nội dung chính - Thu gọn lại để thấy nút quay lại
    float bW = 850.f, bH = 620.f;
    float bX = (W - bW) / 2.f;
    float bY = 90.f;

    DrawNeonRect(window, bX, bY, bW, bH, Cyber::BgPanel, Cyber::Cyan, 1.5f);
    DrawCornerBrackets(window, bX, bY, bW, bH, Cyber::Cyan, 20.f, 2.f);

    // 3. Nội dung căn lề trái chuẩn
    float startX = bX + 60.f;
    float currY = bY + 30.f;
    float lineGap = 28.f; // Thu nhỏ lineGap để nội dung không quá dài

    auto DrawRow = [&](const std::string& text, sf::Color color, bool bold = false) {
        sf::Text t(text, font, 22);
        t.setFillColor(color);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setPosition(startX, currY);
        window.draw(t);
        currY += lineGap;
        };

    DrawRow("PROJECT: CARO MASTER - VERSION 1.0", Cyber::White, true);
    DrawRow("Class: 25CTT7 - HCMUS", Cyber::White);
    DrawRow("Instructor: Mr. Nguyen Thanh Tung", Cyber::White);

    DrawRow("---------------- DEVELOPMENT TEAM ----------------", Cyber::CyanDim);
    currY += 5.f;

    DrawRow("[ Fullscreen Mode ]", Cyber::Yellow);
    DrawRow("The Anh", Cyber::White);

    DrawRow("[ AI Algorithm ]", Cyber::Yellow);
    DrawRow("Tan Phat", Cyber::White);

    DrawRow("[ Undo System ]", Cyber::Yellow);
    DrawRow("Vu Duc Trung", Cyber::White);

    DrawRow("[ Save & Load System ]", Cyber::Yellow);
    DrawRow("Tung Hieu", Cyber::White);

    DrawRow("[ UI/UX & Graphics ]", Cyber::Yellow);
    DrawRow("Dai Hieu", Cyber::White);

    DrawRow("------------------ TECHNOLOGY ------------------", Cyber::CyanDim);
    DrawRow("C++, SFML Library, WinAPI", Cyber::Gray);

    // 4. Nút QUAY LAI nằm DƯỚI khung - Đảm bảo nằm trong màn hình
    const float BW = 250.f, BH = 55.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = bY + bH + 25.f; // Nút nằm tại y = 90 + 620 + 25 = 735 (vẫn < 850)

    sf::Vector2i mp = sf::Mouse::getPosition(window);
    bool hov = (mp.x >= BX && mp.x <= BX + BW && mp.y >= BY && mp.y <= BY + BH);

    DrawNeonRect(window, BX, BY, BW, BH, hov ? sf::Color(30, 40, 70) : Cyber::BgBtn, hov ? Cyber::Magenta : Cyber::Grid, 2.f);
    DrawCentredText(window, font, "QUAY LAI", 22, hov ? Cyber::White : Cyber::Gray, BX + BW / 2.f, BY + BH / 2.f);
}