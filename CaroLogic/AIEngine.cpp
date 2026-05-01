#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define EMPTY 0
#define BOT 1
#define PLAYER 2

// Mảng điểm Heuristic tĩnh (Dùng cho Level 4, 5, 6)
// Điểm tăng theo cấp số nhân để ưu tiên chuỗi dài
const long AttackScores[7] = { 0, 9, 54, 162, 1458, 13122, 118098 };
const long DefendScores[7] = { 0, 3, 27, 99, 729, 6561, 59049 };

// Struct đơn giản để tiện trả về cùng lúc x, y
struct Move {
    int x;
    int y;
};

// Hàm kiểm tra tọa độ hợp lệ
bool IsValid(int x, int y, int boardSize) {
    return (x >= 0 && x < boardSize && y >= 0 && y < boardSize);
}

// Hàm kiểm tra xem ô này có nằm gần các quân cờ khác không (Bán kính 2 ô)
bool HasNeighbor(const int board[30][30], int boardSize, int x, int y) {
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            if (i == 0 && j == 0) continue;
            if (IsValid(x + i, y + j, boardSize) && board[x + i][y + j] != EMPTY) {
                return true;
            }
        }
    }
    return false;
}

// Khai báo trước hàm tính điểm Heuristic (Cực kỳ quan trọng)
long EvaluateCell(const int board[30][30], int boardSize, int x, int y);

// ==========================================
// LEVEL 1: Đánh ngẫu nhiên
// ==========================================
void Level1_Random(const int board[30][30], int boardSize, int* outX, int* outY) {
    int emptyX[900], emptyY[900]; // Mảng lưu các ô trống (30x30 = 900 ô)
    int count = 0;

    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            if (board[i][j] == EMPTY) {
                emptyX[count] = i;
                emptyY[count] = j;
                count++;
            }
        }
    }

    if (count > 0) {
        int r = rand() % count;
        *outX = emptyX[r];
        *outY = emptyY[r];
    }
}

// ==========================================
// LEVEL 2: Đánh ngẫu nhiên có khoanh vùng
// ==========================================
void Level2_ProximityRandom(const int board[30][30], int boardSize, int* outX, int* outY) {
    int validX[900], validY[900];
    int count = 0;

    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            // Chỉ thêm vào danh sách nếu là ô trống VÀ có quân cờ lân cận
            if (board[i][j] == EMPTY && HasNeighbor(board, boardSize, i, j)) {
                validX[count] = i;
                validY[count] = j;
                count++;
            }
        }
    }

    // Nếu bàn cờ trống trơn, đánh vào giữa
    if (count == 0) {
        *outX = boardSize / 2;
        *outY = boardSize / 2;
    }
    else {
        int r = rand() % count;
        *outX = validX[r];
        *outY = validY[r];
    }
}

// ==========================================
// LEVEL 3 & 4: Tính điểm Heuristic Cục bộ
// ==========================================
// L3: Chỉ quan tâm điểm lớn nhất (chặn/thắng)
// L4: Quét toàn bàn cờ một cách cẩn thận
void Level4_Heuristic(const int board[30][30], int boardSize, int* outX, int* outY) {
    long maxScore = -1;

    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            if (board[i][j] == EMPTY && HasNeighbor(board, boardSize, i, j)) {
                long currentScore = EvaluateCell(board, boardSize, i, j);
                if (currentScore > maxScore) {
                    maxScore = currentScore;
                    *outX = i;
                    *outY = j;
                }
            }
        }
    }

    // Fallback nếu bàn cờ trống
    if (maxScore == -1) {
        *outX = boardSize / 2;
        *outY = boardSize / 2;
    }
}

// ==========================================
// LEVEL 5 & 6: Minimax Đệ quy
// ==========================================
// Hàm Minimax thuần đệ quy, không lưu state
long Minimax(int board[30][30], int boardSize, int depth, int alpha, int beta, bool isBotTurn) {
    // 1. Kiểm tra điều kiện dừng (Thắng/Thua/Hòa hoặc hết depth)
    // 2. Tìm các nước đi tiềm năng (Top N nước đi điểm cao nhất)
    // 3. Vòng lặp thử từng nước đi:
    //      board[x][y] = isBotTurn ? BOT : PLAYER;
    //      score = Minimax(board, boardSize, depth - 1, alpha, beta, !isBotTurn);
    //      board[x][y] = EMPTY; // Hoàn tác nước đi (Backtracking)
    //      Cắt tỉa Alpha-Beta...
    // Return score;
    return 0; // Trả về tạm
}

void Level6_DeepMinimax(const int board[30][30], int boardSize, int* outX, int* outY) {
    // Tạo bản sao của bàn cờ để Minimax chạy giả lập (vì const board không cho phép sửa)
    int tempBoard[30][30];
    for (int i = 0; i < 30; i++)
        for (int j = 0; j < 30; j++)
            tempBoard[i][j] = board[i][j];

    // Gọi hàm Minimax... 
    // (Lưu ý: Đối với 30x30, phải kết hợp Heuristic để lọc nước đi trước khi gọi Minimax)
}

void CalculateBestMove(const int board[30][30], int boardSize, int level, int* outX, int* outY) {
    static bool seedInitialized = false;
    if (!seedInitialized) {
        srand(time(NULL));
        seedInitialized = true;
    }

    *outX = boardSize / 2; // Default
    *outY = boardSize / 2;

    if (level == 1) {
        Level1_Random(board, boardSize, outX, outY);
    }
    else if (level == 2) {
        Level2_ProximityRandom(board, boardSize, outX, outY);
    }
    else if (level == 3) {
        // Level 3 dùng chung hàm với Level 4 nhưng bộ điểm Heuristic bị tinh giản
        // (Hoặc đơn giản là để nó chạy Heuristic nhưng thêm yếu tố random để dễ hơn L4)
        Level4_Heuristic(board, boardSize, outX, outY);
    }
    else if (level == 4) {
        Level4_Heuristic(board, boardSize, outX, outY);
    }
    else if (level == 5) {
        // Level 5: Minimax độ sâu 2
        Level6_DeepMinimax(board, boardSize, outX, outY); // Cần cấu hình depth = 2
    }
    else if (level == 6) {
        // Level 6: Minimax độ sâu 4+ kèm sắp xếp nước đi
        Level6_DeepMinimax(board, boardSize, outX, outY); // Cần cấu hình depth = 4
    }
}

// Hàm con: Tính điểm cho một hướng cụ thể
// dx, dy là vector hướng (ví dụ: dx=1, dy=0 là chiều ngang)
long EvaluateDirection(const int board[30][30], int boardSize, int x, int y, int dx, int dy, int player) {
    int consecutive = 0;
    int blocks = 0;

    // 1. Quét về phía chiều dương của hướng (dx, dy)
    int i = x + dx;
    int j = y + dy;
    while (IsValid(i, j, boardSize) && board[i][j] == player) {
        consecutive++;
        i += dx;
        j += dy;
    }
    // Nếu đụng biên hoặc đụng quân địch -> Bị chặn 1 đầu
    if (!IsValid(i, j, boardSize) || (board[i][j] != EMPTY && board[i][j] != player)) {
        blocks++;
    }

    // 2. Quét về phía chiều âm của hướng (-dx, -dy)
    i = x - dx;
    j = y - dy;
    while (IsValid(i, j, boardSize) && board[i][j] == player) {
        consecutive++;
        i -= dx;
        j -= dy;
    }
    // Nếu đụng biên hoặc đụng quân địch -> Bị chặn đầu còn lại
    if (!IsValid(i, j, boardSize) || (board[i][j] != EMPTY && board[i][j] != player)) {
        blocks++;
    }

    // Luật cờ Caro cơ bản: Bị chặn 2 đầu thì chuỗi này coi như vứt (không tạo thành 5 được)
    // Ngoại trừ trường hợp đã có đủ 5 quân thì vẫn thắng
    if (blocks == 2 && consecutive < 4) {
        return 0;
    }

    // Giới hạn để không bị out of bound mảng điểm (tối đa mảng có 7 phần tử)
    int piecesToScore = consecutive + 1; // +1 vì tính cả quân cờ nếu được đặt tại (x,y)
    if (piecesToScore > 6) piecesToScore = 6;

    // Trả về điểm tương ứng từ mảng Heuristic tĩnh
    if (player == BOT) {
        return AttackScores[piecesToScore];
    }
    else {
        return DefendScores[piecesToScore];
    }
}

// Hàm chính: Tính tổng điểm Tấn công + Phòng thủ tại ô (x, y)
long EvaluateCell(const int board[30][30], int boardSize, int x, int y) {
    long totalScore = 0;

    // Mảng định nghĩa 4 hướng cần quét (chỉ cần lấy 1 nửa vòng tròn vì hàm quét cả âm/dương)
    // [0]: Ngang (1, 0)
    // [1]: Dọc (0, 1)
    // [2]: Chéo chính (1, 1)
    // [3]: Chéo phụ (1, -1)
    int dirX[] = { 1, 0, 1, 1 };
    int dirY[] = { 0, 1, 1, -1 };

    // Cộng dồn điểm đánh giá từ 4 hướng
    for (int d = 0; d < 4; d++) {
        // Điểm nếu Bot đánh vào ô này (Tấn công)
        long attackScore = EvaluateDirection(board, boardSize, x, y, dirX[d], dirY[d], BOT);

        // Điểm nếu Người đánh vào ô này (Phòng thủ / Chặn địch)
        long defendScore = EvaluateDirection(board, boardSize, x, y, dirX[d], dirY[d], PLAYER);

        totalScore += (attackScore + defendScore);
    }

    return totalScore;
}