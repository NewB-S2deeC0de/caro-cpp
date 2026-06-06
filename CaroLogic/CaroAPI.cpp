#include "CaroAPI.h"
#include "LogicEngine.h"
#include "AIEngine.h"
#include "DataIO.h"
#include <atomic>
#include <future>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <random>
#include <fstream>

// ============================================================
//  TRẠNG THÁI BÀN CỜ
// ============================================================
int  g_board[30][30] = { 0 };
int  g_boardSize = 15;
bool g_ruleBlock2 = true;
int  g_aiLevel = 1;
int  g_savedGameMode = 1; // 1 = PVE, 2 = PVP

// Tọa độ đường thắng (cho GUI vẽ)
int g_winStartX = -1;
int g_winStartY = -1;
int g_winEndX = -1;
int g_winEndY = -1;

// ============================================================
//  LỊCH SỬ NƯỚC ĐI – Stack để hỗ trợ Undo
//  MoveRecord được khai báo chung trong DataIO.h để DataIO.cpp cũng dùng được.
// ============================================================
MoveRecord g_history[900];
int        g_historyCount = 0;

// ============================================================
//  VIRUS MODE
//  g_board[x][y] == 3 nghia la o virus dang khoa.
// ============================================================
bool g_virusMode = false;
int  g_virusMoveCounter = 0;
int  g_virusTTL[30][30] = { 0 };

unsigned int g_virusSeed = 0; 
std::mt19937 g_virusRng; 

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

static void ClearVirusCells()
{
    g_virusMoveCounter = 0;
    for (int i = 0; i < 30; ++i)
    {
        for (int j = 0; j < 30; ++j)
        {
            g_virusTTL[i][j] = 0;
            if (g_board[i][j] == 3) g_board[i][j] = 0;
        }
    }
}

static int CountVirusCells()
{
    int cnt = 0;
    for (int i = 0; i < g_boardSize; ++i)
        for (int j = 0; j < g_boardSize; ++j)
            if (g_virusTTL[i][j] > 0 && g_board[i][j] == 3) ++cnt;
    return cnt;
}

static int GetVirusWaveSizeInternal()
{
    // Virus se nang cap theo so nuoc da di trong van.
    //  0 - 11 nuoc: moi dot gay nhiem 1 o
    // 12 - 23 nuoc: moi dot gay nhiem 2 o
    // tu 24 nuoc  : moi dot gay nhiem 3 o
    if (g_historyCount >= 24) return 3;
    if (g_historyCount >= 12) return 2;
    return 1;
}

static int GetVirusMaxCellsInternal()
{
    // Khong gioi han virus o muc 5/10/16 nua.
    // Virus se tiep tuc lan sau moi dot cho den khi ban co het o trong
    // hoac van dau ket thuc vi co nguoi chien thang / hoa.
    return g_boardSize * g_boardSize;
}

static bool InfectVirusCell(int x, int y, int threatLevel)
{
    if (x < 0 || x >= g_boardSize || y < 0 || y >= g_boardSize) return false;
    if (g_board[x][y] != 0) return false;

    g_board[x][y] = 3;
    // Gia tri nay khong con la TTL nua. No dai dien cho muc gay hai 1/2/3
    // de GUI co the ve so tren o virus.
    g_virusTTL[x][y] = threatLevel;
    return true;
}

static bool FindRandomEmptyCell(int& outX, int& outY)
{
    int emptyCount = 0;
    for (int i = 0; i < g_boardSize; ++i)
        for (int j = 0; j < g_boardSize; ++j)
            if (g_board[i][j] == 0) ++emptyCount;

    if (emptyCount <= 0) return false;

    int pick = g_virusRng() % emptyCount;
    for (int i = 0; i < g_boardSize; ++i)
    {
        for (int j = 0; j < g_boardSize; ++j)
        {
            if (g_board[i][j] == 0)
            {
                if (pick == 0)
                {
                    outX = i;
                    outY = j;
                    return true;
                }
                --pick;
            }
        }
    }
    return false;
}

static void SpawnVirusWave()
{
    if (!g_virusMode) return;

    int threatLevel = GetVirusWaveSizeInternal();
    int maxCells = GetVirusMaxCellsInternal();
    int active = CountVirusCells();
    if (active >= maxCells) return;

    //static bool seeded = false;
    //if (!seeded) { std::srand(static_cast<unsigned>(std::time(nullptr))); seeded = true; }

    int centerX = -1;
    int centerY = -1;
    if (!FindRandomEmptyCell(centerX, centerY)) return;

    int infected = 0;
    if (InfectVirusCell(centerX, centerY, threatLevel))
    {
        ++infected;
        ++active;
    }

    // Khi threatLevel = 2/3, virus uu tien lay them o sat ben canh de tao cum gay nhieu.
    const int dirs[8][2] = {
        { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
        { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
    };
    bool used[8] = { false };

    while (infected < threatLevel && active < maxCells)
    {
        bool placed = false;

        for (int tries = 0; tries < 16 && !placed; ++tries)
        {
            int k = g_virusRng() % 8;
            if (used[k]) continue;
            used[k] = true;

            int nx = centerX + dirs[k][0];
            int ny = centerY + dirs[k][1];
            if (InfectVirusCell(nx, ny, threatLevel))
            {
                ++infected;
                ++active;
                placed = true;
            }
        }

        // Neu xung quanh tam khong con o trong thi lay ngau nhien o khac.
        if (!placed)
        {
            int rx = -1;
            int ry = -1;
            if (!FindRandomEmptyCell(rx, ry)) break;
            if (InfectVirusCell(rx, ry, threatLevel))
            {
                ++infected;
                ++active;
            }
            else break;
        }
    }
}

static void AdvanceVirusAfterValidMove()
{
    if (!g_virusMode) return;

    // Ban moi: bo decay/burst. Virus khong tu mat di nua.
    // Do gay hai tang theo so nuoc da di: dot sau co the nhiem 2 hoac 3 o.
    ++g_virusMoveCounter;
    if (g_virusMoveCounter >= 4)
    {
        g_virusMoveCounter = 0;
        SpawnVirusWave();
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

    // Reset lịch sử, đường thắng và virus cell
    g_historyCount = 0;
    g_winStartX = g_winStartY = g_winEndX = g_winEndY = -1;
    ClearVirusCells();

    g_virusSeed = static_cast<unsigned int>(std::time(nullptr));
    g_virusRng.seed(g_virusSeed);
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

    int result = CheckWinCondition(x, y, player);
    if (result == 0)
    {
        AdvanceVirusAfterValidMove();
    }

    return result;
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
        if (result.state == 0)
        {
            AdvanceVirusAfterValidMove();
        }

        g_aiResult = result;
        g_hasAIResult = true;

        // Danh dau AI da xong
        g_isAiThinking = false;
    }
}

// ============================================================
//  Save / Load
// ============================================================


extern "C" CARO_API void SetSaveGameMode(int mode)
{
    if (mode == 1 || mode == 2) g_savedGameMode = mode;
}

extern "C" CARO_API int GetSavedGameMode()
{
    return g_savedGameMode;
}

extern "C" CARO_API int GetSlotGameMode(int slotId)
{
    return PeekSlotGameMode(slotId);
}

extern "C" CARO_API int GetSlotVirusMode(int slotId)
{
    return PeekSlotVirusMode(slotId);
}

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
extern "C" CARO_API bool GetHintMove(int player, int* outX, int* outY)
{
    if (!outX || !outY) return false;
    if (player != 1 && player != 2) return false;

    *outX = -1;
    *outY = -1;

    int hintBoard[30][30] = { 0 };

    for (int i = 0; i < g_boardSize; ++i)
    {
        for (int j = 0; j < g_boardSize; ++j)
        {
            // Virus cell xem nhu o bi chan.
            if (g_board[i][j] == 3)
            {
                hintBoard[i][j] = (player == 1) ? 1 : 1;
                continue;
            }

            if (player == 1)
            {
                if (g_board[i][j] == 1) hintBoard[i][j] = 2;
                else if (g_board[i][j] == 2) hintBoard[i][j] = 1;
                else hintBoard[i][j] = 0;
            }
            else
            {
                hintBoard[i][j] = g_board[i][j];
            }
        }
    }

    int hx = -1;
    int hy = -1;
    CalculateBestMove(hintBoard, g_boardSize, g_aiLevel, &hx, &hy);

    if (hx < 0 || hx >= g_boardSize || hy < 0 || hy >= g_boardSize) return false;
    if (g_board[hx][hy] != 0) return false;

    *outX = hx;
    *outY = hy;
    return true;
}

extern "C" CARO_API void SetVirusMode(bool enabled)
{
    g_virusMode = enabled;
    if (!enabled)
    {
        ClearVirusCells();
    }
}

extern "C" CARO_API bool IsVirusMode()
{
    return g_virusMode;
}

extern "C" CARO_API int GetVirusCell(int x, int y)
{
    if (x < 0 || x >= g_boardSize || y < 0 || y >= g_boardSize) return 0;
    if (g_board[x][y] == 3 && g_virusTTL[x][y] > 0) return g_virusTTL[x][y];
    return 0;
}

extern "C" CARO_API void GetVirusInfo(bool* outEnabled, int* outActiveCount, int* outMoveCounter)
{
    if (outEnabled) *outEnabled = g_virusMode;
    if (outActiveCount) *outActiveCount = CountVirusCells();
    if (outMoveCounter) *outMoveCounter = g_virusMoveCounter;
}

extern "C" CARO_API int GetVirusThreatLevel()
{
    return GetVirusWaveSizeInternal();
}

extern "C" CARO_API int GetVirusMaxCells()
{
    return GetVirusMaxCellsInternal();
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

extern "C" CARO_API bool SaveGameReplay(const std::string& filename)
{
    return SaveReplayBinary(filename);
}

static MoveRecord g_replayHistory[900];
static int g_replayTotalMoves = 0;
static int g_replayCurrentMove = 0;

extern "C" CARO_API bool LoadGameReplay(const char* filename) {
    ReplayMetadata meta;
    if (LoadReplayBinary(std::string(filename), &meta, g_replayHistory)) {
        // Dọn dẹp bàn cờ và thiết lập luật chơi y hệt lúc ghi hình
        InitGame(meta.boardSize, meta.ruleBlock2, meta.aiLevel);
        SetVirusMode(meta.virusMode);

        g_replayTotalMoves = meta.historyCount;
        g_replayCurrentMove = 0;
        
        g_virusSeed = meta.virusSeed; 
        g_virusRng.seed(g_virusSeed);
        return true;
    }
    return false;
}

extern "C" CARO_API bool ProcessNextReplayMove() {
    if (g_replayCurrentMove < g_replayTotalMoves) {
        MoveRecord m = g_replayHistory[g_replayCurrentMove];

        ProcessMove(m.x, m.y, m.player);

        g_replayCurrentMove++;
        return true;
    }
    return false; 
}

extern "C" CARO_API bool PeekReplayFile(const char* filename, int* outBoardSize, int* outMoves, bool* outVirusMode, char* outDate) {
    ReplayMetadata meta;
    if (PeekReplayMetadata(std::string(filename), &meta)) {
        *outBoardSize = meta.boardSize;
        *outMoves = meta.historyCount;
        *outVirusMode = meta.virusMode;
        strcpy_s(outDate, 32, meta.replayDate);
        return true;
    }
    return false;
}

extern "C" CARO_API bool GetReplayPreview
(
    const char* filename, int* outBoardSize,
    int* outMoves, int* outVirusMode, char* outDate,
    int outBoard[30][30]
)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return false; 
    }

    ReplayMetadata meta; 
    file.read(reinterpret_cast<char*>(&meta), sizeof(ReplayMetadata)); 
    if (std::strcmp(meta.magic, "CAROREP") != 0) 
    {
        return false;
    }

    *outBoardSize = meta.boardSize;
    *outMoves = meta.historyCount;
    *outVirusMode = meta.virusMode ? 1 : 0;
    strcpy_s(outDate, 32, meta.replayDate);

    if (meta.historyCount > 0) 
    {
        file.seekg(sizeof(MoveRecord) * meta.historyCount, std::ios::cur); 
    }

    if (file.read(reinterpret_cast<char*>(outBoard), sizeof(int) * 30 * 30))
    {
        file.close(); 
        return true;
    }

    file.clear(); 
    file.seekg(sizeof(ReplayMetadata), std::ios::beg); 
    
    for (int i = 0; i < 30; i++)
    {
        for (int j = 0; j < 30; j++)
        {
            outBoard[i][j] = 0;
        }
    }

    for (int i = 0; i < meta.historyCount; i++) {
        MoveRecord m; 
        file.read(reinterpret_cast<char*>(&m), sizeof(MoveRecord)); 
        outBoard[m.x][m.y] = m.player;
    }

    file.close();
    return false; 
}

extern "C" CARO_API bool DeleteReplayFile(const char* filename) {
    return std::remove(filename) == 0;
}