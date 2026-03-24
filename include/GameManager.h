#pragma once
#include "Types.h"
#include "Board.h"
#include "MoveCommand.h"
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <string>

namespace Chess {

// ─── GameManager ─────────────────────────────────────────────────────────────
// Singleton that owns the authoritative board state and move history.
// Implements the Observer pattern: UI components register callbacks
// and receive GameEvent notifications when state changes.

class GameManager {
public:
    // Singleton access
    static GameManager& getInstance();

    // Prevent copying
    GameManager(const GameManager&)            = delete;
    GameManager& operator=(const GameManager&) = delete;

    // Game lifecycle
    void newGame();
    void resign(Color color);

    // Move interface
    std::vector<Move> getLegalMovesForSquare(Square sq) const;
    bool tryMove(Move move); // returns true if move was applied
    bool tryPromote(PieceType type); // call after GameState::Promotion

    // Undo last move (both halves of a full move)
    bool undoLastMove();

    // State queries
    const Board&   getBoard()       const { return m_board; }
    Color          getActiveColor() const { return m_activeColor; }
    GameState      getGameState()   const { return m_gameState; }
    int            getFullmoveNumber() const { return m_fullmoveNumber; }
    bool           isGameOver()     const;

    // Move history (for PGN and UI panel)
    const std::vector<std::unique_ptr<MoveCommand>>& getMoveHistory() const {
        return m_history;
    }
    std::string getPGN() const;
    std::string getCurrentFEN() const;

    // Pending promotion square (set when a pawn reaches 8th rank)
    std::optional<Square> getPendingPromotionSquare() const {
        return m_pendingPromotionSquare;
    }

    // ── Observer pattern ──────────────────────────────────────────────────
    void subscribe(EventCallback cb);
    void unsubscribe(); // simple: clear all (sufficient for 2-party UI)

    // Position repetition tracking (for draw detection)
    int getPositionRepetitionCount(const std::string& fen) const;

private:
    GameManager();

    Board     m_board;
    Color     m_activeColor = Color::White;
    GameState m_gameState   = GameState::Playing;
    int       m_fullmoveNumber = 1;

    std::vector<std::unique_ptr<MoveCommand>> m_history;
    std::vector<EventCallback>                m_subscribers;

    std::optional<Square>  m_pendingPromotionSquare;
    std::optional<Move>    m_pendingPromoMove; // stored until piece type chosen

    // Position history for repetition draw
    std::vector<std::string> m_positionHistory;

    void applyMove(const Move& move);
    void switchTurn();
    void notify(GameEvent event, const Move& move);
    void updateGameState();
    bool isThreefoldRepetition() const;
    bool isInsufficientMaterial() const;
};

} // namespace Chess
