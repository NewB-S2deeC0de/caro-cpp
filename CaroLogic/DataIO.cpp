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

extern MoveRecord g_history[900];
extern int g_historyCount;

extern bool g_virusMode;
extern int g_virusMoveCounter;
extern int g_virusTTL[30][30];
extern int g_savedGameMode;

static void ClearVirusStateIO()
{
    g_virusMode = false;
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

    VirusSaveBlock virusBlock{};
    strcpy_s(virusBlock.magic, sizeof(virusBlock.magic), "VIRUSV");
    virusBlock.version = 1;
    virusBlock.virusMode = g_virusMode;
    virusBlock.virusMoveCounter = g_virusMoveCounter;
    virusBlock.gameMode = g_savedGameMode;
    for (int i = 0; i < 30; ++i)
        for (int j = 0; j < 30; ++j)
            virusBlock.virusTTL[i][j] = g_virusTTL[i][j];
    file.write(reinterpret_cast<const char*>(&virusBlock), sizeof(VirusSaveBlock));

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

    ClearVirusStateIO();

    // Doc block virus theo cach tuong thich nguoc: ban cu chua co gameMode van load duoc virus.
    char vMagic[8] = {};
    if (file.read(reinterpret_cast<char*>(vMagic), sizeof(vMagic))) {
        if (std::strcmp(vMagic, "VIRUSV") == 0) {
            int vVersion = 1;
            bool vMode = false;
            int vCounter = 0;
            int vTTL[30][30] = { 0 };
            int vGameMode = 0;

            file.read(reinterpret_cast<char*>(&vVersion), sizeof(vVersion));
            file.read(reinterpret_cast<char*>(&vMode), sizeof(vMode));
            file.read(reinterpret_cast<char*>(&vCounter), sizeof(vCounter));
            file.read(reinterpret_cast<char*>(vTTL), sizeof(vTTL));
            if (file.read(reinterpret_cast<char*>(&vGameMode), sizeof(vGameMode))) {
                if (vGameMode == 1 || vGameMode == 2) g_savedGameMode = vGameMode;
            }

            g_virusMode = vMode;
            g_virusMoveCounter = vCounter;
            for (int i = 0; i < 30; ++i)
                for (int j = 0; j < 30; ++j)
                {
                    int level = vTTL[i][j];
                    if (level > 3) level = 1; // save cu cua ban decay dung TTL 4-6
                    if (level < 0) level = 0;
                    g_virusTTL[i][j] = level;

                    // BUG FIX: Load xong phai dat lai o virus tren board.
                    // ClearVirusStateIO da xoa cac o board == 3 ve 0, nen can restore tu virusTTL.
                    if (i < g_boardSize && j < g_boardSize && level > 0 && g_board[i][j] == 0) {
                        g_board[i][j] = 3;
                    }
                }
        }
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


int PeekSlotGameMode(int slotId) {
    std::ifstream file(GetSlotPath(slotId), std::ios::in | std::ios::binary);
    if (!file.is_open()) return 0;

    SaveMetadata meta{};
    file.read(reinterpret_cast<char*>(&meta), sizeof(SaveMetadata));
    if (std::strcmp(meta.magic, "CAROSAV") != 0) return 0;

    file.seekg(sizeof(int) * 30 * 30, std::ios::cur);
    if (meta.historyCount > 0) {
        file.seekg(sizeof(MoveRecord) * meta.historyCount, std::ios::cur);
    }

    char vMagic[8] = {};
    if (!file.read(reinterpret_cast<char*>(vMagic), sizeof(vMagic))) return 0;
    if (std::strcmp(vMagic, "VIRUSV") != 0) return 0;

    int vVersion = 1;
    bool vMode = false;
    int vCounter = 0;
    int vTTL[30][30] = { 0 };
    int vGameMode = 0;

    file.read(reinterpret_cast<char*>(&vVersion), sizeof(vVersion));
    file.read(reinterpret_cast<char*>(&vMode), sizeof(vMode));
    file.read(reinterpret_cast<char*>(&vCounter), sizeof(vCounter));
    file.read(reinterpret_cast<char*>(vTTL), sizeof(vTTL));
    if (!file.read(reinterpret_cast<char*>(&vGameMode), sizeof(vGameMode))) return 0;

    return (vGameMode == 1 || vGameMode == 2) ? vGameMode : 0;
}


int PeekSlotVirusMode(int slotId) {
    std::ifstream file(GetSlotPath(slotId), std::ios::in | std::ios::binary);
    if (!file.is_open()) return 0; // Save cu chua co block virus -> NORMAL

    SaveMetadata meta{};
    file.read(reinterpret_cast<char*>(&meta), sizeof(SaveMetadata));
    if (std::strcmp(meta.magic, "CAROSAV") != 0) return 0;

    file.seekg(sizeof(int) * 30 * 30, std::ios::cur);
    if (meta.historyCount > 0) {
        file.seekg(sizeof(MoveRecord) * meta.historyCount, std::ios::cur);
    }

    char vMagic[8] = {};
    if (!file.read(reinterpret_cast<char*>(vMagic), sizeof(vMagic))) return 0;
    if (std::strcmp(vMagic, "VIRUSV") != 0) return 0;

    int vVersion = 1;
    bool vMode = false;
    int vCounter = 0;

    file.read(reinterpret_cast<char*>(&vVersion), sizeof(vVersion));
    if (!file.read(reinterpret_cast<char*>(&vMode), sizeof(vMode))) return 0;
    if (!file.read(reinterpret_cast<char*>(&vCounter), sizeof(vCounter))) return vMode ? 1 : 0;

    return vMode ? 1 : 0;
}

bool SaveReplayBinary(const std::string& filename) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    ReplayMetadata meta = {};
    strcpy_s(meta.magic, sizeof(meta.magic), "CAROREP");
    meta.version = 1;
    meta.boardSize = g_boardSize;
    meta.ruleBlock2 = g_ruleBlock2;
    meta.aiLevel = g_aiLevel;
    meta.virusMode = g_virusMode;
    meta.historyCount = g_historyCount;

    std::time_t t = std::time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &t);
    std::strftime(meta.replayDate, sizeof(meta.replayDate), "%d/%m/%Y %H:%M", &timeinfo);

    file.write(reinterpret_cast<const char*>(&meta), sizeof(ReplayMetadata));

    if (g_historyCount > 0) {
        file.write(reinterpret_cast<const char*>(g_history), sizeof(MoveRecord) * g_historyCount);
    }

    file.close();
    return true;
}

bool LoadReplayBinary(const std::string& filename, ReplayMetadata* outMeta, MoveRecord* outHistory) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    file.read(reinterpret_cast<char*>(outMeta), sizeof(ReplayMetadata));
    if (std::strcmp(outMeta->magic, "CAROREP") != 0) return false;

    if (outMeta->historyCount > 0) {
        file.read(reinterpret_cast<char*>(outHistory), sizeof(MoveRecord) * outMeta->historyCount);
    }

    file.close();
    return true;
}