#pragma once
#include "Types.h"
#include "Piece.h"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Chess {

// ─── Board ───────────────────────────────────────────────────────────────────
// Encapsulates the 8x8 board state.
// Uses a Grid<unique_ptr<Piece>> internally — no outside code touches the array.

class Board {
public:
    Board();
    Board(const Board& other);             // deep copy
    Board& operator=(const Board& other);  // deep copy assign
    ~Board() = default;

    // Setup
    void setupStartPosition();
    void clear();

    // Piece access
    Piece* getPieceAt(Square sq) const;
    Piece* getPieceAt(int file, int rank) const;
    bool   isEmpty(Square sq) const;
    bool   isOccupiedByColor(Square sq, Color c) const;

    // Mutation (used by MoveCommand)
    void placePiece(std::unique_ptr<Piece> piece, Square sq);
    std::unique_ptr<Piece> removePiece(Square sq);
    void movePieceInternal(Square from, Square to);

    // En passant target square (set by double pawn push)
    std::optional<Square> getEnPassantTarget() const { return m_enPassantTarget; }
    void setEnPassantTarget(std::optional<Square> sq) { m_enPassantTarget = sq; }

    // Castle rights
    CastleRights getCastleRights() const { return m_castleRights; }
    void setCastleRights(CastleRights cr) { m_castleRights = cr; }
    void revokeCastleRight(Color c, bool kingside);

    // Find king
    Square findKing(Color color) const;

    // All pieces of a color
    std::vector<Piece*> getPiecesOfColor(Color color) const;

    // FEN export
    std::string toFEN(Color activeColor, int halfmoveClock, int fullmoveNumber) const;

    // Console display
    void printToConsole() const;

    // 50-move rule halfmove clock
    int getHalfmoveClock() const { return m_halfmoveClock; }
    void setHalfmoveClock(int v) { m_halfmoveClock = v; }
    void resetHalfmoveClock()    { m_halfmoveClock = 0; }

private:
    // 8x8 grid: [file][rank], file=0 is 'a', rank=0 is rank 1
    std::array<std::array<std::unique_ptr<Piece>, 8>, 8> m_grid;

    std::optional<Square> m_enPassantTarget;
    CastleRights          m_castleRights;
    int                   m_halfmoveClock = 0;
};

} // namespace Chess
