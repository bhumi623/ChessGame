#pragma once
#include <string>
#include <optional>
#include <vector>
#include <functional>

namespace Chess {

// ─── Enumerations ────────────────────────────────────────────────────────────

enum class Color { White, Black, None };

enum class PieceType {
    King, Queen, Rook, Bishop, Knight, Pawn, None
};

enum class GameState {
    Playing,
    Check,
    Checkmate,
    Stalemate,
    Draw,
    Promotion   // waiting for promotion choice
};

enum class MoveType {
    Normal,
    Capture,
    EnPassant,
    CastleKingside,
    CastleQueenside,
    PawnDouble,     // two-square pawn push (sets en passant target)
    Promotion,
    PromotionCapture
};

// ─── Square ──────────────────────────────────────────────────────────────────

struct Square {
    int file; // 0-7 = a-h
    int rank; // 0-7 = rank 1-8

    Square() : file(-1), rank(-1) {}
    Square(int f, int r) : file(f), rank(r) {}

    bool isValid() const {
        return file >= 0 && file < 8 && rank >= 0 && rank < 8;
    }

    bool operator==(const Square& o) const {
        return file == o.file && rank == o.rank;
    }

    bool operator!=(const Square& o) const { return !(*this == o); }

    // e.g. "e4"
    std::string toAlgebraic() const {
        if (!isValid()) return "--";
        std::string s;
        s += static_cast<char>('a' + file);
        s += static_cast<char>('1' + rank);
        return s;
    }

    static std::optional<Square> fromAlgebraic(const std::string& s) {
        if (s.size() < 2) return std::nullopt;
        int f = s[0] - 'a';
        int r = s[1] - '1';
        Square sq(f, r);
        if (!sq.isValid()) return std::nullopt;
        return sq;
    }
};

// ─── Move ────────────────────────────────────────────────────────────────────

struct Move {
    Square from;
    Square to;
    MoveType type = MoveType::Normal;
    PieceType promoteTo = PieceType::Queen; // used for Promotion types
    std::string san; // Standard Algebraic Notation (filled in by MoveValidator)

    Move() = default;
    Move(Square f, Square t, MoveType mt = MoveType::Normal)
        : from(f), to(t), type(mt) {}

    bool operator==(const Move& o) const {
        return from == o.from && to == o.to && type == o.type && promoteTo == o.promoteTo;
    }
};

// ─── Castle Rights ───────────────────────────────────────────────────────────

struct CastleRights {
    bool whiteKingside  = true;
    bool whiteQueenside = true;
    bool blackKingside  = true;
    bool blackQueenside = true;
};

// ─── Game Event (Observer pattern) ───────────────────────────────────────────

enum class GameEvent {
    BoardChanged,
    CheckDetected,
    CheckmateDetected,
    StalemateDetected,
    PieceCapture,
    Castled,
    EnPassant,
    PawnPromoted,
    TurnChanged,
    DrawDetected
};

using EventCallback = std::function<void(GameEvent, const Move&)>;

} // namespace Chess
