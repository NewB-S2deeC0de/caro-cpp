#include "AIEngine.h"
#include <vector>
#include <algorithm>

// Bảng điểm mới: Khoảng cách cực lớn để phân biệt rõ độ nguy hiểm
const long P_5 = 10000000; // 5 con
const long P_4_Open = 500000; // 4 con thoáng 2 đầu
const long P_4_Blocked = 10000; // 4 con bị chặn 1 đầu
const long P_3_Open = 5000;  // 3 con thoáng 2 đầu
const long P_3_Blocked = 500;

// ==========================================
// 1. HÀM ĐÁNH GIÁ MẪU HÌNH (PATTERN EVAL)
// ==========================================
static long EvaluatePattern(int count, int blocks, bool isAttack) {
    if (blocks == 2 && count < 5)
    {
        return 0;
    }
    if (count >= 5)
    {
        return P_5;
    }

    if (isAttack) // Nếu AI đang tấn công
    { 
        switch (count) 
        {
        case 4:
        {
            return (blocks == 0) ? P_4_Open : P_4_Blocked;
        }
        case 3:
        {
            return (blocks == 0) ? P_3_Open : P_3_Blocked;
        }
        case 2:
        {
            return (blocks == 0) ? 100 : 10;
        }
        default:
        {
            return 1;
        }
        }
    }
    else  // Nếu AI đang phòng ngự (chặn người chơi)
    {
        switch (count) 
        {
        case 4:
        {
            return (blocks == 0) ? P_4_Open * 1.5 : P_4_Blocked * 1.2; // Chặn 4 cực kỳ ưu tiên
        }
        case 3:
        {
            return (blocks == 0) ? P_3_Open * 1.2 : P_3_Blocked;
        }
        default:
        {
            return 2;
        }
        }
    }
}

static long QuickEval(const int board[30][30], int size, int x, int y, int pMode, int aiMode) {
    long total = 0;
    int dx[] = { 1, 0, 1, 1 };
    int dy[] = { 0, 1, 1, -1 };

    for (int d = 0; d < 4; d++) 
    {
        // Tính cho AI (Công)
        int count = 1;
        int blocks = 0;
        for (int i = 1; i <= 4; i++) 
        {
            int nx = x + i * dx[d];
            int ny = y + i * dy[d];

            if (nx < 0 || nx >= size || ny < 0 || ny >= size) 
            { 
                blocks++; 
                break; 
            }
            if (board[nx][ny] == aiMode)
            {
                count++;
            }
            else
            {
                if (board[nx][ny] == pMode)
                {
                    blocks++;
                    break;
                }
            }
        }

        for (int i = 1; i <= 4; i++) 
        {
            int nx = x - i * dx[d];
            int ny = y - i * dy[d];

            if (nx < 0 || nx >= size || ny < 0 || ny >= size) 
            {
                blocks++;
                break; 
            }
            if (board[nx][ny] == aiMode)
            {
                count++;
            }
            else
            {
                if (board[nx][ny] == pMode)
                {
                    blocks++;
                    break;
                }
            }
        }
        
        total += EvaluatePattern(count, blocks, true);

        // Tính cho Người chơi (Thủ)
        count = 1;
        blocks = 0;
        for (int i = 1; i <= 4; i++) 
        {
            int nx = x + i * dx[d];
            int ny = y + i * dy[d];

            if (nx < 0 || nx >= size || ny < 0 || ny >= size) 
            {
                blocks++;
                break; 
            }
            if (board[nx][ny] == pMode)
            {
                count++;
            }
            else
            {
                if (board[nx][ny] == aiMode)
                {
                    blocks++;
                    break;
                }
            }
        }

        for (int i = 1; i <= 4; i++) 
        {
            int nx = x - i * dx[d]; 
            int ny = y - i * dy[d];

            if (nx < 0 || nx >= size || ny < 0 || ny >= size)
            {
                blocks++; 
                break;
            }
            if (board[nx][ny] == pMode)
            {
                count++;
            }
            else
            {
                if (board[nx][ny] == aiMode)
                {
                    blocks++;
                    break;
                }
            }
        }

        total += EvaluatePattern(count, blocks, false);
    }

    return total;
}

// ==========================================
// 2. THIẾT KẾ 3 CẤP ĐỘ MỚI
// ==========================================

// LV 1: Đánh "Ngây thơ" - Chỉ biết nối dài quân mình, ít chặn
static void MoveLV1(const int board[30][30], int size, int pMode, int aiMode, int* outX, int* outY) {
    long maxS = -1;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (board[i][j] == 0)
            {
                long s = QuickEval(board, size, i, j, pMode, aiMode) / 10; // Giảm độ nhạy phòng thủ
                if (s > maxS)
                {
                    maxS = s;
                    *outX = i;
                    *outY = j;
                }
            }
        }
    }
}

// LV 2: Đánh "Phản xạ" - Thấy chỗ nào điểm cao nhất (công hoặc thủ) là đánh ngay
static void MoveLV2(int board[30][30], int size, int pMode, int aiMode, int* outX, int* outY) {
    long maxS = -1;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (board[i][j] == 0)
            {
                long s = QuickEval(board, size, i, j, pMode, aiMode);
                if (s > maxS)
                {
                    maxS = s;
                    *outX = i;
                    *outY = j;
                }
            }
        }
    }
}

// LV 3: Đánh "Chiến thuật" - Tìm nước đi tạo ra 2 hướng thắng (Double Threat)
static void MoveLV3(int board[30][30], int size, int pMode, int aiMode, int* outX, int* outY) {
    long maxS = -1;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (board[i][j] == 0)
            {
                long s = QuickEval(board, size, i, j, pMode, aiMode);

                // Đặc biệt: Nếu nước đi này tạo ra chuỗi 3 thoáng (Open Three) hoặc chuỗi 4
                // AI sẽ ưu tiên cực cao để tạo thế thắng kép
                if (board[i][j] == 0)
                {
                    // Giả lập đánh thử để xem có tạo ra "bẫy" không
                    board[i][j] = aiMode;
                    s += QuickEval(board, size, i, j, pMode, aiMode) * 0.5;
                    board[i][j] = 0;
                }

                if (s > maxS)
                {
                    maxS = s;
                    *outX = i;
                    *outY = j;
                }
            }
        }
    }
}

// ==========================================
// 3. ĐIỀU HƯỚNG TỔNG
// ==========================================
void CalculateBestMove(const int board[30][30], int boardSize, int level, int* outX, int* outY) {
    int pMode = 1; 
    int aiMode = 2;

    // Mặc định tâm bàn cờ nếu nước đầu
    *outX = boardSize / 2;
    *outY = boardSize / 2;

    int tempBoard[30][30]; 
    for (int i = 0; i < boardSize; i++) 
    {
        for (int j = 0; j < boardSize; j++)
        {
            tempBoard[i][j] = board[i][j];
        }
    }

    if (level == 1)
    {
        MoveLV1(board, boardSize, pMode, aiMode, outX, outY);
    }
    else if (level == 2)
    {
        MoveLV2(tempBoard, boardSize, pMode, aiMode, outX, outY);
    }
    else
    {
        MoveLV3(tempBoard, boardSize, pMode, aiMode, outX, outY);
    }
}