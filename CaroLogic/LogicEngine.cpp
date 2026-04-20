#include "LogicEngine.h"

// Tham chiếu đến biến toàn cục trong CaroAPI.cpp
extern int g_board[30][30];
extern int g_boardSize;
extern bool g_ruleBlock2;
extern int g_winStartX, g_winStartY, g_winEndX, g_winEndY;

extern int g_winStartX;
extern int g_winStartY; 
extern int g_winEndX;
extern int g_winEndY;

int CheckWinCondition(int x, int y, int player) {
    const int dx[] = { 1, 0, 1, 1 };
    const int dy[] = { 0, 1, 1, -1 };

    for (int dir = 0; dir < 4; ++dir) {
        int count = 1;
        int blocks = 0;

        int sx = x;
        int sy = y;
        int ex = x;
        int ey = y;

        // Duyệt tới
        int i = 1;
        while (x + i * dx[dir] >= 0 && x + i * dx[dir] < g_boardSize && y + i * dy[dir] >= 0 && y + i * dy[dir] < g_boardSize) {
            if (g_board[x + i * dx[dir]][y + i * dy[dir]] == player) {
                count++; ex = x + i * dx[dir]; ey = y + i * dy[dir];
            }
            else {
                if (g_board[x + i * dx[dir]][y + i * dy[dir]] != 0) blocks++;
                break;
            }
            i++;
        }

        // Duyệt lùi
        i = 1;
        while (x - i * dx[dir] >= 0 && x - i * dx[dir] < g_boardSize && y - i * dy[dir] >= 0 && y - i * dy[dir] < g_boardSize) {
            if (g_board[x - i * dx[dir]][y - i * dy[dir]] == player) {
                count++; sx = x - i * dx[dir]; sy = y - i * dy[dir];
            }
            else {
                if (g_board[x - i * dx[dir]][y - i * dy[dir]] != 0) blocks++;
                break;
            }
            i++;
        }

        if (count >= 5) {
            if (g_ruleBlock2 && blocks == 2 && count == 5)
            {
                continue;
            }
            g_winStartX = sx; 
            g_winStartY = sy;
            g_winEndX = ex; 
            g_winEndY = ey;
            return player; // 1 hoặc 2
        }
    }

    // Check Hòa
    for (int r = 0; r < g_boardSize; ++r)
    {
        for (int c = 0; c < g_boardSize; ++c)
        {
            if (g_board[r][c] == 0) return 0; // Đang chơi
        }
    }

    return 3; // Hòa
}