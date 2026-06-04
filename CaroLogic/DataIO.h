#pragma once

#include <string>

struct MoveRecord {
    int x;
    int y;
    int player;
};


struct VirusSaveBlock {
    char magic[8];       // "VIRUSV"
    int version;
    bool virusMode;
    int virusMoveCounter;
    int virusTTL[30][30];
    int gameMode;        // 1 = PVE, 2 = PVP
};

struct SaveMetadata {
    char magic[8];       // "CAROSAV"
    int version;         // Phien ban
    int boardSize;
    bool ruleBlock2;
    int aiLevel;
    int isPlayerTurn;
    float timeLeft;
    int historyCount;
    char saveDate[32];   // Ngay gio he thong
    char gameName[64];   // Ten game nguoi dung nhap
};

bool SaveSlotBinary(int slotId, float timeLeft, int isPlayerTurn, const char* gameName);
bool LoadSlotBinary(int slotId, float* outTimeLeft, int* outIsPlayerTurn);
bool PeekSlotMetadata(int slotId, SaveMetadata* outMeta);
bool DeleteSlotBinary(int slotId);
bool PeekSlotPreview(int slotId, SaveMetadata* outMeta, int outBoard[30][30]);
int PeekSlotGameMode(int slotId);
int PeekSlotVirusMode(int slotId);

struct ReplayMetadata {
    char magic[8];       
    int version;
    int boardSize;
    bool ruleBlock2;
    int aiLevel;
    bool virusMode;
    int historyCount;    
    char replayDate[32]; 
};

bool SaveReplayBinary(const std::string& filename);