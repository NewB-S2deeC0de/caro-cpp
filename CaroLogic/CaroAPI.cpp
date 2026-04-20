#include "CaroAPI.h"
#include "LogicEngine.h"
#include "AIEngine.h"
#include "DataIO.h"
#include <atomic>
#include <future>
#include <cstring>

// ============================================================
//  TRẠNG THÁI BÀN CỜ
// ============================================================
int  g_board[30][30] = { 0 };
int  g_boardSize = 15;
bool g_ruleBlock2 = true;
int  g_aiLevel = 1;

// Tọa độ đường thắng (cho GUI vẽ)
int g_winStartX = -1;
int g_winStartY = -1;
int g_winEndX = -1;
int g_winEndY = -1;

// ============================================================
//  LỊCH SỬ NƯỚC ĐI – Stack để hỗ trợ Undo
//  Mỗi MoveRecord ghi lại (x, y, player) của 1 nước đã đặt.
//  Kích thước tối đa: 30×30 = 900 nước.
// ============================================================
struct MoveRecord {
    int x;
    int y; 
    int player; // player: 1 = người (X), 2 = AI (O)
};

MoveRecord g_history[900];
int        g_historyCount = 0;

// Helper nội bộ: đẩy 1 nước vào stack
static void PushHistory(int x, int y, int player)
{
    if (g_historyCount < 900)
    {
        g_history[g_historyCount].x = x;
        g_history[g_historyCount].y = y;
        g_history[g_historyCount].player = player;
        ++g_historyCount;
    }
}

// ============================================================
//  LUỒNG AI
// ============================================================
std::atomic<bool> g_isAiThinking = false;
std::future<AIMoveResult> g_aiTask;

static AIMoveResult g_aiResult{ -1, -1, 0 }; 
static std::atomic<bool> g_hasAIResult = false; 
// ============================================================
//  InitGame – Khởi tạo / reset toàn bộ trạng thái
// ============================================================
extern "C" CARO_API void InitGame(int size, bool ruleBlock2, int level)
{
    g_boardSize = size;
    g_ruleBlock2 = ruleBlock2;
    g_aiLevel = level;

    for (int i = 0; i < 30; ++i)
    {
        for (int j = 0; j < 30; ++j)
        {
            g_board[i][j] = 0;
}
    }

    // Reset lịch sử và đường thắng
    g_historyCount = 0;
    g_winStartX = g_winStartY = g_winEndX = g_winEndY = -1;
}

// ============================================================
//  GetCell
// ============================================================
extern "C" CARO_API int GetCell(int x, int y)
{
    if (x < 0 || x >= g_boardSize || y < 0 || y >= g_boardSize)
    {
        return -1;
    }
    return g_board[x][y];
}

// ============================================================
//  GetWinLine
// ============================================================
extern "C" CARO_API void GetWinLine(int* sx, int* sy, int* ex, int* ey)
{
    *sx = g_winStartX;
    *sy = g_winStartY;
    *ex = g_winEndX;
    *ey = g_winEndY;
}

// ============================================================
//  ProcessPlayerMove – giữ lại để tương thích cũ (PvE)
// ============================================================
extern "C" CARO_API int ProcessPlayerMove(int x, int y)
{
    return ProcessMove(x, y, 1);
}

// ============================================================
//  ProcessMove – Tổng quát, dùng cho cả PVP và PVE
//  player: 1 = X (người 1), 2 = O (người 2 hoặc AI)
// ============================================================
extern "C" CARO_API int ProcessMove(int x, int y, int player)
{
    if (x < 0 || x >= g_boardSize ||
        y < 0 || y >= g_boardSize ||
        g_board[x][y] != 0 ||
        (player != 1 && player != 2))
    {
        return -1;
    }

    g_board[x][y] = player;
    PushHistory(x, y, player);

    return CheckWinCondition(x, y, player);
}

// ============================================================
//  UndoMove – Lùi lại theo CẶP nước (AI + người)
//
//  Logic:
//   - Nếu stack có >= 2 nước: xóa 2 nước trên cùng (AI rồi người)
//   - Nếu stack chỉ có 1 nước : xóa 1 nước đó
//   - Nếu stack rỗng           : không làm gì, trả về 0
//
//  Sau khi undo:
//   - Ô bàn cờ tương ứng bị xóa về 0
//   - Nếu có người thắng thì không thể undo được nữa, ván cờ kết thúc
//   - Trả về số nước đã undo thực tế (0 / 1 / 2)
// ============================================================
extern "C" CARO_API int UndoMove()
{
    //Nếu đã có đường thẳng thì không được undo nữa
    if (g_winStartX != -1) {
        return 0;
    }
    if (g_historyCount == g_boardSize * g_boardSize) {
        return 0;
    }
    if (g_historyCount == 0) return 0; // không có gì để undo


    int undone = 0;

    // Undo nước 1 (nước trên cùng stack — thường là nước AI)
    {
        --g_historyCount;
        int x = g_history[g_historyCount].x;
        int y = g_history[g_historyCount].y;
        g_board[x][y] = 0;
        ++undone;
    }

    // Undo thêm nước 2 nếu còn (nước người chơi đứng ngay trước AI)
    if (g_historyCount > 0)
    {
        --g_historyCount;
        int x = g_history[g_historyCount].x;
        int y = g_history[g_historyCount].y;
        g_board[x][y] = 0;
        ++undone;
    }

    return undone; // 1 hoặc 2
}

// ============================================================
//  UndoOneMove – Xóa đúng 1 nước trên cùng stack
//  Dùng cho PVP: mỗi người chỉ lùi lại nước của chính mình
// ============================================================
extern "C" CARO_API int UndoOneMove()
{
    if (g_winStartX != -1) {
        return 0;
    }
    if (g_historyCount == g_boardSize * g_boardSize) {
        return 0;
    }
    if (g_historyCount == 0) return 0;

    --g_historyCount;
    int x = g_history[g_historyCount].x;
    int y = g_history[g_historyCount].y;
    g_board[x][y] = 0;

    return 1;
}

extern "C" CARO_API void StartAIThinking()
{
    g_isAiThinking = true;

    // Snapshot bàn cờ để tránh data race
    int boardCopy[30][30];
    for (int i = 0; i < g_boardSize; ++i)
    {
        for (int j = 0; j < g_boardSize; ++j)
        {
            boardCopy[i][j] = g_board[i][j];
        }
    }

    // Worker Thread 
    // Doc snapshot
    // Return result 
    g_aiTask = std::async(std::launch::async, [boardCopy,
        size = g_boardSize, level = g_aiLevel]() -> AIMoveResult
        {
            AIMoveResult res;
            CalculateBestMove(boardCopy, size, level, &res.x, &res.y);

            //res.state = CheckWinCondition(res.x, res.y, 2);
            return res;
        });
}

// ============================================================
//  IsAIThinking / GetAIResult
// ============================================================
extern "C" CARO_API bool IsAIThinking()
{
    return g_isAiThinking;
}

extern "C" CARO_API int GetAIResult(int* outX, int* outY)
{
    if (!g_hasAIResult)
    {
        return 0; 
    }
    *outX = g_aiResult.x;
    *outY = g_aiResult.y;
    
    int result = g_aiResult.state;

    // Reset sau khi lay ket qua

    g_hasAIResult = false; 

    return result; 
}

extern "C" CARO_API void UpdateAI()
{
    // Neu AI khong chay -> khong lam gi
    if (!g_isAiThinking)
    {
        return;
    }

    // Kiem tra task da hoan thanh chua (KHONG block)
    if (g_aiTask.valid() &&
        g_aiTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            // Lay ket qua tu worker thread
            auto result = g_aiTask.get(); 

            // Duy nhat tai day moi duoc phep sua state
            g_board[result.x][result.y] = 2;
            PushHistory(result.x, result.y, 2); 

            result.state = CheckWinCondition(result.x, result.y, 2);

            g_aiResult = result; 
            g_hasAIResult = true; 

            // Danh dau AI da xong
        g_isAiThinking = false;
        }
}

// ============================================================
//  Save / Load
// ============================================================

extern "C" CARO_API bool SaveGameSlot(int slotId, float timeLeft, int isPlayerTurn, const char* gameName) {
    return SaveSlotBinary(slotId, timeLeft, isPlayerTurn, gameName);
}

extern "C" CARO_API bool LoadGameSlot(int slotId, float* timeLeft, int* isPlayerTurn) {
    return LoadSlotBinary(slotId, timeLeft, isPlayerTurn);
}

extern "C" CARO_API bool PeekGameSlot(int slotId, int* outBoardSize, int* outMoves, int* outTurn, char* outName) {
    SaveMetadata meta;
    if (PeekSlotMetadata(slotId, &meta)) {
        *outBoardSize = meta.boardSize; *outMoves = meta.historyCount; *outTurn = meta.isPlayerTurn;
        strcpy_s(outName, 64, meta.gameName);
        return true;
    }
    return false;
}

extern "C" CARO_API bool DeleteGameSlot(int slotId) { return DeleteSlotBinary(slotId); }

extern "C" CARO_API bool GetSlotPreview(int slotId, int* outBoardSize, int* outMoves, int* outTurn, char* outDate, char* outName, int outBoard[30][30]) {
    SaveMetadata meta;
    if (PeekSlotPreview(slotId, &meta, outBoard)) {
        *outBoardSize = meta.boardSize; *outMoves = meta.historyCount; *outTurn = meta.isPlayerTurn;
        strcpy_s(outDate, 32, meta.saveDate); strcpy_s(outName, 64, meta.gameName);
        return true;
    }
    return false;
}
extern "C" CARO_API int EvaluateBoard() {
    // Nếu chưa có nước cờ nào được đánh thì chắc chắn là đang chơi (0)
    if (g_historyCount == 0) return 0;
    // Lấy tọa độ của nước đi cuối cùng ra
    int lastX = g_history[g_historyCount - 1].x;
    int lastY = g_history[g_historyCount - 1].y;
    int lastPlayer = g_history[g_historyCount - 1].player;
    // Check lại xem nước đi cuối đó có tạo thành 5 ố thẳng hàng không
    return CheckWinCondition(lastX, lastY, lastPlayer);
}