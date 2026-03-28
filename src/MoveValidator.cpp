#include "MoveValidator.h"
#include "MoveCommand.h"
#include <sstream>
#include <algorithm>

namespace Chess {

// ─── Public API ──────────────────────────────────────────────────────────────

std::vector<Move> MoveValidator::getLegalMoves(const Board& board, Color color) {
    std::vector<Move> legal;
    auto pieces = board.getPiecesOfColor(color);
    for (Piece* p : pieces) {
        auto pm = getLegalMovesForPiece(board, p);
        legal.insert(legal.end(), pm.begin(), pm.end());
    }
    return legal;
}

std::vector<Move> MoveValidator::getLegalMovesForPiece(const Board& board, const Piece* piece) {
    if (!piece) return {};

    auto pseudo = piece->getPseudoLegalMoves(board);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());

    for (auto& m : pseudo) {
        if (!leavesKingInCheck(board, m, piece->getColor()))
            legal.push_back(m);
    }

    // Add castling if it's a king
    if (piece->getType() == PieceType::King) {
        auto castles = getCastlingMoves(board, static_cast<const King*>(piece));
        legal.insert(legal.end(), castles.begin(), castles.end());
    }

    return legal;
}

bool MoveValidator::isMoveLegal(const Board& board, const Move& move, Color color) {
    auto legal = getLegalMoves(board, color);
    for (auto& m : legal) {
        if (m.from == move.from && m.to == move.to) {
            // For promotions, also match promoteTo if specified
            if ((move.type == MoveType::Promotion ||
                 move.type == MoveType::PromotionCapture) &&
                move.promoteTo != PieceType::None)
                return m.promoteTo == move.promoteTo;
            return true;
        }
    }
    return false;
}

bool MoveValidator::isInCheck(const Board& board, Color color) {
    Square king = board.findKing(color);
    if (!king.isValid()) return false;
    Color enemy = (color == Color::White) ? Color::Black : Color::White;
    return isSquareAttackedBy(board, king, enemy);
}

bool MoveValidator::isSquareAttackedBy(const Board& board, Square sq, Color attacker) {
    // Check all attacker pieces for pseudo-legal moves that land on sq
    auto pieces = board.getPiecesOfColor(attacker);
    for (Piece* p : pieces) {
        auto moves = p->getPseudoLegalMoves(board);
        for (auto& m : moves) {
            if (m.to == sq) return true;
        }
    }
    return false;
}

GameState MoveValidator::detectGameState(const Board& board, Color colorToMove) {
    bool inCheck  = isInCheck(board, colorToMove);
    auto legal    = getLegalMoves(board, colorToMove);
    bool noMoves  = legal.empty();

    if (noMoves && inCheck)  return GameState::Checkmate;
    if (noMoves && !inCheck) return GameState::Stalemate;
    if (inCheck)             return GameState::Check;
    return GameState::Playing;
}

// ─── Castling ────────────────────────────────────────────────────────────────

std::vector<Move> MoveValidator::getCastlingMoves(const Board& board, const King* king) {
    std::vector<Move> moves;
    if (!king || king->hasMoved()) return moves;

    Color  c        = king->getColor();
    Color  enemy    = (c == Color::White) ? Color::Black : Color::White;
    int    rank     = (c == Color::White) ? 0 : 7;
    auto   rights   = board.getCastleRights();

    // King must not be in check
    if (isInCheck(board, c)) return moves;

    // Kingside (files e=4, f=5, g=6, rook at h=7)
    bool ksRight = (c == Color::White) ? rights.whiteKingside : rights.blackKingside;
    if (ksRight) {
        Square f5(5, rank), f6(6, rank), rookSq(7, rank);
        Piece* rook = board.getPieceAt(rookSq);
        if (rook && rook->getType() == PieceType::Rook &&
            rook->getColor() == c && !rook->hasMoved() &&
            board.isEmpty(f5) && board.isEmpty(f6) &&
            !isSquareAttackedBy(board, f5, enemy) &&
            !isSquareAttackedBy(board, f6, enemy))
        {
            moves.emplace_back(king->getPosition(), f6, MoveType::CastleKingside);
        }
    }

    // Queenside (files d=3, c=2, b=1, rook at a=0)
    bool qsRight = (c == Color::White) ? rights.whiteQueenside : rights.blackQueenside;
    if (qsRight) {
        Square f3(3, rank), f2(2, rank), f1(1, rank), rookSq(0, rank);
        Piece* rook = board.getPieceAt(rookSq);
        if (rook && rook->getType() == PieceType::Rook &&
            rook->getColor() == c && !rook->hasMoved() &&
            board.isEmpty(f3) && board.isEmpty(f2) && board.isEmpty(f1) &&
            !isSquareAttackedBy(board, f3, enemy) &&
            !isSquareAttackedBy(board, f2, enemy))
        {
            moves.emplace_back(king->getPosition(), f2, MoveType::CastleQueenside);
        }
    }

    return moves;
}

// ─── SAN ─────────────────────────────────────────────────────────────────────

std::string MoveValidator::buildSAN(const Board& board, const Move& move, Color color) {
    Piece* p = board.getPieceAt(move.from);
    if (!p) return "??";

    std::ostringstream san;

    if (move.type == MoveType::CastleKingside)  { return "O-O"; }
    if (move.type == MoveType::CastleQueenside) { return "O-O-O"; }

    bool isPawn = (p->getType() == PieceType::Pawn);

    // Piece letter (not for pawns)
    if (!isPawn) {
        char sym = std::toupper(p->getSymbol());
        san << sym;
    }

    // Disambiguation: find other pieces of same type that can also go to target
    if (!isPawn) {
        auto allies = board.getPiecesOfColor(color);
        bool needFile = false, needRank = false;
        for (Piece* ally : allies) {
            if (ally == p || ally->getType() != p->getType()) continue;
            auto allyMoves = getLegalMovesForPiece(board, ally);
            for (auto& am : allyMoves) {
                if (am.to == move.to) {
                    if (ally->getPosition().file != p->getPosition().file) needFile = true;
                    else needRank = true;
                }
            }
        }
        if (needFile) san << static_cast<char>('a' + move.from.file);
        if (needRank) san << static_cast<char>('1' + move.from.rank);
    }

    // Capture
    bool isCapture = (move.type == MoveType::Capture ||
                      move.type == MoveType::EnPassant ||
                      move.type == MoveType::PromotionCapture);
    if (isCapture) {
        if (isPawn) san << static_cast<char>('a' + move.from.file);
        san << 'x';
    }

    // Destination
    san << move.to.toAlgebraic();

    // Promotion
    if (move.type == MoveType::Promotion || move.type == MoveType::PromotionCapture) {
        san << '=';
        switch (move.promoteTo) {
            case PieceType::Queen:  san << 'Q'; break;
            case PieceType::Rook:   san << 'R'; break;
            case PieceType::Bishop: san << 'B'; break;
            case PieceType::Knight: san << 'N'; break;
            default: break;
        }
    }

    if (move.type == MoveType::EnPassant) san << " e.p.";

    return san.str();
}

// ─── Internal: test move legality ────────────────────────────────────────────

bool MoveValidator::leavesKingInCheck(const Board& board, const Move& move, Color color) {
    // Apply move on a copy, then test for check
    Board copy = board;
    MoveCommand cmd(move, color);
    cmd.execute(copy);
    return isInCheck(copy, color);
}

} // namespace Chess
