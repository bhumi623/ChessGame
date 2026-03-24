#pragma once
#include "Types.h"
#include "Board.h"
#include <memory>
#include <string>

namespace Chess {

// ─── MoveCommand ─────────────────────────────────────────────────────────────
// Command pattern: every move is an object that knows how to execute AND undo.
// This enables full move history, undo, and PGN generation.

class MoveCommand {
public:
    MoveCommand(Move move, Color color);

    // Apply the move to the board
    void execute(Board& board);

    // Reverse the move exactly
    void undo(Board& board);

    const Move&   getMove()  const { return m_move; }
    Color         getColor() const { return m_color; }
    const std::string& getSAN() const { return m_move.san; }

    // Snapshot for restoration on undo
    bool wasCapture()     const { return m_capturedPiece != nullptr; }
    bool wasPromotion()   const { return m_wasPromotion; }
    PieceType getPromotedTo() const { return m_promotedTo; }

private:
    Move                     m_move;
    Color                    m_color;

    // State to restore on undo
    std::unique_ptr<Piece>   m_capturedPiece;    // piece removed from board
    std::unique_ptr<Piece>   m_enPassantCapture; // pawn captured via en passant
    std::optional<Square>    m_prevEnPassantTarget;
    CastleRights             m_prevCastleRights;
    int                      m_prevHalfmoveClock = 0;
    bool                     m_wasPromotion = false;
    PieceType                m_promotedTo   = PieceType::None;

    void executeNormal(Board& board);
    void executeCastle(Board& board, bool kingside);
    void executeEnPassant(Board& board);
    void executePromotion(Board& board);
};

} // namespace Chess
