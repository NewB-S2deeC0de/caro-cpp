#include "InputUI.h"
#include "CaroAPI.h"
#include <algorithm>
#include <fstream>
#include <cstdio>

extern int gLoadPreviewSlotUI;

// ============================================================
//  Helper nội bộ
// ============================================================
static float PanelX(int boardSize)
{
    int   cellSz = GetDynCellSize(boardSize);
    float boardRight = static_cast<float>(Config::OFFSET_X + boardSize * cellSz);
    float gapWidth = static_cast<float>(Config::WIN_WIDTH) - boardRight;
    return boardRight + (gapWidth - static_cast<float>(Config::PANEL_W)) / 2.0f;
}


static std::string HintStatePath(int slotId)
{
    return "save_slot_" + std::to_string(slotId) + "_hint.bin";
}

static void SaveHintState(int slotId, const int hintLeft[2])
{
    if (slotId < 1 || slotId > 5) return;

    std::ofstream file(HintStatePath(slotId), std::ios::out | std::ios::binary);
    if (!file.is_open()) return;

    const char magic[8] = { 'H','I','N','T','S','A','V','\0' };
    int version = 1;
    file.write(magic, sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(hintLeft), sizeof(int) * 2);
}

static void LoadHintState(int slotId, int hintLeft[2])
{
    hintLeft[0] = 1;
    hintLeft[1] = 1;

    if (slotId < 1 || slotId > 5) return;

    std::ifstream file(HintStatePath(slotId), std::ios::in | std::ios::binary);
    if (!file.is_open()) return; // Save cu: mac dinh moi nguoi con 1 hint.

    char magic[8] = {};
    int version = 0;
    int savedHint[2] = { 1, 1 };

    file.read(magic, sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(savedHint), sizeof(int) * 2);

    if (std::string(magic) == "HINTSAV" && version == 1) {
        hintLeft[0] = std::max(0, std::min(1, savedHint[0]));
        hintLeft[1] = std::max(0, std::min(1, savedHint[1]));
    }
}

static void DeleteHintState(int slotId)
{
    if (slotId < 1 || slotId > 5) return;
    std::remove(HintStatePath(slotId).c_str());
}

// ============================================================
//  HandleMenuInput
// ============================================================
void HandleMenuInput(
    sf::RenderWindow& window,
    int mouseX, int mouseY,
    AppState& currentState,
    GameMode& gameMode,
    int boardSize, bool ruleBlock2, int aiLevel,
    float& timeRemaining, bool& isPlayerTurn, int& gameStatus,
    sf::Sound& errSound, int& currentLoadedSlot,
    std::string& currentLoadedName)
{
    const float BTN_W = 350.f;
    const float BTN_H = 60.f;
    const float BTN_X = Config::WIN_WIDTH / 2.f - BTN_W / 2.f;

    for (int i = 0; i < 6; ++i)
    {
        float bY = 300.f + i * 80.f;

        if (mouseX >= BTN_X && mouseX <= BTN_X + BTN_W &&
            mouseY >= bY && mouseY <= bY + BTN_H)
        {
            if (i == 0 || i == 1)
            {
                gameMode = (i == 0) ? GameMode::PVP : GameMode::PVE;

                InitGame(boardSize, ruleBlock2, aiLevel);
                currentState = AppState::IN_GAME_SCREEN;
                isPlayerTurn = true;
                timeRemaining = 60.f;
                gameStatus = 0;

                // RESET: Game mới nên không nhớ slot nào hết
                currentLoadedSlot = -1;
                currentLoadedName = "";
            }
            else if (i == 2)
            {
                currentState = AppState::SETTINGS_SCREEN;
            }
            else if (i == 3)
            {
                currentState = AppState::LOAD_SCREEN;
            }
            else if (i == 4) // Chuyển sang màn hình About[cite: 2]
            {
                currentState = AppState::ABOUT_SCREEN;
            }
            else if (i == 5) // Nút thoát được đẩy xuống cuối[cite: 2]
            {
                window.close();
            }
        }
    }
}

// ============================================================
//  HandleInGameInput
//
//  Undo PVP:
//    - Mỗi người có Config::UNDO_MAX lượt (mặc định 3)
//    - Không được dùng liên tiếp: sau khi dùng undo phải đi 1 nc
//      thực sự trước khi được undo tiếp
//    - Undo cho lùi 1 nước của chính mình (UndoOneMove)
//    - Sau undo vẫn là lượt của người đó (được đi lại)
//
//  Undo PVE:
//    - Giữ nguyên hành vi cũ: undo theo cặp (AI + người)
//    - Không giới hạn lượt (undoLeft không áp dụng)
// ============================================================
void HandleInGameInput(
    int mouseX, int mouseY,
    AppState& currentState,
    int boardSize, GameMode gameMode,
    bool& isPlayerTurn, int& gameStatus,
    float& timeRemaining, int undoLeft[2],
    int& lastUndoPlayer, float& saveNotifTimer,
    sf::Sound& errSound, int& currentLoadedSlot,
    std::string& currentLoadedName,
    bool ruleBlock2, int aiLevel,
    int& hintX, int& hintY, int hintLeft[2],
    bool& isConfirmMainMenu) // them hintLeft de moi nguoi chi goi y 1 lan
{
    static sf::Clock clickCooldown;
    if (clickCooldown.getElapsedTime().asMilliseconds() < 150) return;
    clickCooldown.restart();

    // =========================================================
    // ── 0. PANEL KẾT QUẢ CHỈ DÙNG ĐỂ HIỂN THỊ, KHÔNG CÓ NÚT RIÊNG ──
    // =========================================================

    // =========================================================
    // ── A. TÍNH TOÁN TỌA ĐỘ BÀN CỜ CANH GIỮA ──
    // =========================================================
    int cellSz = GetDynCellSize(boardSize);
    float boardW = static_cast<float>(boardSize * cellSz);

    float BOARD_LEFT = (static_cast<float>(Config::WIN_WIDTH) - boardW) / 2.f;
    float BOARD_TOP = (static_cast<float>(Config::WIN_HEIGHT) - boardW - 130.f) / 2.f + 20.f;
    float BOARD_RIGHT = BOARD_LEFT + boardW;
    float BOARD_BOTTOM = BOARD_TOP + boardW;

    // Nếu click vào khu vực bàn cờ thì bỏ qua (vì game dùng phím WASD để chơi)
    if (mouseX >= BOARD_LEFT && mouseX <= BOARD_RIGHT &&
        mouseY >= BOARD_TOP && mouseY <= BOARD_BOTTOM)
    {
        return;
    }

    // =========================================================
    // ── B. TỌA ĐỘ 3 NÚT BẤM (Ở BẢNG ĐIỀU KHIỂN DƯỚI BÀN CỜ) ──
    // =========================================================
    float boardCenterX = static_cast<float>(Config::WIN_WIDTH) / 2.f;
    float CARD_H = 340.f;
    float bottomPanelY = std::max(BOARD_BOTTOM, BOARD_TOP + CARD_H) + 20.f;
    float timerH = 35.f;

    const float BTN_W = 170.f;
    const float BTN_H = 50.f;
    const float BTN_GAP = 22.f;
    float totalBtnsW = 3 * BTN_W + 2 * BTN_GAP;
    float startBtnsX = boardCenterX - totalBtnsW / 2.f;

    float btnsY = bottomPanelY + 20.f + timerH + 20.f;

    for (int i = 0; i < 3; ++i)
    {
        float bX = startBtnsX + i * (BTN_W + BTN_GAP);
        if (mouseX >= bX && mouseX <= bX + BTN_W && mouseY >= btnsY && mouseY <= btnsY + BTN_H)
        {
            if (i == 0) // UNDO
            {
                if (gameStatus != 0) { errSound.play(); continue; }
                if (gameMode == GameMode::PVP)
                {
                    int playerIdx = isPlayerTurn ? 0 : 1;
                    if (lastUndoPlayer == playerIdx || undoLeft[playerIdx] <= 0) { errSound.play(); return; }
                    int undone = UndoMove();
                    if (undone == 0) { errSound.play(); return; }
                    undoLeft[playerIdx]--;
                    lastUndoPlayer = playerIdx;
                    gameStatus = 0; timeRemaining = 60.f;
                    if (undone == 1) isPlayerTurn = true;
                }
                else // PVE
                {
                    if (IsAIThinking()) { StartAIThinking(); errSound.play(); return; }
                    int undone = UndoMove();
                    if (undone == 0) { errSound.play(); return; }
                    gameStatus = 0; isPlayerTurn = true; timeRemaining = 60.f;
                }
            }
            else if (i == 1) // HINT
            {
                if (gameStatus != 0 || IsAIThinking() || (gameMode == GameMode::PVE && !isPlayerTurn))
                {
                    errSound.play();
                    return;
                }

                int player = isPlayerTurn ? 1 : 2;
                int playerIdx = player - 1;

                // Moi nguoi chi duoc dung goi y 1 lan trong mot van
                if (hintLeft[playerIdx] <= 0)
                {
                    errSound.play();
                    return;
                }

                int hx = -1;
                int hy = -1;

                if (GetHintMove(player, &hx, &hy))
                {
                    hintX = hx;
                    hintY = hy;
                    hintLeft[playerIdx]--;
                }
                else
                {
                    hintX = -1;
                    hintY = -1;
                    errSound.play();
                }
            }
            else if (i == 2) // SAVE GAME
            {
                if (currentLoadedSlot != -1) {
                    SetSaveGameMode(gameMode == GameMode::PVP ? 2 : 1);
                    if (SaveGameSlot(currentLoadedSlot, timeRemaining, isPlayerTurn ? 1 : 0, currentLoadedName.c_str())) {
                        SaveHintState(currentLoadedSlot, hintLeft);
                        saveNotifTimer = 2.0f;
                    }
                    else errSound.play();
                }
                else currentState = AppState::SAVE_SCREEN;
            }
            return;
        }
    }
}
void HandleAboutInput(int mouseX, int mouseY, AppState& currentState, sf::Sound& errSound)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    // Tọa độ khớp hoàn toàn với RenderUI ở trên
    float bH_frame = 520.f;
    float bY_frame = 160.f;

    const float BW = 250.f, BH = 55.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = bY_frame + bH_frame + 30.f;

    if (mouseX >= BX && mouseX <= BX + BW && mouseY >= BY && mouseY <= BY + BH)
    {
        currentState = AppState::MENU_SCREEN;
    }
}

// ============================================================
//  HandleSettingsInput (không ??i)
// ============================================================
void HandleSettingsInput(
    int mouseX, int mouseY,
    AppState& currentState,
    int& boardSize, bool& ruleBlock2, int& aiLevel,
    float& sfxVolume, bool& bgmEnabled, bool& virusMode,
    sf::Sound& errSound, bool audioOnly)
{
    static sf::Clock clickCooldown;

    float panelW = 900.f;
    float panelH = 540.f;
    float pX = static_cast<float>(Config::WIN_WIDTH) / 2.f - panelW / 2.f;
    float pY = 160.f;

    const float CX = pX + 450.f;
    const float RG = 68.f;
    float startY = pY + 54.f;

    auto handleSlider = [&](float cy, float minV, float maxV, float& target) -> bool {
        float barW = 320.f;
        float barX = CX;
        if (mouseX >= barX && mouseX <= barX + barW && mouseY >= cy - 10.f && mouseY <= cy + 40.f) {
            float ratio = (mouseX - barX) / barW;
            if (ratio < 0.f) ratio = 0.f;
            if (ratio > 1.f) ratio = 1.f;
            target = std::round(minV + ratio * (maxV - minV));
            return true;
        }
        return false;
        };

    auto handleToggle = [&](float cy, bool& target) -> bool {
        float tW = 80.f, tH = 30.f;
        if (mouseX >= CX && mouseX <= CX + tW + 80.f && mouseY >= cy - 10.f && mouseY <= cy + tH + 10.f) {
            if (clickCooldown.getElapsedTime().asMilliseconds() > 200) {
                target = !target;
                clickCooldown.restart();
            }
            return true;
        }
        return false;
        };

    if (audioOnly) {
        // Settings mo tu Pause/Game: hien du cac tuy chon nhung khoa gameplay.
        // Chi cho phep chinh SFX va BGM. Virus Mode cung bi khoa vi tac dong den van co.
        if (handleSlider(startY + 3 * RG, 0.f, 100.f, sfxVolume)) return;
        if (handleToggle(startY + 4 * RG, bgmEnabled)) return;
    }
    else {
        for (int i = 0; i < 6; ++i) {
            float cy = startY + i * RG;

            if (i == 0) {
                float tmp = static_cast<float>(boardSize);
                if (handleSlider(cy, 10.f, 30.f, tmp)) {
                    boardSize = static_cast<int>(tmp);
                    return;
                }
            }
            else if (i == 1) {
                if (handleToggle(cy, ruleBlock2)) return;
            }
            else if (i == 2) {
                float tmp = static_cast<float>(aiLevel);
                if (handleSlider(cy, 1.f, 3.f, tmp)) {
                    aiLevel = static_cast<int>(tmp);
                    return;
                }
            }
            else if (i == 3) {
                if (handleSlider(cy, 0.f, 100.f, sfxVolume)) return;
            }
            else if (i == 4) {
                if (handleToggle(cy, bgmEnabled)) return;
            }
            else if (i == 5) {
                if (handleToggle(cy, virusMode)) return;
            }
        }
    }

    if (clickCooldown.getElapsedTime().asMilliseconds() > 200) {
        float BW = 300.f, BH = 60.f;
        float BX = static_cast<float>(Config::WIN_WIDTH) / 2.f - BW / 2.f;
        float BY = pY + panelH - 76.f;
        if (mouseX >= BX && mouseX <= BX + BW && mouseY >= BY && mouseY <= BY + BH) {
            currentState = AppState::MENU_SCREEN;
            clickCooldown.restart();
            return;
        }
    }
}

void HandleLoadInput(sf::RenderWindow& window, int mouseX, int mouseY, AppState& currentState, float& timeRemaining, bool& isPlayerTurn, int& gameStatus, sf::Sound& errSound, int& currentLoadedSlot, std::string& currentLoadedName, GameMode& gameMode, int hintLeft[2]) {
    const float panelW = 980.f;
    const float panelH = 520.f;
    const float panelX = Config::WIN_WIDTH / 2.f - panelW / 2.f;
    const float panelY = 150.f;

    const float slotX = panelX + 28.f;
    const float slotY = panelY + 58.f;
    const float slotW = 390.f;
    const float slotH = 72.f;
    const float gap = 18.f;

    const float prevX = panelX + 455.f;
    const float prevY = panelY + 58.f;
    const float prevW = 490.f;

    // Nut Xoa nam o goc tren ben phai bang thong tin lon ben phai
    if (gLoadPreviewSlotUI >= 1 && gLoadPreviewSlotUI <= 5) {
        int size = 0, moves = 0, turn = 0;
        char name[64] = "";
        bool hasData = PeekGameSlot(gLoadPreviewSlotUI, &size, &moves, &turn, name);

        const float delW = 34.f;
        const float delH = 24.f;
        const float delX = prevX + prevW - delW - 10.f;
        const float delY = prevY + 10.f;

        if (hasData && mouseX >= delX && mouseX <= delX + delW &&
            mouseY >= delY && mouseY <= delY + delH) {
            if (DeleteGameSlot(gLoadPreviewSlotUI)) {
                DeleteHintState(gLoadPreviewSlotUI);
                if (currentLoadedSlot == gLoadPreviewSlotUI) {
                    currentLoadedSlot = -1;
                    currentLoadedName.clear();
                }

                int nextSlot = -1;
                for (int i = 1; i <= 5; ++i) {
                    int bs = 0, mv = 0, tn = 0;
                    char nm[64] = "";
                    if (PeekGameSlot(i, &bs, &mv, &tn, nm)) {
                        nextSlot = i;
                        break;
                    }
                }
                gLoadPreviewSlotUI = (nextSlot != -1) ? nextSlot : 1;
            }
            else {
                errSound.play();
            }
            return;
        }
    }

    for (int i = 1; i <= 5; ++i) {
        float y = slotY + (i - 1) * (slotH + gap);

        if (mouseX >= slotX && mouseX <= slotX + slotW &&
            mouseY >= y && mouseY <= y + slotH) {
            gLoadPreviewSlotUI = i;

            int size = 0, moves = 0, turn = 0;
            char name[64] = "";
            bool hasData = PeekGameSlot(i, &size, &moves, &turn, name);
            if (!hasData) {
                errSound.play();
                return;
            }

            float tRem = 0.f;
            int turnData = 0;
            if (LoadGameSlot(i, &tRem, &turnData)) {
                timeRemaining = tRem;
                isPlayerTurn = (turnData == 1);
                gameStatus = EvaluateBoard();
                currentLoadedSlot = i;
                currentLoadedName = std::string(name);
                int savedMode = GetSavedGameMode();
                if (savedMode == 2) gameMode = GameMode::PVP;
                else if (savedMode == 1) gameMode = GameMode::PVE;
                LoadHintState(i, hintLeft);
                currentState = AppState::IN_GAME_SCREEN;
            }
            else {
                errSound.play();
            }
            return;
        }
    }

    const float BW = 300.f;
    const float BH = 60.f;
    float BX = Config::WIN_WIDTH / 2.f - BW / 2.f;
    float BY = panelY + panelH + 32.f;

    if (mouseX >= BX && mouseX <= BX + BW &&
        mouseY >= BY && mouseY <= BY + BH) {
        currentState = AppState::MENU_SCREEN;
        return;
    }
}

void HandleSaveInput(
    sf::RenderWindow& window, int mouseX, int mouseY, AppState& currentState,
    float timeRemaining, bool isPlayerTurn, float& saveNotifTimer, sf::Sound& errSound,
    bool& isNaming, int& selectedSlot, std::string& inputName,
    int& currentLoadedSlot, std::string& currentLoadedName,
    GameMode gameMode,
    bool& isConfirmOverwrite, int& slotToOverwrite,
    int hintLeft[2])
{
    const float panelW = 980.f;
    const float panelH = 520.f;
    const float panelX = Config::WIN_WIDTH / 2.f - panelW / 2.f;
    const float panelY = 150.f;

    const float slotX = panelX + 28.f;
    const float slotY = panelY + 58.f;
    const float slotW = 390.f;
    const float slotH = 72.f;
    const float gap = 18.f;

    float w = Config::WIN_WIDTH / 2.f;
    float h = Config::WIN_HEIGHT / 2.f;

    // --- XỬ LÝ CLICK TRÊN HỘP THOẠI XÁC NHẬN GHI ĐÈ ---
    if (isConfirmOverwrite) {
        float btnW = 120.f, btnH = 50.f;
        float yesX = w - 140.f, yesY = h + 30.f;
        float noX = w + 20.f, noY = h + 30.f;

        if (mouseX >= yesX && mouseX <= yesX + btnW && mouseY >= yesY && mouseY <= yesY + btnH) {
            SetSaveGameMode(gameMode == GameMode::PVP ? 2 : 1);
            if (SaveGameSlot(slotToOverwrite, timeRemaining, isPlayerTurn ? 1 : 0, inputName.c_str())) {
                SaveHintState(slotToOverwrite, hintLeft);
                saveNotifTimer = 2.0f;
                currentLoadedSlot = slotToOverwrite;
                currentLoadedName = inputName;
                isConfirmOverwrite = false;
                isNaming = false;
                currentState = AppState::IN_GAME_SCREEN;
            }
            else errSound.play();
        }
        else if (mouseX >= noX && mouseX <= noX + btnW && mouseY >= noY && mouseY <= noY + btnH) {
            isConfirmOverwrite = false;
        }
        return;
    }

    if (!isNaming) {
        for (int i = 1; i <= 5; ++i) {
            float y = slotY + (i - 1) * (slotH + gap);
            if (mouseX >= slotX && mouseX <= slotX + slotW &&
                mouseY >= y && mouseY <= y + slotH) {
                selectedSlot = i;
                isNaming = true;
                inputName = "";
                return;
            }
        }

        const float BW = 300.f;
        const float BH = 60.f;
        float BX = Config::WIN_WIDTH / 2.f - BW / 2.f;
        float BY = panelY + panelH + 32.f;
        if (mouseX >= BX && mouseX <= BX + BW && mouseY >= BY && mouseY <= BY + BH) {
            currentState = AppState::IN_GAME_SCREEN;
            return;
        }
    }
    else {
        // Xử lý khi nhấn nút "CONFIRM" lưu
        if (mouseX >= w - 105.f && mouseX <= w + 105.f && mouseY >= h + 44.f && mouseY <= h + 96.f) {
            if (inputName.empty()) inputName = "Untitled Game";

            int existingSlot = -1;
            for (int i = 1; i <= 5; ++i) {
                int bs, mv, tn; char gName[64];
                if (PeekGameSlot(i, &bs, &mv, &tn, gName)) {
                    if (inputName == std::string(gName)) {
                        existingSlot = i;
                        break;
                    }
                }
            }

            if (existingSlot != -1) {
                isConfirmOverwrite = true;
                slotToOverwrite = existingSlot;
            }
            else {
                SetSaveGameMode(gameMode == GameMode::PVP ? 2 : 1);
                if (SaveGameSlot(selectedSlot, timeRemaining, isPlayerTurn ? 1 : 0, inputName.c_str())) {
                    SaveHintState(selectedSlot, hintLeft);
                    saveNotifTimer = 2.0f;
                    currentLoadedSlot = selectedSlot;
                    currentLoadedName = inputName;
                    isNaming = false;
                    currentState = AppState::IN_GAME_SCREEN;
                }
                else errSound.play();
            }
        }
        else if (mouseX < w - 260.f || mouseX > w + 260.f || mouseY < h - 135.f || mouseY > h + 135.f) {
            isNaming = false;
        }
    }
}

