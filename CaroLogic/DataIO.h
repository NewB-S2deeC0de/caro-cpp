#pragma once

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
