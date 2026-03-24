#pragma once
#include "Types.h"
#include "Board.h"
#include <vector>

namespace Chess {

// ─── MoveValidator ───────────────────────────────────────────────────────────
// Filters pseudo-legal moves to fully legal moves.
// Handles: check detection, pin detection, castling legality.
// Demonstrates: Single Responsibility Principle

class MoveValidator {
public:
    // Get all fully legal moves for a color
    static std::vector<Move> getLegalMoves(const Board& board, Color color);

    // Get all fully legal moves for a specific piece
    static std::vector<Move> getLegalMovesForPiece(const Board& board, const Piece* piece);

    // Check if a specific move is legal
    static bool isMoveLegal(const Board& board, const Move& move, Color color);

    // Is the given color's king currently in check?
    static bool isInCheck(const Board& board, Color color);

    // Is the given square attacked by the given color?
    static bool isSquareAttackedBy(const Board& board, Square sq, Color attacker);

    // Detect game state after a move has been applied
    static GameState detectGameState(const Board& board, Color colorToMove);

    // Generate castling moves (if legal)
    static std::vector<Move> getCastlingMoves(const Board& board, const King* king);

    // Build SAN notation for a move (before it's applied)
    static std::string buildSAN(const Board& board, const Move& move, Color color);

private:
    // Apply move temporarily, test if king is in check, then undo
    static bool leavesKingInCheck(const Board& board, const Move& move, Color color);
};

} // namespace Chess
