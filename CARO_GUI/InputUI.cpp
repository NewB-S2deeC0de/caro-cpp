#include "InputUI.h"
#include "CaroAPI.h"
#include <algorithm>

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
    std::string& currentLoadedName)

{
    static sf::Clock clickCooldown;
    if (clickCooldown.getElapsedTime().asMilliseconds() < 150)
    {
        return; // Click quá nhanh thì bỏ qua không xử lý buffer này
    }
    clickCooldown.restart();
    // --- A. Khu vực bàn cờ ---
    int cellSz = GetDynCellSize(boardSize);
    const int BOARD_LEFT = Config::OFFSET_X;
    const int BOARD_TOP = Config::OFFSET_Y;
    const int BOARD_RIGHT = Config::OFFSET_X + boardSize * cellSz;
    const int BOARD_BOTTOM = Config::OFFSET_Y + boardSize * cellSz;

    if (mouseX >= BOARD_LEFT && mouseX <= BOARD_RIGHT &&
        mouseY >= BOARD_TOP && mouseY <= BOARD_BOTTOM)
    {
        if (gameMode == GameMode::PVE && !isPlayerTurn)
        {
            return;
        }

        if (gameStatus == 0 && !IsAIThinking())
        {
            int gX = (mouseX - Config::OFFSET_X) / cellSz;
            int gY = (mouseY - Config::OFFSET_Y) / cellSz;

            int currentPlayer = (gameMode == GameMode::PVP && !isPlayerTurn) ? 2 : 1;
            int res = ProcessMove(gX, gY, currentPlayer);

            if (res == -1)
            {
                errSound.play();
            }
            else
            {
                gameStatus = res;
                timeRemaining = 60.f;

                // Ng??i v?a ?i n??c th?c ? reset quy?n undo c?a h?
                // (???c php undo l?i ? l??t sau n?u cn l??t)
                //
                // 
                // old 
                // int playerIdx = (currentPlayer == 1) ? 0 : 1;
                lastUndoPlayer = -1; // ai cũng có thể undo, không bị chặn liên tiếp

                if (gameMode == GameMode::PVP)
                {
                    isPlayerTurn = !isPlayerTurn;
                }
                else {
                    isPlayerTurn = false;
                    if (gameStatus == 0) StartAIThinking();
                }
            }
        }
        return;
    }

    // --- B. Nút Undo / Save / Main Menu ---
    const float pX = PanelX(boardSize);
    const float BTN_W = static_cast<float>(Config::PANEL_W);
    const float BTN_H = 52.f;

    for (int i = 0; i < 3; ++i)
    {
        float bX = pX;
        float bY = 420.f + i * 80.f;
        if (!(mouseX >= bX && mouseX <= bX + BTN_W &&
            mouseY >= bY && mouseY <= bY + BTN_H))
        {
            continue;
        }

        if (i == 0) // UNDO 
        {
            // Nếu hết giờ hoặc có người thắng (gameStatus != 0) thì ko undo được nữa
            if (gameStatus != 0)
            {
                errSound.play(); // Phát tiếng báo lỗi
                continue;        // Thoát ra, không làm gì thêm
            }
            if (gameMode == GameMode::PVP)
            {
                // playerIdx = người đang đến lượt (người bấm Undo)
                // isPlayerTurn true = lượt X (P1, idx 0)
                // isPlayerTurn false = lượt O (P2, idx 1)
                int playerIdx = isPlayerTurn ? 0 : 1;

                // Chọn undo liên tiếp: phải đi 1 nước thực trước
                if (lastUndoPlayer == playerIdx || undoLeft[playerIdx] <= 0)
                {
                    errSound.play();
                    return;
                }

                // ?? FIX BUG 1 & 2: dùng UndoMove() pop cặp 2 nước đi
                // Lí do:
                //   Khi đến lượt P1, stack top là nước của P2 (vừa đánh).
                //   UndoOneMove() chỉ xóa nước P2 và P1 không lấy lại được nước của mình, và lượt bị lệch.
                //   UndoMove() xóa cả 2 (P2 + P1) ? P1 về đúng trạng thái trước khi P1 bấm nước đó, isPlayerTurn không ??i ? P1 được đi lại đúng.
                int undone = UndoMove();
                if (undone == 0)
                {
                    errSound.play(); // stack rỗng
                    return;
                }

                // Undo thành công
                undoLeft[playerIdx]--;
                lastUndoPlayer = playerIdx; // đánh dấu người này vừa undo

                gameStatus = 0;
                timeRemaining = 60.f;

                // isPlayerTurn KHÔNG ??i ? ?úng người vừa undo
                // Được đi lại nước của mình.
                //
                // Trường hợp đặc biệt: nếu stack chỉ có 1 nước
                // (P1 mới đánh 1 nước, chưa ai đánh thêm) và P1 undo
                // ? undone == 1, board sạch, trả lượt về P1 (isPlayerTurn = true)
                if (undone == 1)
                {
                    isPlayerTurn = true;
                }
            }
            else // PVE
            {
                if (IsAIThinking())
                {
                    StartAIThinking();
                    errSound.play();
                    return;
                }

                int undone = UndoMove(); // undo cặp AI + người
                if (undone == 0)
                {
                    errSound.play();
                    return;
                }

                gameStatus = 0;
                isPlayerTurn = true;
                timeRemaining = 60.f;
            }
        }
        else if (i == 1) // Save
        {
            //
            // 
            // old
            // SaveGameBinary("savegame.bin", timeRemaining, isPlayerTurn ? 1 : 0);
            if (currentLoadedSlot != -1)
            {
                if (SaveGameSlot(currentLoadedSlot, timeRemaining, isPlayerTurn ? 1 : 0, currentLoadedName.c_str()))
                {
                    saveNotifTimer = 2.0f;
                }
                else
                {
                    errSound.play();
                }
            }
            else
            {
                currentState = AppState::SAVE_SCREEN;
            }
        }
        else if (i == 2) // Main Menu
        {
            currentState = AppState::MENU_SCREEN;
        }
    }
}

//
void HandleAboutInput(int mouseX, int mouseY, AppState& currentState, sf::Sound& errSound)
{
    float W = static_cast<float>(Config::WIN_WIDTH);
    float H = static_cast<float>(Config::WIN_HEIGHT);

    // Khớp tọa độ tuyệt đối với RenderUI
    float bH_frame = 620.f;
    float bY_frame = 90.f;

    const float BW = 250.f, BH = 55.f;
    float BX = W / 2.f - BW / 2.f;
    float BY = bY_frame + bH_frame + 25.f;

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
    float& sfxVolume, bool& bgmEnabled,
    sf::Sound& errSound)
{
    const float SY = 200.f;
    const float CX = 650.f;
    const float PX2 = CX + 200.f;
    const float BS = 50.f;
    const float RG = 80.f;

    auto hMinus = [&](float ry)
        {
            return mouseX >= CX && mouseX <= CX + BS && mouseY >= ry && mouseY <= ry + BS;
        };
    auto hPlus = [&](float ry)
        {
            return mouseX >= PX2 && mouseX <= PX2 + BS && mouseY >= ry && mouseY <= ry + BS;
        };
    auto hToggle = [&](float ry)
        {
            return mouseX >= CX && mouseX <= CX + 250 && mouseY >= ry && mouseY <= ry + BS;
        };

    float r0 = SY;
    if (hMinus(r0))
    {
        boardSize = std::max(10, boardSize - 1);
        errSound.play();
    }
    else if (hPlus(r0))
    {
        boardSize = std::min(30, boardSize + 1);
        errSound.play();
    }

    if (hToggle(SY + RG))
    {
        ruleBlock2 = !ruleBlock2;
        errSound.play();
    }

    float r2 = SY + RG * 2;
    if (hMinus(r2))
    {
        aiLevel = std::max(1, aiLevel - 1);
        errSound.play();
    }
    else if (hPlus(r2))
    {
        aiLevel = std::min(3, aiLevel + 1);
        errSound.play();
    }

    float r3 = SY + RG * 3;
    if (hMinus(r3))
    {
        sfxVolume = std::max(0.f, sfxVolume - 10.f);
        errSound.setVolume(sfxVolume);
        errSound.play();
    }
    else if (hPlus(r3))
    {
        sfxVolume = std::min(100.f, sfxVolume + 10.f);
        errSound.setVolume(sfxVolume);
        errSound.play();
    }

    if (hToggle(SY + RG * 4))
    {
        bgmEnabled = !bgmEnabled;
        errSound.play();
    }

    const float BW = 300.f;
    const float BH = 60.f;
    const float BX = Config::WIN_WIDTH / 2.f - BW / 2.f;
    const float BY = 650.f;
    if (mouseX >= BX && mouseX <= BX + BW && mouseY >= BY && mouseY <= BY + BH)
    {
        currentState = AppState::MENU_SCREEN;
    }
}

void HandleLoadInput(sf::RenderWindow& window, int mouseX, int mouseY, AppState& currentState, float& timeRemaining, bool& isPlayerTurn, int& gameStatus, sf::Sound& errSound, int& currentLoadedSlot, std::string& currentLoadedName) {
    const float START_X = 80.f;
    const float START_Y = 150.f;
    const float BTN_W = 400.f;
    const float BTN_H = 75.f;
    const float DEL_W = 70.f;

    for (int i = 1; i <= 5; ++i) {
        float bY = START_Y + (i - 1) * 95.f;
        int size = 0, moves = 0, turn = 0; char name[64];
        bool hasData = PeekGameSlot(i, &size, &moves, &turn, name);

        if (hasData && mouseX >= START_X + BTN_W + 10.f && mouseX <= START_X + BTN_W + 10.f + DEL_W && mouseY >= bY && mouseY <= bY + BTN_H) {
            DeleteGameSlot(i); errSound.play(); return;
        }

        if (mouseX >= START_X && mouseX <= START_X + BTN_W && mouseY >= bY && mouseY <= bY + BTN_H) {
            float tRem = 0; int turnData = 0;
            if (LoadGameSlot(i, &tRem, &turnData)) {
                timeRemaining = tRem;
                isPlayerTurn = (turnData == 1);
                gameStatus = EvaluateBoard();

                // Ghi nhớ lại Slot và Tên vừa load để đánh tiếp save khỏi hỏi
                currentLoadedSlot = i;
                currentLoadedName = std::string(name);

                currentState = AppState::IN_GAME_SCREEN;
            }
            else { errSound.play(); }
            return;
        }
    }
    if (mouseX >= 80.f && mouseX <= 280.f && mouseY >= 680.f && mouseY <= 740.f) currentState = AppState::MENU_SCREEN;
}

void HandleSaveInput(sf::RenderWindow& window, int mouseX, int mouseY, AppState& currentState, float timeRemaining, bool isPlayerTurn, float& saveNotifTimer, sf::Sound& errSound, bool& isNaming, int& selectedSlot, std::string& inputName, int& currentLoadedSlot, std::string& currentLoadedName) {
    const float START_X = 80.f, START_Y = 150.f, BTN_W = 480.f, BTN_H = 75.f;
    if (!isNaming) {
        for (int i = 1; i <= 5; ++i) {
            float bY = START_Y + (i - 1) * 95.f;
            if (mouseX >= START_X && mouseX <= START_X + BTN_W && mouseY >= bY && mouseY <= bY + BTN_H) {
                selectedSlot = i; isNaming = true; inputName = ""; return;
            }
        }
        if (mouseX >= 80.f && mouseX <= 280.f && mouseY >= 680.f && mouseY <= 740.f) currentState = AppState::IN_GAME_SCREEN;
    }
    else {
        float w = Config::WIN_WIDTH / 2.f, h = Config::WIN_HEIGHT / 2.f;
        if (mouseX >= w - 100.f && mouseX <= w + 100.f && mouseY >= h + 35.f && mouseY <= h + 85.f) {
            if (inputName.empty()) inputName = "Game khong ten";
            if (SaveGameSlot(selectedSlot, timeRemaining, isPlayerTurn ? 1 : 0, inputName.c_str())) {
                saveNotifTimer = 2.0f;

                // Ghi nhớ lại Slot và Tên lần đầu tiên Save này!
                currentLoadedSlot = selectedSlot;
                currentLoadedName = inputName;

                isNaming = false;
                currentState = AppState::IN_GAME_SCREEN;
            }
            else errSound.play();
        }
        else if (mouseX < w - 250.f || mouseX > w + 250.f || mouseY < h - 125.f || mouseY > h + 125.f) {
            isNaming = false;
        }
    }
}
