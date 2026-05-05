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
    if (!font.loadFromFile("assets/Rajdhani.ttf"))
    {
        std::cout << "Loi: Khong tim thay Rajdhani.ttf" << std::endl;
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

    sf::Texture menuBgTex;
    if (!menuBgTex.loadFromFile("assets/cyberpunk.png"))
    {
        std::cout << "Loi: Khong tim thay cyberpunk.png" << std::endl;
    }
    sf::Sprite menuBgSprite(menuBgTex);
    menuBgSprite.setScale(
        static_cast<float>(Config::WIN_WIDTH) / menuBgTex.getSize().x,
        static_cast<float>(Config::WIN_HEIGHT) / menuBgTex.getSize().y
    );

    sf::Texture charSelBgTex;
    if (!charSelBgTex.loadFromFile("assets/NenChonCharacter.jpg"))
    {
        std::cout << "Loi: Khong tim thay NenChonCharacter.jpg" << std::endl;
    }
    sf::Sprite charSelBgSprite(charSelBgTex);
    charSelBgSprite.setScale(
        static_cast<float>(Config::WIN_WIDTH) / charSelBgTex.getSize().x,
        static_cast<float>(Config::WIN_HEIGHT) / charSelBgTex.getSize().y
    );

    sf::Texture charTex[4];
    sf::Sprite charSprites[4];
    std::string charFiles[4] = { "assets/Player1.jpg", "assets/Player2.jpg", "assets/Player3.jpg", "assets/Player4.jpg" };

    for (int i = 0; i < 4; ++i) {
        if (!charTex[i].loadFromFile(charFiles[i])) {
            std::cout << "Loi: Khong tim thay " << charFiles[i] << std::endl;
        }
        charSprites[i].setTexture(charTex[i]);
    }

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

    int undoLeft[2] = { Config::UNDO_MAX, Config::UNDO_MAX };
    int lastUndoPlayer = -1;
    float saveNotifTimer = 0.f;

    bool isNaming = false;
    int selectedSlotToSave = -1;
    std::string currentInputName = "";
    int currentLoadedSlot = -1;
    std::string currentLoadedName = "";
    bool isConfirmOverwrite = false;
    int slotToOverwrite = -1;

    int p1Char = -1;
    int p2Char = -1;
    std::string p1Name = "";
    std::string p2Name = "";
    int typingState = 0;
    int selectionStep = 0;

    int animatingCharIdx = -1;
    sf::Clock confirmAnimClk;

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

            // ── XỬ LÝ GÕ PHÍM (Bấm Enter chuyển bước) ─────────
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Enter && currentState == AppState::CHAR_SELECT) {
                    if (selectionStep == 0 && p1Char != -1) {
                        selectionStep = 1;
                        typingState = 1; // PVE và PVP đều mở nhập tên

                        animatingCharIdx = p1Char;
                        confirmAnimClk.restart();
                    }
                    else if (selectionStep == 1 && !p1Name.empty()) {
                        // [SỬA LOGIC PVE ĐẶT TÊN]
                        if (gameMode == GameMode::PVP) {
                            selectionStep = 2; // Qua P2 chọn hình
                            typingState = 0;
                        }
                        else {
                            selectionStep = 4; // Nhảy thẳng ra sẵn sàng luôn vì PVE ko có P2
                            typingState = 0;
                        }
                    }
                    else if (selectionStep == 2 && p2Char != -1) {
                        selectionStep = 3;
                        typingState = 2;

                        animatingCharIdx = p2Char;
                        confirmAnimClk.restart();
                    }
                    else if (selectionStep == 3 && !p2Name.empty()) {
                        selectionStep = 4;
                        typingState = 0;
                    }
                    else if (selectionStep == 4) {
                        currentState = AppState::IN_GAME_SCREEN;
                    }
                }
            }

            // ── XỬ LÝ GÕ CHỮ ĐẶT TÊN ────────────────────────
            if (event.type == sf::Event::TextEntered)
            {
                if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    // Bo qua Enter
                }
                else if (isNaming && !isConfirmOverwrite && currentState == AppState::SAVE_SCREEN) {
                    if (event.text.unicode == '\b') {
                        if (!currentInputName.empty()) currentInputName.pop_back();
                    }
                    else if (event.text.unicode < 128 && event.text.unicode >= 32 && currentInputName.size() < 25) {
                        currentInputName += static_cast<char>(event.text.unicode);
                    }
                }
                else if (currentState == AppState::CHAR_SELECT) {
                    if (typingState == 1 || typingState == 2) {
                        std::string& curName = (typingState == 1) ? p1Name : p2Name;
                        if (event.text.unicode == '\b') {
                            if (!curName.empty()) curName.pop_back();
                        }
                        else if (event.text.unicode < 128 && event.text.unicode >= 32 && curName.size() < 12) {
                            curName += static_cast<char>(event.text.unicode);
                        }
                    }
                }
            }

            // ── XỬ LÝ CLICK CHUỘT ──────────────────────────────
            if (event.type == sf::Event::MouseButtonPressed)
            {
                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                if (currentState == AppState::CHAR_SELECT)
                {
                    float W = static_cast<float>(Config::WIN_WIDTH);
                    float H = static_cast<float>(Config::WIN_HEIGHT);

                    if (event.mouseButton.button == sf::Mouse::Left) {
                        const float BOX_W = 220.f, BOX_H = 300.f, SPACING = 30.f;
                        const float TOTAL_W = 4 * BOX_W + 3 * SPACING;
                        const float START_X = W / 2.f - TOTAL_W / 2.f;
                        const float BOX_Y = 160.f;

                        for (int i = 0; i < 4; ++i) {
                            float cX = START_X + i * (BOX_W + SPACING);
                            if (mx >= cX && mx <= cX + BOX_W && my >= BOX_Y && my <= BOX_Y + BOX_H) {
                                if (selectionStep == 0) p1Char = i;
                                else if (selectionStep == 2) p2Char = i;
                            }
                        }

                        // [SỬA BẮT TỌA ĐỘ THEO PVE / PVP]
                        float NAME_Y = 520.f, NAME_W = 350.f, NAME_H = 60.f;

                        if (gameMode == GameMode::PVP) {
                            float P1_X = W / 2.f - NAME_W - 30.f;
                            float P2_X = W / 2.f + 30.f;
                            if (mx >= P1_X && mx <= P1_X + NAME_W && my >= NAME_Y && my <= NAME_Y + NAME_H) {
                                typingState = 1; selectionStep = 1;
                            }
                            else if (mx >= P2_X && mx <= P2_X + NAME_W && my >= NAME_Y && my <= NAME_Y + NAME_H) {
                                if (selectionStep >= 2) { typingState = 2; selectionStep = 3; }
                            }
                        }
                        else {
                            float P1_X = W / 2.f - NAME_W / 2.f;
                            if (mx >= P1_X && mx <= P1_X + NAME_W && my >= NAME_Y && my <= NAME_Y + NAME_H) {
                                typingState = 1; selectionStep = 1;
                            }
                        }

                        float btnX = W / 2.f - 150.f, btnY = 640.f;
                        if (mx >= btnX && mx <= btnX + 300.f && my >= btnY && my <= btnY + 70.f) {
                            if (selectionStep == 4) {
                                currentState = AppState::IN_GAME_SCREEN;
                            }
                        }
                    }
                }
                else if (event.mouseButton.button == sf::Mouse::Left)
                {
                    if (currentState == AppState::MENU_SCREEN)
                    {
                        HandleMenuInput(
                            window, mx, my, currentState,
                            gameMode,
                            boardSize, ruleBlock2, aiLevel,
                            timeRemaining, isPlayerTurn, gameStatus,
                            errSound, currentLoadedSlot, currentLoadedName
                        );

                        if (currentState == AppState::IN_GAME_SCREEN) {
                            currentState = AppState::CHAR_SELECT;
                            p1Char = -1; p2Char = -1;
                            p1Name = ""; p2Name = "";
                            selectionStep = 0;
                            typingState = 0;
                            animatingCharIdx = -1;
                        }

                        winX1 = winY1 = winX2 = winY2 = -1;
                        undoLeft[0] = Config::UNDO_MAX;
                        undoLeft[1] = Config::UNDO_MAX;
                        lastUndoPlayer = -1;
                        saveNotifTimer = 0.f;
                    }
                    else if (currentState == AppState::LOAD_SCREEN)
                    {
                        HandleLoadInput(window, mx, my, currentState, timeRemaining, isPlayerTurn, gameStatus, errSound, currentLoadedSlot, currentLoadedName);
                    }
                    else if (currentState == AppState::SAVE_SCREEN)
                    {
                        HandleSaveInput(window, mx, my, currentState, timeRemaining, isPlayerTurn, saveNotifTimer, errSound, isNaming, selectedSlotToSave, currentInputName, currentLoadedSlot, currentLoadedName, isConfirmOverwrite, slotToOverwrite);
                    }
                    else if (currentState == AppState::IN_GAME_SCREEN)
                    {
                        HandleInGameInput(
                            mx, my, currentState,
                            boardSize, gameMode,
                            isPlayerTurn, gameStatus, timeRemaining,
                            undoLeft, lastUndoPlayer, saveNotifTimer,
                            errSound, currentLoadedSlot,
                            currentLoadedName
                        );
                    }
                    else if (currentState == AppState::SETTINGS_SCREEN)
                    {
                        HandleSettingsInput(
                            mx, my, currentState,
                            boardSize, ruleBlock2, aiLevel,
                            sfxVolume, bgmEnabled, errSound
                        );
                        if (bgmEnabled) bgMusic.play();
                        else            bgMusic.pause();
                    }
                }
            }
        }

        if (saveNotifTimer > 0.f)
        {
            saveNotifTimer -= dt;
        }

        UpdateAI();
        if (currentState == AppState::IN_GAME_SCREEN && gameStatus == 0)
        {
            timeRemaining -= dt;
            if (timeRemaining <= 0.f)
            {
                gameStatus = isPlayerTurn ? 2 : 1;
            }

            if (gameMode == GameMode::PVE && !isPlayerTurn && !IsAIThinking())
            {
                int aiX = -1;
                int aiY = -1;
                int result = GetAIResult(&aiX, &aiY);

                if (aiX != -1)
                {
                    gameStatus = result;
                    if (gameStatus != 0)
                    {
                        GetWinLine(&winX1, &winY1, &winX2, &winY2);
                    }
                    isPlayerTurn = true;
                    timeRemaining = 60.f;
                }
            }
        }

        if (gameStatus != 0 && winX1 == -1)
        {
            GetWinLine(&winX1, &winY1, &winX2, &winY2);
        }
        else if (gameStatus == 0) {
            winX1 = -1; winY1 = -1; winX2 = -1; winY2 = -1;
        }

        window.clear(BG_COLOR);

        if (currentState == AppState::MENU_SCREEN)
        {
            DrawMenu(window, font, menuBgSprite);
        }
        else if (currentState == AppState::CHAR_SELECT)
        {
            window.draw(charSelBgSprite);
            DrawCharacterSelectScreen(window, font, gameMode == GameMode::PVP, p1Char, p2Char, p1Name, p2Name, typingState, selectionStep, charSprites, animatingCharIdx, confirmAnimClk);
        }
        else if (currentState == AppState::LOAD_SCREEN)
        {
            DrawLoadScreen(window, font);
        }
        else if (currentState == AppState::SAVE_SCREEN)
        {
            DrawSaveScreen(window, font, isNaming, currentInputName, clock, isConfirmOverwrite);
        }
        else if (currentState == AppState::SETTINGS_SCREEN)
        {
            DrawSettings(window, font, boardSize, ruleBlock2, aiLevel, sfxVolume, bgmEnabled);
        }
        else if (currentState == AppState::IN_GAME_SCREEN)
        {
            window.draw(menuBgSprite);
            DrawBoard(window, boardSize);
            DrawPieces(window, boardSize);

            bool showHover = (gameStatus == 0) && (gameMode == GameMode::PVP || isPlayerTurn);
            if (showHover)
            {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                DrawHoverEffect(window, mp.x, mp.y, boardSize);
            }

            if (gameStatus != 0)
            {
                DrawWinLine(window, winX1, winY1, winX2, winY2, boardSize);
            }

            // TRUYỀN THÊM p1Char, p2Char, p1Name, p2Name, charSprites VÀO ĐÂY:
            DrawInGamePanel(window, font, timeRemaining, isPlayerTurn, gameStatus, boardSize, gameMode, undoLeft, saveNotifTimer, p1Char, p2Char, p1Name, p2Name, charSprites);
        }

        window.display();
    }

    return 0;
}