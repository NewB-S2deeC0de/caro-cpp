#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#pragma comment(lib, "sfml-graphics-d.lib")
#pragma comment(lib, "sfml-window-d.lib")
#pragma comment(lib, "sfml-system-d.lib")
#pragma comment(lib, "sfml-audio-d.lib")
#pragma comment(lib, "Winmm.lib")
#include "Constants.h"
#include "CaroAPI.h"
#include "RenderUI.h"
#include "InputUI.h"
#include <iostream>

int main()
{
    sf::RenderWindow window(
        sf::VideoMode(Config::WIN_WIDTH, Config::WIN_HEIGHT),
        "Caro Master"
    );
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf"))
    {
        std::cout << "Loi: Khong tim thay arial.ttf" << std::endl;
    }

    sf::SoundBuffer errBuffer;
    sf::Sound errSound;
    if (errBuffer.loadFromFile("assets/error.wav"))
    {
        errSound.setBuffer(errBuffer);
    }

    sf::Music bgMusic;
    if (bgMusic.openFromFile("assets/bgm.ogg"))
    {
        bgMusic.setLoop(true);
        bgMusic.setVolume(50.f);
        bgMusic.play();
    }

    // ── Trạng thái game ──────────────────────────────────────
    AppState currentState = AppState::MENU_SCREEN;
    GameMode gameMode = GameMode::PVE;

    int   boardSize = 15;
    bool  isPlayerTurn = true;
    float timeRemaining = 60.f;
    int   gameStatus = 0;

    bool  ruleBlock2 = true;
    int   aiLevel = 3;
    float sfxVolume = 100.f;
    bool  bgmEnabled = true;

    int winX1 = -1, winY1 = -1, winX2 = -1, winY2 = -1;

    // ── Trạng thái Undo (PVP) ────────────────────────────────-
    int undoLeft[2] = { Config::UNDO_MAX, Config::UNDO_MAX };
    int lastUndoPlayer = -1;
    float saveNotifTimer = 0.f;

    bool isNaming = false;
    int selectedSlotToSave = -1;
    std::string currentInputName = "";

    int currentLoadedSlot = -1;
    std::string currentLoadedName = "";

    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        sf::Event event;

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (event.type == sf::Event::TextEntered && isNaming && currentState == AppState::SAVE_SCREEN) {
                if (event.text.unicode == '\b') {
                    if (!currentInputName.empty()) {
                        currentInputName.pop_back();
                    }
                }
                else if (event.text.unicode < 128 && event.text.unicode >= 32 && currentInputName.size() < 25) {
                    currentInputName += static_cast<char>(event.text.unicode);
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                if (currentState == AppState::MENU_SCREEN)
                {
                    HandleMenuInput(window, mx, my, currentState, gameMode, boardSize, ruleBlock2, aiLevel, timeRemaining, isPlayerTurn, gameStatus, errSound, currentLoadedSlot, currentLoadedName);
                    // Reset trạng thái ván mới
                    winX1 = winY1 = winX2 = winY2 = -1;
                    undoLeft[0] = undoLeft[1] = Config::UNDO_MAX;
                    lastUndoPlayer = -1;
                    saveNotifTimer = 0.f;
                }
                else if (currentState == AppState::LOAD_SCREEN)
                    HandleLoadInput(window, mx, my, currentState, timeRemaining, isPlayerTurn, gameStatus, errSound, currentLoadedSlot, currentLoadedName);
                else if (currentState == AppState::SAVE_SCREEN)
                    HandleSaveInput(window, mx, my, currentState, timeRemaining, isPlayerTurn, saveNotifTimer, errSound, isNaming, selectedSlotToSave, currentInputName, currentLoadedSlot, currentLoadedName);
                else if (currentState == AppState::IN_GAME_SCREEN)
                    HandleInGameInput(mx, my, currentState, boardSize, gameMode, isPlayerTurn, gameStatus, timeRemaining, undoLeft, lastUndoPlayer, saveNotifTimer, errSound, currentLoadedSlot, currentLoadedName);
                else if (currentState == AppState::SETTINGS_SCREEN)
                {
                    HandleSettingsInput(mx, my, currentState, boardSize, ruleBlock2, aiLevel, sfxVolume, bgmEnabled, errSound);
                    if (bgmEnabled) bgMusic.play(); else bgMusic.pause();
                }
                else if (currentState == AppState::ABOUT_SCREEN) // Xử lý nhấn chuột màn About
                {
                    HandleAboutInput(mx, my, currentState, errSound);
                }
            }
        }

        if (saveNotifTimer > 0.f) saveNotifTimer -= dt;

        // ── Cập nhật logic ───────────────────────────────────
        UpdateAI();
        if (currentState == AppState::IN_GAME_SCREEN && gameStatus == 0)
        {
            timeRemaining -= dt;
            if (timeRemaining <= 0.f)
            {
                gameStatus = isPlayerTurn ? 2 : 1; // Hết giờ thì thua
            }

            if (gameMode == GameMode::PVE && !isPlayerTurn && !IsAIThinking())
            {
                int aiX = -1, aiY = -1;
                int result = GetAIResult(&aiX, &aiY);
                if (aiX != -1)
                {
                    gameStatus = result;
                    if (gameStatus != 0) GetWinLine(&winX1, &winY1, &winX2, &winY2);
                    isPlayerTurn = true;
                    timeRemaining = 60.f;
                }
            }
        }

        // --- FIX LỖI KHỰNG (CHỈ TÍNH WINLINE 1 LẦN) ---
        if (gameStatus != 0)
        {
            if (winX1 == -1) GetWinLine(&winX1, &winY1, &winX2, &winY2);
        }
        else {
            winX1 = winY1 = winX2 = winY2 = -1;
        }

        // ── Vẽ ──────────────────────────────────────────────
        window.clear(BG_COLOR);

        if (currentState == AppState::MENU_SCREEN) DrawMenu(window, font);
        else if (currentState == AppState::LOAD_SCREEN) DrawLoadScreen(window, font);
        else if (currentState == AppState::SAVE_SCREEN) DrawSaveScreen(window, font, isNaming, currentInputName, clock);
        else if (currentState == AppState::SETTINGS_SCREEN) DrawSettings(window, font, boardSize, ruleBlock2, aiLevel, sfxVolume, bgmEnabled);
        else if (currentState == AppState::ABOUT_SCREEN) DrawAbout(window, font); // Vẽ màn About
        else if (currentState == AppState::IN_GAME_SCREEN)
        {
            DrawBoard(window, boardSize);
            DrawPieces(window, boardSize);

            if (gameStatus == 0 && (gameMode == GameMode::PVP || isPlayerTurn))
            {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                DrawHoverEffect(window, mp.x, mp.y, boardSize);
            }

            if (gameStatus != 0) DrawWinLine(window, winX1, winY1, winX2, winY2, boardSize);

            DrawInGamePanel(window, font, timeRemaining, isPlayerTurn, gameStatus, boardSize, gameMode, undoLeft, saveNotifTimer);
        }

        window.display();
    }

    return 0;
}