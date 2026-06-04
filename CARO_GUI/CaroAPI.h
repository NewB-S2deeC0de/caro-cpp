#pragma once

#ifdef CAROLOGIC_EXPORTS
#define CARO_API __declspec(dllexport)
#else
#define CARO_API __declspec(dllimport)
#endif

struct AIMoveResult {
    int x;
    int y;
    int state;
};

extern "C" {
    // 1. Quản lý trạng thái
    CARO_API void InitGame(int size, bool ruleBlock2, int level);
    CARO_API int  GetCell(int x, int y);
    CARO_API void GetWinLine(int* startX, int* startY, int* endX, int* endY);

    // 2. Tương tác game
    //    Trả về: 0=Chơi tiếp, 1=X Thắng, 2=O Thắng, 3=Hòa, -1=Lỗi/ô đã có quân
    CARO_API int  ProcessPlayerMove(int x, int y);

    // Phiên bản tổng quát — dùng cho PVP (player: 1=X, 2=O)
    CARO_API int  ProcessMove(int x, int y, int player);

    // 3. Undo
    //    UndoMove()    – undo theo CẶP (AI + người), dùng cho PVE
    //    UndoOneMove() – undo đúng 1 nước trên cùng stack, dùng cho PVP
    //    Cả hai trả về số nước đã undo thực tế (0 nếu stack rỗng)
    CARO_API int  UndoMove();
    CARO_API int  UndoOneMove();

    // 4. Xử lý AI Đa luồng
    CARO_API void StartAIThinking();
    CARO_API bool IsAIThinking();
    CARO_API int  GetAIResult(int* outX, int* outY);
    CARO_API void UpdateAI();

    // Multi-slot IO
    CARO_API bool SaveGameSlot(int slotId, float timeLeft, int isPlayerTurn, const char* gameName);
    CARO_API bool LoadGameSlot(int slotId, float* timeLeft, int* isPlayerTurn);
    CARO_API bool PeekGameSlot(int slotId, int* outBoardSize, int* outMoves, int* outTurn, char* outName);
    CARO_API bool DeleteGameSlot(int slotId);
    CARO_API bool GetSlotPreview(int slotId, int* outBoardSize, int* outMoves, int* outTurn, char* outDate, char* outName, int outBoard[30][30]);

    // Save info
    // mode: 1 = PVE, 2 = PVP
    CARO_API void SetSaveGameMode(int mode);
    CARO_API int  GetSavedGameMode();
    CARO_API int  GetSlotGameMode(int slotId);
    // 0 = NORMAL, 1 = VIRUS MODE
    CARO_API int  GetSlotVirusMode(int slotId);


    // Hint: goi y nuoc di tot cho player hien tai (1 = X, 2 = O)
    CARO_API bool GetHintMove(int player, int* outX, int* outY);

    // Virus Mode
    CARO_API void SetVirusMode(bool enabled);
    CARO_API bool IsVirusMode();
    CARO_API int  GetVirusCell(int x, int y);       // 0 = khong co virus, 1/2/3 = muc gay hai cua dot nhiem
    CARO_API void GetVirusInfo(bool* outEnabled, int* outActiveCount, int* outMoveCounter);
    CARO_API int  GetVirusThreatLevel();          // 1/2/3: moi dot nhiem bao nhieu o
    CARO_API int  GetVirusMaxCells();             // gioi han so o virus theo threat level

    CARO_API int  EvaluateBoard();
}
