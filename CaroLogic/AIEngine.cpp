#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <climits>

#define EMPTY 0
#define BOT 1
#define PLAYER 2

// Struct lưu trữ nước đi và điểm số
struct MoveVal {
    int x;
    int y;
    long long score;
};

// Hàm sắp xếp nước đi theo điểm giảm dần
bool CompareMoves(const MoveVal& a, const MoveVal& b) {
    return a.score > b.score;
}

// Kiểm tra tọa độ hợp lệ
bool IsValid(int x, int y, int boardSize) {
    return (x >= 0 && x < boardSize && y >= 0 && y < boardSize);
}

// Kiểm tra xem ô có lân cận quân cờ nào không (để tối ưu không quét toàn bàn cờ trống)
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

// ==========================================
// HỆ THỐNG CHẤM ĐIỂM (HEURISTIC)
// ==========================================
// Điểm thưởng cho các chuỗi cờ (5: Thắng, 4 hở: Gần thắng,...)
long long GetPatternScore(int count, int blocks) {
    if (count >= 5) return 100000000;         // Thắng chắc
    if (blocks == 2) return 0;                

    if (count == 4) return (blocks == 0) ? 10000000 : 1000000; // 4 hở / 4 chặn 1 đầu
    if (count == 3) return (blocks == 0) ? 100000 : 10000;     // 3 hở / 3 chặn 1 đầu
    if (count == 2) return (blocks == 0) ? 1000 : 100;         // 2 hở / 2 chặn 1 đầu
    if (count == 1) return 10;
    return 0;
}

// Tính điểm của 1 ô cụ thể khi ĐẶT THỬ 1 quân cờ (Dùng để lọc nước đi)
long long EvaluateDirectionForMove(const int board[30][30], int boardSize, int x, int y, int dx, int dy, int player) {
    int count = 1;
    int blocks = 0;

    int nx = x + dx, ny = y + dy;
    while (IsValid(nx, ny, boardSize) && board[nx][ny] == player) { count++; nx += dx; ny += dy; }
    if (!IsValid(nx, ny, boardSize) || board[nx][ny] != EMPTY) blocks++;

    nx = x - dx; ny = y - dy;
    while (IsValid(nx, ny, boardSize) && board[nx][ny] == player) { count++; nx -= dx; ny -= dy; }
    if (!IsValid(nx, ny, boardSize) || board[nx][ny] != EMPTY) blocks++;

    return GetPatternScore(count, blocks);
}

// Tổng hợp điểm Công + Thủ của một ô (Heuristic cục bộ)
long long EvaluateCell(const int board[30][30], int boardSize, int x, int y) {
    long long score = 0;
    int dx[] = { 1, 0, 1, 1 };
    int dy[] = { 0, 1, 1, -1 };

    for (int d = 0; d < 4; d++) {
        long long attack = EvaluateDirectionForMove(board, boardSize, x, y, dx[d], dy[d], BOT);
        long long defend = EvaluateDirectionForMove(board, boardSize, x, y, dx[d], dy[d], PLAYER);
        // Cộng tổng: Vừa muốn tạo thế công cho mình, vừa muốn chặn thế công của địch
        score += attack + defend;
    }
    return score;
}

// ==========================================
// TẠO & SẮP XẾP NƯỚC ĐI (MOVE ORDERING)
// ==========================================
// Lấy ra danh sách K nước đi tiềm năng nhất để Minimax không bị quá tải
std::vector<MoveVal> GetTopMoves(const int board[30][30], int boardSize, int maxMoves) {
    std::vector<MoveVal> moves;
    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            if (board[i][j] == EMPTY && HasNeighbor(board, boardSize, i, j)) {
                long long score = EvaluateCell(board, boardSize, i, j);
                moves.push_back({ i, j, score });
            }
        }
    }
    std::sort(moves.begin(), moves.end(), CompareMoves);
    if (moves.size() > maxMoves) moves.resize(maxMoves);
    return moves;
}

// ==========================================
// ĐÁNH GIÁ TOÀN BÀN CỜ (Dùng cho nhánh lá của Minimax)
// ==========================================
long long EvaluateBoardState(const int board[30][30], int boardSize) {
    long long botScore = 0;
    long long playerScore = 0;

    int dx[] = { 1, 0, 1, 1 };
    int dy[] = { 0, 1, 1, -1 };

    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            if (board[i][j] == EMPTY) continue;
            int player = board[i][j];

            for (int d = 0; d < 4; d++) {
                int prevX = i - dx[d], prevY = j - dy[d];
                // Chỉ quét theo chiều tiến để tránh tính lặp 1 chuỗi 2 lần
                if (IsValid(prevX, prevY, boardSize) && board[prevX][prevY] == player) continue;

                int count = 1;
                int blocks = 0;

                if (!IsValid(prevX, prevY, boardSize) || board[prevX][prevY] != EMPTY) blocks++;

                int nextX = i + dx[d], nextY = j + dy[d];
                while (IsValid(nextX, nextY, boardSize) && board[nextX][nextY] == player) {
                    count++;
                    nextX += dx[d];
                    nextY += dy[d];
                }

                if (!IsValid(nextX, nextY, boardSize) || board[nextX][nextY] != EMPTY) blocks++;

                long long score = GetPatternScore(count, blocks);
                if (player == BOT) botScore += score;
                else playerScore += score;
            }
        }
    }
    return botScore - playerScore;
}

// ==========================================
// THUẬT TOÁN MINIMAX VỚI ALPHA-BETA PRUNING
// ==========================================
long long Minimax(int board[30][30], int boardSize, int depth, long long alpha, long long beta, bool isMaximizing) {
    long long boardVal = EvaluateBoardState(board, boardSize);

    // Nếu đạt độ sâu tối đa hoặc đã có người thắng (điểm quá lớn)
    if (depth == 0 || std::abs(boardVal) > 50000000) {
        return boardVal;
    }

    std::vector<MoveVal> moves = GetTopMoves(board, boardSize, 12); // Chỉ xét 12 nước đi tốt nhất
    if (moves.empty()) return boardVal; // Bàn cờ hòa hoặc đầy

    if (isMaximizing) { // Lượt của BOT
        long long maxEval = -LLONG_MAX;
        for (const auto& move : moves) {
            board[move.x][move.y] = BOT;
            long long eval = Minimax(board, boardSize, depth - 1, alpha, beta, false);
            board[move.x][move.y] = EMPTY; // Backtracking

            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break; // Cắt tỉa Alpha-Beta
        }
        return maxEval;
    }
    else { // Lượt của PLAYER
        long long minEval = LLONG_MAX;
        for (const auto& move : moves) {
            board[move.x][move.y] = PLAYER;
            long long eval = Minimax(board, boardSize, depth - 1, alpha, beta, true);
            board[move.x][move.y] = EMPTY; // Backtracking

            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break; // Cắt tỉa Alpha-Beta
        }
        return minEval;
    }
}

// ==========================================
// HÀM TÌM NƯỚC ĐI TỐI ƯU DỰA TRÊN ĐỘ SÂU (LEVEL)
// ==========================================
void FindBestMoveMinimax(int board[30][30], int boardSize, int depth, int* outX, int* outY) {
    std::vector<MoveVal> moves = GetTopMoves(board, boardSize, 12);
    if (moves.empty()) return;

    // Cấp độ DỄ: Chỉ lấy điểm đánh giá cục bộ cao nhất, không nội suy tương lai
    if (depth <= 1) {
        *outX = moves[0].x;
        *outY = moves[0].y;
        return;
    }

    // Cấp độ VỪA và KHÓ: Chạy Minimax
    long long bestScore = -LLONG_MAX;
    *outX = moves[0].x;
    *outY = moves[0].y;

    for (const auto& move : moves) {
        board[move.x][move.y] = BOT;
        long long score = Minimax(board, boardSize, depth - 1, -LLONG_MAX, LLONG_MAX, false);
        board[move.x][move.y] = EMPTY;

        if (score > bestScore) {
            bestScore = score;
            *outX = move.x;
            *outY = move.y;
        }
    }
}

// ==========================================
// HÀM CHÍNH ĐƯỢC GỌI TỪ NGOÀI
// ==========================================
void CalculateBestMove(const int board[30][30], int boardSize, int level, int* outX, int* outY) {
    // Tạo bản sao bàn cờ để thuật toán có thể điền thử (vì mảng truyền vào là const)
    int tempBoard[30][30];
    int pieceCount = 0;

    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            tempBoard[i][j] = board[i][j];
            if (board[i][j] != EMPTY) pieceCount++;
        }
    }

    // Nước đầu tiên của ván cờ luôn đánh ở chính giữa bàn
    if (pieceCount == 0) {
        *outX = boardSize / 2;
        *outY = boardSize / 2;
        return;
    }

    // Phân luồng Cấp Độ
    if (level == 1) {
        FindBestMoveMinimax(tempBoard, boardSize, 2, outX, outY);
    }
    else if (level == 2) {
        FindBestMoveMinimax(tempBoard, boardSize, 4, outX, outY);
    }
    else {
        FindBestMoveMinimax(tempBoard, boardSize, 1, outX, outY);

    }
}