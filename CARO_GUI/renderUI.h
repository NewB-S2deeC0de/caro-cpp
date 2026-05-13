#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Constants.h"

void DrawMenu(sf::RenderWindow& window, const sf::Font& font, sf::Sprite& bgSprite);

void DrawBoard(sf::RenderWindow& window, int boardSize);
void DrawPieces(sf::RenderWindow& window, int boardSize);
void DrawHoverEffect(sf::RenderWindow& window, int gX, int gY, int boardSize);

void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY);
void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY, int boardSize);

void DrawInGamePanel(sf::RenderWindow& window, const sf::Font& font,
                     float timeRemaining, bool isPlayerTurn, int gameStatus,
                     int boardSize, GameMode gameMode, int undoLeft[2],
                     float saveNotifTimer, int p1Char, int p2Char,
                     const std::string& p1Name, const std::string& p2Name,
                     sf::Sprite charSprites[4]
                     );

void DrawSettings(sf::RenderWindow& window, const sf::Font& font,
    int boardSize, bool ruleBlock2, int aiLevel,
    float sfxVolume, bool bgmEnabled
);

void DrawLoadScreen(sf::RenderWindow& window, const sf::Font& font);
void DrawSaveScreen(sf::RenderWindow& window, const sf::Font& font,
    bool isNaming, const std::string& inputName, sf::Clock& clock,
    bool isConfirmOverwrite
);
void DrawCharacterSelectScreen(sf::RenderWindow& window, const sf::Font& font, bool isPVP,
    int p1Char, int p2Char, const std::string& p1Name, const std::string& p2Name,
    int typingState, int selectionStep, sf::Sprite charSprites[4],
    int animatingCharIdx, sf::Clock& confirmAnimClk); 
void DrawAbout(sf::RenderWindow& window, const sf::Font& font);
