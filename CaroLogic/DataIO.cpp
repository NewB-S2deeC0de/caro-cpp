#include "DataIO.h"
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <ctime>

extern int g_board[30][30];
extern int g_boardSize;
extern bool g_ruleBlock2;
extern int g_aiLevel;

struct MoveRecord { int x, y, player; };
extern MoveRecord g_history[900];
extern int g_historyCount;

std::string GetSlotPath(int slotId) {
    return "save_slot_" + std::to_string(slotId) + ".bin";
}

bool SaveSlotBinary(int slotId, float timeLeft, int isPlayerTurn, const char* gameName) {
    std::ofstream file(GetSlotPath(slotId), std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    SaveMetadata meta;
    strcpy_s(meta.magic, sizeof(meta.magic), "CAROSAV");
    meta.version = 1;
    meta.boardSize = g_boardSize;
    meta.ruleBlock2 = g_ruleBlock2;
    meta.aiLevel = g_aiLevel;
    meta.isPlayerTurn = isPlayerTurn;
    meta.timeLeft = timeLeft;
    meta.historyCount = g_historyCount;
    strcpy_s(meta.gameName, sizeof(meta.gameName), gameName);

    std::time_t t = std::time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &t);
    std::strftime(meta.saveDate, sizeof(meta.saveDate), "%d/%m/%Y %H:%M", &timeinfo);

    file.write(reinterpret_cast<const char*>(&meta), sizeof(SaveMetadata));
    file.write(reinterpret_cast<const char*>(g_board), sizeof(g_board));
    if (g_historyCount > 0) {
        file.write(reinterpret_cast<const char*>(g_history), sizeof(MoveRecord) * g_historyCount);
    }
    file.close();
    return true;
}

bool LoadSlotBinary(int slotId, float* outTimeLeft, int* outIsPlayerTurn) {
    std::ifstream file(GetSlotPath(slotId), std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    SaveMetadata meta;
    file.read(reinterpret_cast<char*>(&meta), sizeof(SaveMetadata));
    if (std::strcmp(meta.magic, "CAROSAV") != 0) return false;

    g_boardSize = meta.boardSize;
    g_ruleBlock2 = meta.ruleBlock2;
    g_aiLevel = meta.aiLevel;
    *outIsPlayerTurn = meta.isPlayerTurn;
    *outTimeLeft = meta.timeLeft;
    g_historyCount = meta.historyCount;

    file.read(reinterpret_cast<char*>(g_board), sizeof(g_board));
    if (g_historyCount > 0) {
        file.read(reinterpret_cast<char*>(g_history), sizeof(MoveRecord) * g_historyCount);
    }
    file.close();
    return true;
}

bool PeekSlotMetadata(int slotId, SaveMetadata* outMeta) {
    std::ifstream file(GetSlotPath(slotId), std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;
    file.read(reinterpret_cast<char*>(outMeta), sizeof(SaveMetadata));
    file.close();
    return (std::strcmp(outMeta->magic, "CAROSAV") == 0);
}

bool DeleteSlotBinary(int slotId) {
    std::string path = GetSlotPath(slotId);
    return std::remove(path.c_str()) == 0;
}

bool PeekSlotPreview(int slotId, SaveMetadata* outMeta, int outBoard[30][30]) {
    std::ifstream file(GetSlotPath(slotId), std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;
    file.read(reinterpret_cast<char*>(outMeta), sizeof(SaveMetadata));
    if (std::strcmp(outMeta->magic, "CAROSAV") != 0) return false;
    file.read(reinterpret_cast<char*>(outBoard), sizeof(int) * 30 * 30);
    file.close();
    return true;
}
