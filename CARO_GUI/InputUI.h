#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include "Constants.h"

// ============================================================
//  InputUI.h – Khai báo các hàm xử lý input theo từng màn hình
//
//  Nguyên tắc: main.cpp cho gọi hàm, không ch?a logic click.
//  Toàn bộ logic kiểm tra tọa độ chuột nằm trong InputUI.cpp.
// ============================================================

// Xử lý click chuột ở màn hình Menu
//   gameMode: Được SET bên trong hàm khi người chơi chọn PVP hoặc PVE
void HandleMenuInput(
    sf::RenderWindow& window,
    int mouseX, int mouseY,
    AppState& currentState,
    GameMode& gameMode,          // <-- THÊM: để biết chế độ khi vào game
    int boardSize, bool ruleBlock2, int aiLevel,
    float& timeRemaining, bool& isPlayerTurn, int& gameStatus,
    sf::Sound& errSound, int& currentLoadedSlot,
    std::string& currentLoadedName
);

// Xử lý click chuột khi đang trong trận (bàn cờ + nút Undo/Save/Menu)
//   undoLeft[0] = số undo còn lại củaa P1, undoLeft[1] = của P2
//   lastUndoPlayer: 0/1 = người vừa dùng undo, -1 = chưa ai dùng
void HandleInGameInput(
    int mouseX, int mouseY, 
    AppState& currentState, 
    int boardSize, GameMode gameMode, 
    bool& isPlayerTurn, int& gameStatus, 
    float& timeRemaining, int undoLeft[2], 
    int& lastUndoPlayer, float& saveNotifTimer, 
    sf::Sound& errSound, int& currentLoadedSlot, 
    std::string& currentLoadedName
);

// Xử lý click chuột ở màn hình Cài đặt (không ??i)
void HandleSettingsInput(
    int mouseX, int mouseY,
    AppState& currentState,
    int& boardSize, bool& ruleBlock2, int& aiLevel,
    float& sfxVolume, bool& bgmEnabled,
    sf::Sound& errSound
);

void HandleLoadInput(
    sf::RenderWindow& window, 
    int mouseX, int mouseY, 
    AppState& currentState, 
    float& timeRemaining, bool& isPlayerTurn, 
    int& gameStatus, sf::Sound& errSound, 
    int& currentLoadedSlot, 
    std::string& currentLoadedName
);

void HandleSaveInput(
    sf::RenderWindow& window, 
    int mouseX, int mouseY, 
    AppState& currentState, 
    float timeRemaining, bool isPlayerTurn, 
    float& saveNotifTimer, sf::Sound& errSound, 
    bool& isNaming, int& selectedSlot, 
    std::string& inputName, int& currentLoadedSlot, 
    std::string& currentLoadedName
);
void HandleAboutInput(int mouseX, int mouseY, AppState& currentState, sf::Sound& errSound);