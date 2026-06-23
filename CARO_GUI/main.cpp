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
#include <fstream>
#include <string>
#include <filesystem>

static std::string HintStatePath_Main(int slotId)
{
    return "save_slot_" + std::to_string(slotId) + "_hint.bin";
}

static void SaveHintState(int slotId, const int hintLeft[2])
{
    if (slotId < 1 || slotId > 5) return;

    std::ofstream file(HintStatePath_Main(slotId), std::ios::out | std::ios::binary);
    if (!file.is_open()) return; 

    const char magic[8] = { 'H','I','N','T','S','A','V','\0' };
    int version = 1;

    file.write(magic, sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(hintLeft), sizeof(int) * 2);
}

static void ResizeView(const sf::Window& window, sf::View& view) {
    float windowRatio = window.getSize().x / (float)window.getSize().y;
    float viewRatio = static_cast<float>(Config::WIN_WIDTH) / static_cast<float>(Config::WIN_HEIGHT);
    float sizeX = 1;
    float sizeY = 1;
    float posX = 0;
    float posY = 0;

    bool horizontalSpacing = windowRatio > viewRatio;

    if (horizontalSpacing) 
    {
        sizeX = viewRatio / windowRatio;
        posX = (1 - sizeX) / 2.f;
    }
    else 
    {
        sizeY = windowRatio / viewRatio;
        posY = (1 - sizeY) / 2.f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode(Config::WIN_WIDTH, Config::WIN_HEIGHT),
        "Caro Master",
        sf::Style::Default

    );
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    sf::View view(sf::FloatRect(0.f, 0.f, static_cast<float>(Config::WIN_WIDTH), static_cast<float>(Config::WIN_HEIGHT)));
    window.setView(view);

    sf::Font font;
    if (!font.loadFromFile("assets/Rajdhani.ttf"))
    {
        std::cout << "Error: Cannot find Rajdhani.ttf" << std::endl;
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
        std::cout << "Error: Cannot find cyberpunk.png" << std::endl;
    }
    sf::Sprite menuBgSprite(menuBgTex);
    menuBgSprite.setScale(
        static_cast<float>(Config::WIN_WIDTH) / menuBgTex.getSize().x,
        static_cast<float>(Config::WIN_HEIGHT) / menuBgTex.getSize().y
    );

    sf::Texture charSelBgTex;
    if (!charSelBgTex.loadFromFile("assets/NenChonCharacter.jpg"))
    {
        std::cout << "Error: Cannot find NenChonCharacter.jpg" << std::endl;
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
            std::cout << "Error: Cannot find " << charFiles[i] << std::endl;
        }
        charSprites[i].setTexture(charTex[i]);
    }

    AppState currentState = AppState::MENU_SCREEN;
    GameMode gameMode = GameMode::PVE;

    int   boardSize = 15;
    bool  isPlayerTurn = true;
    float timeRemaining = 60.f;
    int   gameStatus = 0;

    bool  isRecording = false; 
    bool  hasSavedReplay = false; 
    bool isReplaying = false; 
    float replayTimer = 0.f;

    bool  ruleBlock2 = true;
    int   aiLevel = 3;
    float sfxVolume = 100.f;
    bool  bgmEnabled = true;
    bool  virusMode = false;

    // Biến cho con trỏ 
    int p1CursorX = boardSize / 2;
    int p1CursorY = boardSize / 2;
    int p2CursorX = boardSize / 2;
    int p2CursorY = boardSize / 2;

    int winX1 = -1;
    int winY1 = -1;
    int winX2 = -1;
    int winY2 = -1;

    int hintX = -1;
    int hintY = -1;
    int hintLeft[2] = { 1, 1 };

    // ── Trạng thái Undo (PVP) ────────────────────────────────-
    int undoLeft[2] = { Config::UNDO_MAX, Config::UNDO_MAX };
    int lastUndoPlayer = -1;
    float saveNotifTimer = 0.f;
    bool isConfirmMainMenu = false;
    bool isConfirmNewGame = false;

    bool isNaming = false;
    int selectedSlotToSave = -1;
    std::string currentInputName = "";
    int currentLoadedSlot = -1;
    std::string currentLoadedName = "";
    bool isConfirmOverwrite = false;
    int slotToOverwrite = -1;

    bool isPaused = false;
    bool settingsFromPause = false;

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

            if (event.type == sf::Event::Resized)
            {
                ResizeView(window, view);
                window.setView(view);
            }

            // ── XỬ LÝ GÕ PHÍM (Bấm Enter chuyển bước) ─────────
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::F11)
                {
                    static bool isFullscreen = false;
                    isFullscreen = !isFullscreen;

                    if (isFullscreen) {
                        // Chuyển sang Fullscreen với độ phân giải của Desktop hiện tại
                        window.create(sf::VideoMode::getDesktopMode(), "Caro Master", sf::Style::Fullscreen);
                    }
                    else {
                        // Trở về chế độ cửa sổ với kích thước gốc của cấu hình
                        window.create(sf::VideoMode(Config::WIN_WIDTH, Config::WIN_HEIGHT), "Caro Master", sf::Style::Default);
                    }

                    // Bắt buộc phải thiết lập lại các thuộc tính này sau khi gọi window.create()
                    window.setFramerateLimit(60);
                    window.setKeyRepeatEnabled(false);

                    // Cập nhật lại View ngay lập tức cho cửa sổ mới
                    ResizeView(window, view);
                    window.setView(view);
                }
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
                        isRecording = false; 
                        hasSavedReplay = false; 
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
                sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);

                int mx = static_cast<int>(worldPos.x);
                int my = static_cast<int>(worldPos.y);

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

                                isRecording = false; 
                                hasSavedReplay = false; 
                            }
                        }
                    }
                }
                else if (event.mouseButton.button == sf::Mouse::Left)
                {
                    if (isPaused && currentState == AppState::IN_GAME_SCREEN && !isConfirmMainMenu && !isConfirmNewGame)
                    {
                        const float btnW = 280.f;
                        const float btnH = 50.f;
                        const float gapY = 13.f;
                        const float W = static_cast<float>(Config::WIN_WIDTH);
                        const float H = static_cast<float>(Config::WIN_HEIGHT);
                        const float boxH = 405.f;
                        const float boxY = std::max(24.f, H / 2.f - boxH / 2.f - 4.f);
                        const float startX = W / 2.f - btnW / 2.f;
                        const float startY = boxY + 118.f;

                        for (int i = 0; i < 4; ++i)
                        {
                            float by = startY + i * (btnH + gapY);
                            if (mx >= startX && mx <= startX + btnW && my >= by && my <= by + btnH)
                            {
                                if (i == 0) // RESUME
                                {
                                    isPaused = false;
                                }
                                else if (i == 1) // NEW GAME
                                {
                                    isConfirmNewGame = true;
                                }
                                else if (i == 2) // SETTINGS
                                {
                                    settingsFromPause = true;
                                    isPaused = false;
                                    currentState = AppState::SETTINGS_SCREEN;
                                }
                                else if (i == 3) // MAIN MENU
                                {
                                    isConfirmMainMenu = true;
                                }
                                break;
                            }
                        }
                        continue;
                    }

                    if (isConfirmNewGame)
                    {
                        const float btnW = 190.f;
                        const float btnH = 55.f;
                        const float gap = 34.f;
                        float W = static_cast<float>(Config::WIN_WIDTH);
                        float H = static_cast<float>(Config::WIN_HEIGHT);
                        float boxH = 250.f;
                        float boxY = H / 2.f - boxH / 2.f;
                        float yesX = W / 2.f - btnW - gap / 2.f;
                        float noX = W / 2.f + gap / 2.f;
                        float btnY = boxY + 160.f;


                        if (mx >= yesX && mx <= yesX + btnW && my >= btnY && my <= btnY + btnH)
                        {
                            InitGame(boardSize, ruleBlock2, aiLevel);
                            SetVirusMode(virusMode);
                            isConfirmNewGame = false;
                            isConfirmMainMenu = false;
                            isConfirmNewGame = false;
                            isPaused = false;
                            settingsFromPause = false;

                            isPlayerTurn = true;
                            timeRemaining = 60.f;
                            gameStatus = 0;
                            winX1 = winY1 = winX2 = winY2 = -1;
                            hintX = -1;
                            hintY = -1;
                            hintLeft[0] = 1;
                            hintLeft[1] = 1;
                            undoLeft[0] = Config::UNDO_MAX;
                            undoLeft[1] = Config::UNDO_MAX;
                            isRecording = false; 
                            hasSavedReplay = false; 
                            lastUndoPlayer = -1;
                            saveNotifTimer = 0.f;
                            currentLoadedSlot = -1;
                            currentLoadedName.clear();
                            p1CursorX = boardSize / 2;
                            p1CursorY = boardSize / 2;
                            p2CursorX = boardSize / 2;
                            p2CursorY = boardSize / 2;
                        }
                        else if (mx >= noX && mx <= noX + btnW && my >= btnY && my <= btnY + btnH)
                        {
                            isConfirmNewGame = false;
                        }
                        continue;
                    }

                    if (isConfirmMainMenu)
                    {
                        const float btnW = 190.f;
                        const float btnH = 55.f;
                        const float gap = 34.f;
                        float W = static_cast<float>(Config::WIN_WIDTH);
                        float H = static_cast<float>(Config::WIN_HEIGHT);
                        float boxH = 250.f;
                        float boxY = H / 2.f - boxH / 2.f;
                        float yesX = W / 2.f - btnW - gap / 2.f;
                        float noX = W / 2.f + gap / 2.f;
                        float btnY = boxY + 160.f;

                        if (mx >= yesX && mx <= yesX + btnW && my >= btnY && my <= btnY + btnH)
                        {
                            isConfirmMainMenu = false;
                            isConfirmNewGame = false;
                            isPaused = false;
                            currentState = AppState::MENU_SCREEN;
                            settingsFromPause = false;
                            hintX = -1;
                            hintY = -1;
                            saveNotifTimer = 0.f;
                        }
                        else if (mx >= noX && mx <= noX + btnW && my >= btnY && my <= btnY + btnH)
                        {
                            isConfirmMainMenu = false;
                        }
                        continue;
                    }

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
                            SetVirusMode(virusMode);
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
                        hintLeft[0] = 1;
                        hintLeft[1] = 1;
                        lastUndoPlayer = -1;
                        saveNotifTimer = 0.f;
                        isConfirmMainMenu = false;
                        isConfirmNewGame = false;
                        isPaused = false;
                    }
                    else if (currentState == AppState::LOAD_SCREEN)
                    {
                        AppState beforeLoadState = currentState;

                        HandleLoadInput(
                            window, mx, my,
                            currentState,
                            timeRemaining,
                            isPlayerTurn,
                            gameStatus,
                            errSound,
                            currentLoadedSlot,
                            currentLoadedName,
                            gameMode,
                            hintLeft
                        );

                        // Save cũ chưa lưu avatar/tên người chơi nên sau khi load phải gán lại mặc định.
                        // Nếu không gán, p1Char/p2Char có thể vẫn là -1 làm khung nhân vật bị trống.
                        if (beforeLoadState == AppState::LOAD_SCREEN &&
                            currentState == AppState::IN_GAME_SCREEN)
                        {
                            if (p1Char == -1) p1Char = 0;
                            if (p2Char == -1) p2Char = 1;
                            if (p1Name.empty())
                            {
                                p1Name = currentLoadedName.empty() ? "PLAYER 1" : currentLoadedName;
                            }

                            if (gameMode == GameMode::PVE)
                            {
                                p2Name = "AI";
                            }
                            else
                            {
                                if (p2Name.empty()) p2Name = "PLAYER 2";
                            }

                            // Đưa trạng thái chọn nhân vật về trạng thái đã hoàn tất.
                            selectionStep = 4;
                            typingState = 0;
                            animatingCharIdx = -1;
                            virusMode = IsVirusMode();
                        }
                    }
                    else if (currentState == AppState::LOAD_REPLAY_SCREEN)
                    {
                        HandleLoadReplayInput(window, mx, my, currentState, isReplaying, replayTimer, errSound, p1Char, p2Char, p1Name, p2Name);
                        
                        if (currentState == AppState::IN_GAME_SCREEN)
                        {
                            selectionStep = 4;

                            gameStatus = 0;           
                            timeRemaining = 60.f;     
                            isPlayerTurn = true;      
                            
                            winX1 = -1;
                            winY1 = -1;
                            winX2 = -1;
                            winY2 = -1;
                        }
                    }
                    else if (currentState == AppState::SAVE_SCREEN)
                    {
                        HandleSaveInput(window, mx, my, currentState, timeRemaining, isPlayerTurn, saveNotifTimer, errSound, isNaming, selectedSlotToSave, currentInputName, currentLoadedSlot, currentLoadedName, gameMode, isConfirmOverwrite, slotToOverwrite, hintLeft);
                    }
                    else if (currentState == AppState::IN_GAME_SCREEN)
                    {
                        if (!isReplaying) {
                            HandleInGameInput(
                                mx, my,
                                currentState, boardSize, gameMode,
                                isPlayerTurn, gameStatus, timeRemaining, undoLeft,
                                lastUndoPlayer, saveNotifTimer, errSound,
                                currentLoadedSlot, currentLoadedName,
                                ruleBlock2, aiLevel,
                                hintX, hintY, hintLeft,
                                isConfirmMainMenu, isRecording
                            );
                        }                 
                    }
                    else if (currentState == AppState::SETTINGS_SCREEN)
                    {
                        AppState beforeSettings = currentState;
                        HandleSettingsInput(
                            mx, my, currentState,
                            boardSize, ruleBlock2, aiLevel,
                            sfxVolume, bgmEnabled, virusMode, errSound, settingsFromPause
                        );
                        if (bgmEnabled) bgMusic.play();
                        else            bgMusic.pause();

                        // Neu vao Settings tu Pause Menu thi nut quay lai se dua ve van dang choi,
                        // khong quay thang ra Main Menu.
                        if (settingsFromPause && currentState == AppState::MENU_SCREEN)
                        {
                            currentState = AppState::IN_GAME_SCREEN;
                            isPaused = true;
                            settingsFromPause = false;
                        }
                    }
                    else if (currentState == AppState::ABOUT_SCREEN) // Xử lý nhấn chuột màn About
                    {
                        HandleAboutInput(mx, my, currentState, errSound);
                    }

                }
            }
            if (event.type == sf::Event::KeyPressed && currentState == AppState::IN_GAME_SCREEN && !isConfirmMainMenu && !isConfirmNewGame)
            {
                if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::P)
                {
                    isPaused = !isPaused;
                    continue;
                }

                if (isPaused || isReplaying)
                {
                    continue;
                }

                if (isPlayerTurn)
                {
                    if (event.key.code == sf::Keyboard::W)
                    {
                        p1CursorY = std::max(0, p1CursorY - 1);
                    }
                    else if (event.key.code == sf::Keyboard::S)
                    {
                        p1CursorY = std::min(boardSize - 1, p1CursorY + 1);
                    }
                    else if (event.key.code == sf::Keyboard::A)
                    {
                        p1CursorX = std::max(0, p1CursorX - 1);
                    }
                    else if (event.key.code == sf::Keyboard::D)
                    {
                        p1CursorX = std::min(boardSize - 1, p1CursorX + 1);
                    }
                    else if (event.key.code == sf::Keyboard::Space)
                    {
                        if (gameStatus == 0 && !IsAIThinking())
                        {
                            int res = ProcessMove(p1CursorX, p1CursorY, 1);
                            if (res == -1)
                            {
                                errSound.play();
                            }
                            else
                            {
                                gameStatus = res;
                                timeRemaining = 60.f;
                                lastUndoPlayer = -1;
                                hintX = -1;
                                hintY = -1;
                                isPlayerTurn = false;
                                if (gameMode == GameMode::PVE && gameStatus == 0)
                                {
                                    StartAIThinking();
                                }
                            }
                        }

                    }

                }

                else if (gameMode == GameMode::PVP && !isPlayerTurn)
                {
                    if (event.key.code == sf::Keyboard::Up)
                    {
                        p2CursorY = std::max(0, p2CursorY - 1);
                    }
                    else if (event.key.code == sf::Keyboard::Down)
                    {
                        p2CursorY = std::min(boardSize - 1, p2CursorY + 1);
                    }
                    else if (event.key.code == sf::Keyboard::Left)
                    {
                        p2CursorX = std::max(0, p2CursorX - 1);
                    }
                    else if (event.key.code == sf::Keyboard::Right)
                    {
                        p2CursorX = std::min(boardSize - 1, p2CursorX + 1);
                    }
                    else if (event.key.code == sf::Keyboard::Enter)
                    {
                        if (gameStatus == 0)
                        {
                            int res = ProcessMove(p2CursorX, p2CursorY, 2);
                            if (res == -1) {
                                errSound.play();
                            }
                            else {
                                gameStatus = res;
                                timeRemaining = 60.f;
                                lastUndoPlayer = -1;
                                hintX = -1;
                                hintY = -1;
                                isPlayerTurn = true; // Trả lại lượt cho Player 1
                            }
                        }
                    }
                }
            }
        }

        if (saveNotifTimer > 0.f)
        {
            saveNotifTimer -= dt;
        }

        if (!isConfirmMainMenu && !isConfirmNewGame && !isPaused)
        {
            UpdateAI();
        }
        if (isReplaying && !isPaused && !isConfirmMainMenu) {
            replayTimer += dt;
            if (replayTimer >= 3.0f) {
                replayTimer = 0.f;
                if (ProcessNextReplayMove()) {
                    gameStatus = EvaluateBoard(); 
                }
            }
        }
        if (currentState == AppState::IN_GAME_SCREEN && gameStatus == 0 && !isConfirmMainMenu && !isConfirmNewGame && !isPaused && !isReplaying)
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
                    if (gameStatus != 0)
                    {
                        GetWinLine(&winX1, &winY1, &winX2, &winY2);

                    }
                    isPlayerTurn = true;
                    hintX = -1;
                    hintY = -1;
                    timeRemaining = 60.f;
                }
            }
        }

        // --- FIX LỖI KHỰNG (CHỈ TÍNH WINLINE 1 LẦN) ---
        if (gameStatus != 0)
        {
            if (winX1 == -1) GetWinLine(&winX1, &winY1, &winX2, &winY2);

            if (isRecording && !hasSavedReplay)
            {
                std::time_t t = std::time(nullptr);
                std::string filename = "replay_" + std::to_string(t) + ".rep";

                SaveGameReplay(filename);

                hasSavedReplay = true; 
            }
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
        else if (currentState == AppState::LOAD_REPLAY_SCREEN)
        {
            DrawLoadReplayScreen(window, font);
        }
        else if (currentState == AppState::SAVE_SCREEN)
        {
            DrawSaveScreen(window, font, isNaming, currentInputName, clock, isConfirmOverwrite);
        }
        else if (currentState == AppState::SETTINGS_SCREEN)
        {
            DrawSettings(window, font, boardSize, ruleBlock2, aiLevel, sfxVolume, bgmEnabled, virusMode, settingsFromPause);
        }
        else if (currentState == AppState::IN_GAME_SCREEN)
        {

            window.draw(menuBgSprite);
            DrawBoard(window, boardSize);
            DrawPieces(window, boardSize);
            DrawVirusCells(window, font, boardSize);
            if (gameStatus == 0)
            {
                DrawVirusStatusBadge(window, font);
            }

            if (gameStatus == 0 && hintX != -1 && hintY != -1)
            {
                DrawHintEffect(window, font, hintX, hintY, boardSize);
            }

            bool showHover = (gameStatus == 0) && (gameMode == GameMode::PVP || isPlayerTurn);
            if (showHover)
            {
                int cellSz = GetDynCellSize(boardSize);

                if (isPlayerTurn)
                {
                    DrawHoverEffect(window, p1CursorX, p1CursorY, boardSize);
                }
                else if (gameMode == GameMode::PVP && !isPlayerTurn)
                {
                    DrawHoverEffect(window, p2CursorX, p2CursorY, boardSize);
                }
            }

            if (gameStatus != 0) DrawWinLine(window, winX1, winY1, winX2, winY2, boardSize);
            // TRUYỀN THÊM p1Char, p2Char, p1Name, p2Name, charSprites VÀO ĐÂY:
            DrawInGamePanel(window, font, timeRemaining, isPlayerTurn, gameStatus, boardSize, gameMode, undoLeft, hintLeft, saveNotifTimer, p1Char, p2Char, p1Name, p2Name, charSprites, isRecording);

            if (isPaused)
            {
                DrawPauseOverlay(window, font);
            }

            if (isConfirmMainMenu)
            {
                DrawConfirmMainMenuOverlay(window, font);
                isReplaying = false; 
            }

            if (isConfirmNewGame)
            {
                DrawConfirmNewGameOverlay(window, font);
                isReplaying = false; 
            }
        }
        else if (currentState == AppState::ABOUT_SCREEN)
        {
            DrawAbout(window, font);
        }

        window.display();
    }

    return 0;
}