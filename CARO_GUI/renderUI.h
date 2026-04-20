#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Constants.h"
#include <string>

void DrawMenu(sf::RenderWindow& window, const sf::Font& font);

void DrawBoard(sf::RenderWindow& window, int boardSize);
void DrawPieces(sf::RenderWindow& window, int boardSize);
void DrawHoverEffect(sf::RenderWindow& window, int mouseX, int mouseY, int boardSize);

void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY);
void DrawWinLine(sf::RenderWindow& window, int sX, int sY, int eX, int eY, int boardSize);

void DrawInGamePanel(sf::RenderWindow& window, const sf::Font& font,
    float timeRemaining, bool isPlayerTurn,
    int gameStatus, int boardSize, GameMode gameMode,
    int undoLeft[2], float saveNotifTimer
); 

// void DrawInGamePanel(sf::RenderWindow& window, const sf::Font& font,
//float timeRemaining, bool isPlayerTurn,
//int gameStatus, int boardSize, GameMode gameMode,
//int undoLeft[2])

void DrawSettings(sf::RenderWindow& window, const sf::Font& font,
    int boardSize, bool ruleBlock2, int aiLevel,
    float sfxVolume, bool bgmEnabled
);

void DrawLoadScreen(sf::RenderWindow& window, const sf::Font& font);
void DrawSaveScreen(sf::RenderWindow& window, const sf::Font& font,
    bool isNaming, const std::string& inputName, sf::Clock& clock
);