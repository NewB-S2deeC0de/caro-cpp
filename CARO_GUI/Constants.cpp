#include "Constants.h"

// ============================================================
//  GetDynCellSize – Tính kích thước ô ĐỘNG theo boardSize
//
//  Đảm bảo bàn cờ KHÔNG BAO GIỜ đè lên panel bên phải,
//  đồng thời không vượt quá chiều cao cửa sổ.
//
//  Không gian dành cho bàn cờ:
//    Ngang: WIN_WIDTH - OFFSET_X - PANEL_W - PANEL_GAP - margin_phải(10)
//    Dọc  : WIN_HEIGHT - 2 * OFFSET_Y
//
//  Ô = min(CELL_SIZE, avail_W / boardSize, avail_H / boardSize)
// ============================================================
int GetDynCellSize(int boardSize)
{
    if (boardSize <= 0)
    {
        return Config::CELL_SIZE;
    }

    // Không gian thực sự dành cho bàn cờ
    const int availW = Config::WIN_WIDTH - Config::OFFSET_X
        - Config::PANEL_W - Config::PANEL_GAP - 10;
    const int availH = Config::WIN_HEIGHT - 2 * Config::OFFSET_Y;

    int cellW = availW / boardSize;
    int cellH = availH / boardSize;
    int dynCell = std::min({ Config::CELL_SIZE, cellW, cellH });

    return (dynCell < 4) ? 4 : dynCell; // tối thiểu 4px, không để quá nhỏ
}

// ============================================================
//  Định nghĩa màu sắc – ch? ???c khởi tạo DUY NHẤT ở đây.
//  Tất cả các file khác đều trỏ về biến này qua từ khóa extern.
// ============================================================

const sf::Color BG_COLOR(25, 25, 30);               // Nền tối Dark Mode
const sf::Color GRID_COLOR(70, 70, 80);             // Lưới xám tro
const sf::Color COLOR_X(255, 50, 50);               // Đỏ Neon – quân X
const sf::Color COLOR_O(50, 255, 150);              // Xanh l� Neon � qu�n O
const sf::Color HOVER_COLOR(255, 255, 255, 30);     // Sáng mờ khi hover (alpha = 30)
const sf::Color WIN_LINE_COLOR(255, 215, 0);        // Vàng n?i b?t – đường thẳng
const sf::Color BTN_COLOR(40, 40, 50);              // Màu nền nút bấm