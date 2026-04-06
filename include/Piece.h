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

    // Helper: is opponent's piece at target?
    bool isEnemy(Color other) const { return m_color != other && other != Color::None; }

    std::string getName() const;

protected:
    Color     m_color;
    PieceType m_type;
    Square    m_position;
    bool      m_hasMoved = false;

    // Helpers for sliding pieces (Bishop/Rook/Queen)
    void addSlidingMoves(const Board& board, std::vector<Move>& moves,
                         const std::vector<std::pair<int,int>>& directions) const;

    void addMoveIfValid(const Board& board, std::vector<Move>& moves,
                        Square target) const;

    void addCaptureIfValid(const Board& board, std::vector<Move>& moves,
                           Square target) const;
};

// ─── King ────────────────────────────────────────────────────────────────────

class King : public Piece {
public:
    King(Color color, Square pos) : Piece(color, PieceType::King, pos) {}

    std::vector<Move> getPseudoLegalMoves(const Board& board) const override;
    char getSymbol()  const override { return (m_color == Color::White) ? 'K' : 'k'; }
    int  getValue()   const override { return 20000; }
    std::unique_ptr<Piece> clone() const override { return std::make_unique<King>(*this); }
};

// ─── Queen ───────────────────────────────────────────────────────────────────

class Queen : public Piece {
public:
    Queen(Color color, Square pos) : Piece(color, PieceType::Queen, pos) {}

    std::vector<Move> getPseudoLegalMoves(const Board& board) const override;
    char getSymbol()  const override { return (m_color == Color::White) ? 'Q' : 'q'; }
    int  getValue()   const override { return 900; }
    std::unique_ptr<Piece> clone() const override { return std::make_unique<Queen>(*this); }
};

// ─── Rook ────────────────────────────────────────────────────────────────────

class Rook : public Piece {
public:
    Rook(Color color, Square pos) : Piece(color, PieceType::Rook, pos) {}

    std::vector<Move> getPseudoLegalMoves(const Board& board) const override;
    char getSymbol()  const override { return (m_color == Color::White) ? 'R' : 'r'; }
    int  getValue()   const override { return 500; }
    std::unique_ptr<Piece> clone() const override { return std::make_unique<Rook>(*this); }
};

// ─── Bishop ──────────────────────────────────────────────────────────────────

class Bishop : public Piece {
public:
    Bishop(Color color, Square pos) : Piece(color, PieceType::Bishop, pos) {}

    std::vector<Move> getPseudoLegalMoves(const Board& board) const override;
    char getSymbol()  const override { return (m_color == Color::White) ? 'B' : 'b'; }
    int  getValue()   const override { return 330; }
    std::unique_ptr<Piece> clone() const override { return std::make_unique<Bishop>(*this); }
};

// ─── Knight ──────────────────────────────────────────────────────────────────

class Knight : public Piece {
public:
    Knight(Color color, Square pos) : Piece(color, PieceType::Knight, pos) {}

    std::vector<Move> getPseudoLegalMoves(const Board& board) const override;
    char getSymbol()  const override { return (m_color == Color::White) ? 'N' : 'n'; }
    int  getValue()   const override { return 320; }
    std::unique_ptr<Piece> clone() const override { return std::make_unique<Knight>(*this); }
};

// ─── Pawn ────────────────────────────────────────────────────────────────────

class Pawn : public Piece {
public:
    Pawn(Color color, Square pos) : Piece(color, PieceType::Pawn, pos) {}

    std::vector<Move> getPseudoLegalMoves(const Board& board) const override;
    char getSymbol()  const override { return (m_color == Color::White) ? 'P' : 'p'; }
    int  getValue()   const override { return 100; }
    std::unique_ptr<Piece> clone() const override { return std::make_unique<Pawn>(*this); }
};

} // namespace Chess
