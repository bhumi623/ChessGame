#include "UIManager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Chess {

UIManager::UIManager() {}

#ifdef HAS_SFML

bool UIManager::loadFont(const std::string& fontPath) {
    if (m_font.loadFromFile(fontPath)) return true;
    // Fallback system fonts
    if (m_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) return true;
    if (m_font.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) return true;
    return false;
}

// ─── Update ──────────────────────────────────────────────────────────────────

void UIManager::update(GameState state, Color activeColor) {
    float elapsed = m_sfClock.restart().asSeconds();

    if (!m_clockRunning) {
        m_clockRunning = true;
        m_clockColor   = activeColor;
    }

    // Only tick clock during active play
    if (state == GameState::Playing || state == GameState::Check) {
        if (activeColor == Color::White)
            m_whiteTimeSeconds = std::max(0.f, m_whiteTimeSeconds - elapsed);
        else
            m_blackTimeSeconds = std::max(0.f, m_blackTimeSeconds - elapsed);
        m_clockColor = activeColor;
    }
}

// ─── Main Render ─────────────────────────────────────────────────────────────

void UIManager::render(sf::RenderWindow& window, const GameManager& gm) {
    drawPanel(window);
    drawPlayerInfo(window, gm);
    drawMoveHistory(window, gm);
    drawStatus(window, gm);
    drawButtons(window);
    drawCapturedPieces(window, gm);
}

// ─── Panel Background ────────────────────────────────────────────────────────

void UIManager::drawPanel(sf::RenderWindow& window) {
    sf::RectangleShape panel(sf::Vector2f(m_panelWidth, 760.f));
    panel.setPosition(m_panelX, 0.f);
    panel.setFillColor(sf::Color(22, 22, 22, 255));
    window.draw(panel);

    // Separator line
    sf::RectangleShape sep(sf::Vector2f(2.f, 760.f));
    sep.setPosition(m_panelX, 0.f);
    sep.setFillColor(sf::Color(60, 60, 60));
    window.draw(sep);
}

// ─── Player Info + Clock ──────────────────────────────────────────────────────

void UIManager::drawPlayerInfo(sf::RenderWindow& window, const GameManager& gm) {
    auto makeClockStr = [](float secs) -> std::string {
        int m = static_cast<int>(secs) / 60;
        int s = static_cast<int>(secs) % 60;
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << m << ":"
            << std::setw(2) << std::setfill('0') << s;
        return oss.str();
    };

    struct PlayerRow { std::string name; Color color; float timeLeft; float y; };
    std::vector<PlayerRow> rows = {
        { "Black", Color::Black, m_blackTimeSeconds, 18.f },
        { "White", Color::White, m_whiteTimeSeconds, 700.f }
    };

    for (auto& row : rows) {
        bool active  = (gm.getActiveColor() == row.color) && !gm.isGameOver();
        bool lowTime = row.timeLeft < 30.f;

        // Active indicator stripe
        if (active) {
            sf::RectangleShape stripe(sf::Vector2f(m_panelWidth, 42.f));
            stripe.setPosition(m_panelX, row.y);
            stripe.setFillColor(sf::Color(50, 50, 50));
            window.draw(stripe);
        }

        // Name
        sf::Text nameText;
        nameText.setFont(m_font);
        nameText.setString(row.name);
        nameText.setCharacterSize(15);
        nameText.setFillColor(active ? sf::Color(255, 255, 255) : sf::Color(160, 160, 160));
        nameText.setPosition(m_panelX + 12.f, row.y + 8.f);
        window.draw(nameText);

        // Clock
        sf::Text clockText;
        clockText.setFont(m_font);
        clockText.setString(makeClockStr(row.timeLeft));
        clockText.setCharacterSize(18);
        clockText.setStyle(sf::Text::Bold);
        clockText.setFillColor(lowTime ? sf::Color(220, 60, 60) :
                                (active ? sf::Color(255, 215, 0) : sf::Color(180, 180, 180)));
        clockText.setPosition(m_panelX + m_panelWidth - 80.f, row.y + 6.f);
        window.draw(clockText);
    }
}

// ─── Move History ────────────────────────────────────────────────────────────

void UIManager::drawMoveHistory(sf::RenderWindow& window, const GameManager& gm) {
    float x   = m_panelX + 10.f;
    float y   = 72.f;
    float maxY = 620.f;

    // Header
    sf::Text header;
    header.setFont(m_font);
    header.setString("Move history");
    header.setCharacterSize(13);
    header.setFillColor(sf::Color(130, 130, 130));
    header.setPosition(x, y);
    window.draw(header);
    y += 22.f;

    sf::RectangleShape divider(sf::Vector2f(m_panelWidth - 20.f, 1.f));
    divider.setPosition(x, y);
    divider.setFillColor(sf::Color(55, 55, 55));
    window.draw(divider);
    y += 8.f;

    const auto& history = gm.getMoveHistory();
    int total = static_cast<int>(history.size());

    // Show last N moves that fit
    int maxRows = static_cast<int>((maxY - y) / 20.f);
    int startIdx = std::max(0, total - maxRows * 2);
    // Align to even (start of white's move)
    if (startIdx % 2 == 1) startIdx--;

    int moveNum = (startIdx / 2) + 1;

    for (int i = startIdx; i < total; i += 2) {
        if (y > maxY) break;

        // Move number
        sf::Text numText;
        numText.setFont(m_font);
        numText.setString(std::to_string(moveNum++) + ".");
        numText.setCharacterSize(13);
        numText.setFillColor(sf::Color(100, 100, 100));
        numText.setPosition(x, y);
        window.draw(numText);

        // White's move
        sf::Text wMove;
        wMove.setFont(m_font);
        wMove.setString(history[i]->getSAN());
        wMove.setCharacterSize(14);
        bool isLastWhite = (i == total - 1) || (i == total - 2 && total % 2 == 0);
        wMove.setFillColor(isLastWhite ? sf::Color(255, 215, 0) : sf::Color(220, 220, 220));
        wMove.setPosition(x + 32.f, y);
        window.draw(wMove);

        // Black's move (if it exists)
        if (i + 1 < total) {
            sf::Text bMove;
            bMove.setFont(m_font);
            bMove.setString(history[i + 1]->getSAN());
            bMove.setCharacterSize(14);
            bool isLastBlack = (i + 1 == total - 1);
            bMove.setFillColor(isLastBlack ? sf::Color(255, 215, 0) : sf::Color(220, 220, 220));
            bMove.setPosition(x + 110.f, y);
            window.draw(bMove);
        }

        y += 20.f;
    }
}

// ─── Status Bar ──────────────────────────────────────────────────────────────

void UIManager::drawStatus(sf::RenderWindow& window, const GameManager& gm) {
    std::string statusStr;
    sf::Color   statusColor = sf::Color(200, 200, 200);

    switch (gm.getGameState()) {
        case GameState::Playing:
            statusStr  = (gm.getActiveColor() == Color::White) ? "White to move" : "Black to move";
            break;
        case GameState::Check:
            statusStr  = (gm.getActiveColor() == Color::White) ? "White is in check!" : "Black is in check!";
            statusColor = sf::Color(230, 100, 60);
            break;
        case GameState::Checkmate: {
            Color winner = (gm.getActiveColor() == Color::White) ? Color::Black : Color::White;
            statusStr  = (winner == Color::White) ? "Checkmate! White wins." : "Checkmate! Black wins.";
            statusColor = sf::Color(80, 220, 80);
            break;
        }
        case GameState::Stalemate:
            statusStr  = "Stalemate — Draw!";
            statusColor = sf::Color(200, 200, 80);
            break;
        case GameState::Draw:
            statusStr  = "Draw!";
            statusColor = sf::Color(200, 200, 80);
            break;
        case GameState::Promotion:
            statusStr  = "Choose promotion piece";
            statusColor = sf::Color(100, 180, 255);
            break;
    }

    sf::Text status;
    status.setFont(m_font);
    status.setString(statusStr);
    status.setCharacterSize(14);
    status.setFillColor(statusColor);
    // Center in panel
    auto bounds = status.getLocalBounds();
    status.setPosition(m_panelX + (m_panelWidth - bounds.width) * 0.5f, 632.f);
    window.draw(status);
}

// ─── Buttons ─────────────────────────────────────────────────────────────────

void UIManager::drawButtons(sf::RenderWindow& window) {
    struct BtnDef { sf::FloatRect* rect; std::string label; sf::Color color; };
    float bx   = m_panelX + 10.f;
    float bw   = (m_panelWidth - 25.f) * 0.5f;
    float bh   = 30.f;

    m_btnNewGame = {bx,         656.f, bw, bh};
    m_btnUndo    = {bx + bw + 5.f, 656.f, bw, bh};
    m_btnFlip    = {bx,         694.f, bw, bh};
    m_btnResign  = {bx + bw + 5.f, 694.f, bw, bh};

    std::vector<BtnDef> btns = {
        {&m_btnNewGame, "New Game",  sf::Color(50, 130, 80)},
        {&m_btnUndo,    "Undo",      sf::Color(80, 80, 140)},
        {&m_btnFlip,    "Flip Board",sf::Color(80, 100, 120)},
        {&m_btnResign,  "Resign",    sf::Color(140, 60, 60)},
    };

    for (auto& btn : btns) {
        sf::RectangleShape shape(sf::Vector2f(btn.rect->width, btn.rect->height));
        shape.setPosition(btn.rect->left, btn.rect->top);
        shape.setFillColor(btn.color);
        shape.setOutlineColor(sf::Color(120, 120, 120));
        shape.setOutlineThickness(1.f);
        window.draw(shape);

        sf::Text label;
        label.setFont(m_font);
        label.setString(btn.label);
        label.setCharacterSize(12);
        label.setFillColor(sf::Color::White);
        auto lb = label.getLocalBounds();
        label.setPosition(btn.rect->left + (btn.rect->width  - lb.width)  * 0.5f,
                           btn.rect->top  + (btn.rect->height - lb.height) * 0.5f - 2.f);
        window.draw(label);
    }
}

// ─── Captured Pieces ─────────────────────────────────────────────────────────

void UIManager::drawCapturedPieces(sf::RenderWindow& window, const GameManager& gm) {
    // Count captured pieces from material difference
    // We scan move history for captures
    std::map<std::pair<Color,PieceType>, int> captured;
    for (const auto& cmd : gm.getMoveHistory()) {
        if (cmd->wasCapture()) {
            // We can't easily get the captured piece type from the public API here.
            // In a full implementation, MoveCommand would expose it.
            // For now, we just display the move count as a proxy.
        }
    }
    // Displayed as move count below the history (simplification)
    sf::Text moveCount;
    moveCount.setFont(m_font);
    std::string ms = "Moves: " + std::to_string(gm.getMoveHistory().size());
    moveCount.setString(ms);
    moveCount.setCharacterSize(12);
    moveCount.setFillColor(sf::Color(100, 100, 100));
    moveCount.setPosition(m_panelX + 10.f, 737.f);
    window.draw(moveCount);
}

// ─── Button Clicks ────────────────────────────────────────────────────────────

UIManager::ButtonResult UIManager::handleClick(sf::Vector2i pos) {
    ButtonResult result;
    sf::Vector2f fp(static_cast<float>(pos.x), static_cast<float>(pos.y));
    if (m_btnNewGame.contains(fp)) result.newGame = true;
    if (m_btnUndo.contains(fp))    result.undo    = true;
    if (m_btnFlip.contains(fp))    result.flip    = true;
    if (m_btnResign.contains(fp))  result.resign  = true;
    return result;
}

#endif

} // namespace Chess
