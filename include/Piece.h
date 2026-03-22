#pragma once
#include "Types.h"
#include <vector>
#include <memory>
#include <string>

namespace Chess {

class Board; // forward declaration

// ─── Abstract Base: Piece ────────────────────────────────────────────────────
// Demonstrates: Abstraction, Encapsulation, Polymorphism

class Piece {
public:
    Piece(Color color, PieceType type, Square position);
    virtual ~Piece() = default;

    // Pure virtual — every piece must implement its own move generation
    virtual std::vector<Move> getPseudoLegalMoves(const Board& board) const = 0;

    // Pure virtual — returns piece symbol for console display
    virtual char getSymbol() const = 0;

    // Pure virtual — returns piece value for evaluation
    virtual int getValue() const = 0;

    // Virtual clone for deep copy (Prototype pattern)
    virtual std::unique_ptr<Piece> clone() const = 0;

    // Accessors (encapsulation)
    Color       getColor()    const { return m_color; }
    PieceType   getType()     const { return m_type; }
    Square      getPosition() const { return m_position; }
    bool        hasMoved()    const { return m_hasMoved; }

    void setPosition(Square sq) { m_position = sq; }
    void setHasMoved(bool v)    { m_hasMoved = v; }
