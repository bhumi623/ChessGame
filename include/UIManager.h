#pragma once
#include "Types.h"
#include "GameManager.h"
#include <string>
#include <vector>
#include <chrono>

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#endif

namespace Chess {

// ─── UIManager ───────────────────────────────────────────────────────────────
// Renders the side panel: player info, move history, status messages,
// captured pieces, and buttons (New Game, Undo, Flip Board).

class UIManager {
public:
    UIManager();

#ifdef HAS_SFML
    bool loadFont(const std::string& fontPath);

    void render(sf::RenderWindow& window, const GameManager& gm);

    // Returns true if Undo was clicked
    // Returns true if New Game was clicked
    // Returns true if Flip was clicked
    struct ButtonResult {
        bool newGame  = false;
        bool undo     = false;
        bool flip     = false;
        bool resign   = false;
    };

    ButtonResult handleClick(sf::Vector2i pos);

    // Update game clock each frame
    void update(GameState state, Color activeColor);

    float getPanelX()     const { return m_panelX; }
    float getPanelWidth() const { return m_panelWidth; }

private:
    void drawPanel(sf::RenderWindow& window);
    void drawPlayerInfo(sf::RenderWindow& window, const GameManager& gm);
    void drawMoveHistory(sf::RenderWindow& window, const GameManager& gm);
    void drawStatus(sf::RenderWindow& window, const GameManager& gm);
    void drawButtons(sf::RenderWindow& window);
    void drawCapturedPieces(sf::RenderWindow& window, const GameManager& gm);

    sf::Font  m_font;
    float     m_panelX     = 680.f; // right of board (8*80 + 2*20)
    float     m_panelWidth = 240.f;

    // Button rects
    sf::FloatRect m_btnNewGame;
    sf::FloatRect m_btnUndo;
    sf::FloatRect m_btnFlip;
    sf::FloatRect m_btnResign;

    // Clocks (chess clock per side)
    float m_whiteTimeSeconds = 600.f; // 10 min default
    float m_blackTimeSeconds = 600.f;
    sf::Clock m_sfClock;
    bool  m_clockRunning = false;
    Color m_clockColor   = Color::White;
#endif
};

} // namespace Chess
