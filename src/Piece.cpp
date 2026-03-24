#include "Piece.h"
#include "Board.h"
#include <stdexcept>

namespace Chess {

// ─── Piece base ──────────────────────────────────────────────────────────────

Piece::Piece(Color color, PieceType type, Square position)
    : m_color(color), m_type(type), m_position(position) {}

std::string Piece::getName() const {
    switch (m_type) {
        case PieceType::King:   return "King";
        case PieceType::Queen:  return "Queen";
        case PieceType::Rook:   return "Rook";
        case PieceType::Bishop: return "Bishop";
        case PieceType::Knight: return "Knight";
        case PieceType::Pawn:   return "Pawn";
        default:                return "None";
    }
}

void Piece::addSlidingMoves(const Board& board,
                             std::vector<Move>& moves,
                             const std::vector<std::pair<int,int>>& dirs) const {
    for (auto [df, dr] : dirs) {
        for (int step = 1; step < 8; ++step) {
            Square target(m_position.file + df * step,
                          m_position.rank + dr * step);
            if (!target.isValid()) break;

            Piece* occupant = board.getPieceAt(target);
            if (occupant == nullptr) {
                moves.emplace_back(m_position, target, MoveType::Normal);
            } else {
                if (occupant->getColor() != m_color) {
                    moves.emplace_back(m_position, target, MoveType::Capture);
                }
                break; // blocked
            }
        }
    }
}

void Piece::addMoveIfValid(const Board& board,
                            std::vector<Move>& moves,
                            Square target) const {
    if (!target.isValid()) return;
    if (board.isEmpty(target))
        moves.emplace_back(m_position, target, MoveType::Normal);
}

void Piece::addCaptureIfValid(const Board& board,
                               std::vector<Move>& moves,
                               Square target) const {
    if (!target.isValid()) return;
    Piece* p = board.getPieceAt(target);
    if (p && p->getColor() != m_color)
        moves.emplace_back(m_position, target, MoveType::Capture);
}

// ─── King ────────────────────────────────────────────────────────────────────

std::vector<Move> King::getPseudoLegalMoves(const Board& board) const {
    std::vector<Move> moves;
    static const int offsets[8][2] = {
        {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}
    };
    for (auto& off : offsets) {
        Square t(m_position.file + off[0], m_position.rank + off[1]);
        if (!t.isValid()) continue;
        Piece* p = board.getPieceAt(t);
        if (!p) {
            moves.emplace_back(m_position, t, MoveType::Normal);
        } else if (p->getColor() != m_color) {
            moves.emplace_back(m_position, t, MoveType::Capture);
        }
    }
    // Castling moves are generated separately in MoveValidator
    return moves;
}

// ─── Queen ───────────────────────────────────────────────────────────────────

std::vector<Move> Queen::getPseudoLegalMoves(const Board& board) const {
    std::vector<Move> moves;
    static const std::vector<std::pair<int,int>> dirs = {
        {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}
    };
    addSlidingMoves(board, moves, dirs);
    return moves;
}

// ─── Rook ────────────────────────────────────────────────────────────────────

std::vector<Move> Rook::getPseudoLegalMoves(const Board& board) const {
    std::vector<Move> moves;
    static const std::vector<std::pair<int,int>> dirs = {
        {1,0},{-1,0},{0,1},{0,-1}
    };
    addSlidingMoves(board, moves, dirs);
    return moves;
}

// ─── Bishop ──────────────────────────────────────────────────────────────────

std::vector<Move> Bishop::getPseudoLegalMoves(const Board& board) const {
    std::vector<Move> moves;
    static const std::vector<std::pair<int,int>> dirs = {
        {1,1},{1,-1},{-1,1},{-1,-1}
    };
    addSlidingMoves(board, moves, dirs);
    return moves;
}

// ─── Knight ──────────────────────────────────────────────────────────────────

std::vector<Move> Knight::getPseudoLegalMoves(const Board& board) const {
    std::vector<Move> moves;
    static const int offsets[8][2] = {
        {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
    };
    for (auto& off : offsets) {
        Square t(m_position.file + off[0], m_position.rank + off[1]);
        if (!t.isValid()) continue;
        Piece* p = board.getPieceAt(t);
        if (!p) {
            moves.emplace_back(m_position, t, MoveType::Normal);
        } else if (p->getColor() != m_color) {
            moves.emplace_back(m_position, t, MoveType::Capture);
        }
    }
    return moves;
}

// ─── Pawn ────────────────────────────────────────────────────────────────────

std::vector<Move> Pawn::getPseudoLegalMoves(const Board& board) const {
    std::vector<Move> moves;
    int dir = (m_color == Color::White) ? 1 : -1;
    int startRank   = (m_color == Color::White) ? 1 : 6;
    int promoteRank = (m_color == Color::White) ? 7 : 0;

    // One step forward
    Square oneStep(m_position.file, m_position.rank + dir);
    if (oneStep.isValid() && board.isEmpty(oneStep)) {
        if (oneStep.rank == promoteRank) {
            for (auto pt : {PieceType::Queen, PieceType::Rook,
                             PieceType::Bishop, PieceType::Knight}) {
                Move m(m_position, oneStep, MoveType::Promotion);
                m.promoteTo = pt;
                moves.push_back(m);
            }
        } else {
            moves.emplace_back(m_position, oneStep, MoveType::Normal);

            // Two steps from start
            if (m_position.rank == startRank) {
                Square twoStep(m_position.file, m_position.rank + 2 * dir);
                if (board.isEmpty(twoStep)) {
                    moves.emplace_back(m_position, twoStep, MoveType::PawnDouble);
                }
            }
        }
    }

    // Captures (diagonal)
    for (int df : {-1, 1}) {
        Square cap(m_position.file + df, m_position.rank + dir);
        if (!cap.isValid()) continue;

        Piece* p = board.getPieceAt(cap);
        if (p && p->getColor() != m_color) {
            if (cap.rank == promoteRank) {
                for (auto pt : {PieceType::Queen, PieceType::Rook,
                                 PieceType::Bishop, PieceType::Knight}) {
                    Move m(m_position, cap, MoveType::PromotionCapture);
                    m.promoteTo = pt;
                    moves.push_back(m);
                }
            } else {
                moves.emplace_back(m_position, cap, MoveType::Capture);
            }
        }

        // En passant
        auto ep = board.getEnPassantTarget();
        if (ep && *ep == cap) {
            moves.emplace_back(m_position, cap, MoveType::EnPassant);
        }
    }

    return moves;
}

} // namespace Chess
