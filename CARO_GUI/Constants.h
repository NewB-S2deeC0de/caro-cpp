#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm> // std::min

// ============================================================
//  Enum trạng thái luồng game
// ============================================================
enum class AppState { MENU_SCREEN, IN_GAME_SCREEN, SETTINGS_SCREEN, LOAD_SCREEN, SAVE_SCREEN };

// ============================================================
//  Enum chế độ chơi
//  PVP = 2 người chơi lần lượt xen kẽ (không có AI)
//  PVE = người vs máy
// ============================================================
enum class GameMode { PVP, PVE };

// ============================================================
//  POD struct – nút bấm thủ tục (Plain Old Data, không hàm)
// ============================================================
struct ButtonRect {
    // (x, y) vị trí góc trên trái
    float x;    
    float y;
    float width;
    float height;
};

// ============================================================
//  Cấu hình cửa sổ và bàn cờ
// ============================================================
struct Config {
    // Size window game = 1200x800
    static const int WIN_WIDTH = 1200;
    static const int WIN_HEIGHT = 800;

    static const int CELL_SIZE = 30;

    // Khoảng cách từ mép cửa sổ trò chơi tới bàn cờ 
    static const int OFFSET_X = 50;
    static const int OFFSET_Y = 50;

    // Panel bên phải, dành chỗ cho bảng thông tin, undo, restart...
    static const int PANEL_W = 260;     // Độ rộng panel
    static const int PANEL_GAP = 40;    // Khoảng cách giữa panel và bàn cờ 

    // Số lượt Undo tối đa mỗi người trong chế độ PVP
    static const int UNDO_MAX = 3;
};

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
int GetDynCellSize(int boardSize);

// ============================================================
//  Màu sắc – CHỈ KHAI BÁO (extern) ở đây.
//  Định nghĩa (khởi tạo) nằm trong Constants.cpp.
// ============================================================
extern const sf::Color BG_COLOR;        // Nền tối Dark Mode       (25, 25, 30)
extern const sf::Color GRID_COLOR;      // Lưới xám tro            (70, 70, 80)
extern const sf::Color COLOR_X;         // Đỏ Neon – quân X        (255, 50, 50)
extern const sf::Color COLOR_O;         // Xanh lá Neon – quân O   (50, 255, 150)
extern const sf::Color HOVER_COLOR;     // Sáng mờ khi hover       (255, 255, 255, 30)
extern const sf::Color WIN_LINE_COLOR;  // Vàng – đường thắng      (255, 215, 0)
extern const sf::Color BTN_COLOR;       // Nền nút bấm             (40, 40, 50)
