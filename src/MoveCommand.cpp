#include "MoveCommand.h"
#include "PieceFactory.h"
#include <cassert>

namespace Chess {

MoveCommand::MoveCommand(Move move, Color color)
    : m_move(move), m_color(color) {}

void MoveCommand::execute(Board& board) {
    // Save state for undo
    m_prevEnPassantTarget = board.getEnPassantTarget();
    m_prevCastleRights    = board.getCastleRights();
    m_prevHalfmoveClock   = board.getHalfmoveClock();

    Piece* initialMover = board.getPieceAt(m_move.from);
    if (initialMover && !initialMover->hasMoved()) {
        m_wasFirstMove = true;
    }

    board.setEnPassantTarget(std::nullopt); // clear by default; PawnDouble sets it

    switch (m_move.type) {
        case MoveType::CastleKingside:
        case MoveType::CastleQueenside:
            executeCastle(board, m_move.type == MoveType::CastleKingside);
            break;
        case MoveType::EnPassant:
            executeEnPassant(board);
            break;
        case MoveType::Promotion:
        case MoveType::PromotionCapture:
            executePromotion(board);
            break;
        default:
            executeNormal(board);
            break;
    }

    // Revoke castle rights when king or rook moves
    Piece* mover = board.getPieceAt(m_move.to);
    if (mover) {
        if (mover->getType() == PieceType::King) {
            board.revokeCastleRight(m_color, true);
            board.revokeCastleRight(m_color, false);
        } else if (mover->getType() == PieceType::Rook) {
            int backRank = (m_color == Color::White) ? 0 : 7;
            if (m_move.from.file == 7 && m_move.from.rank == backRank)
                board.revokeCastleRight(m_color, true);
            if (m_move.from.file == 0 && m_move.from.rank == backRank)
                board.revokeCastleRight(m_color, false);
        }
    }

    // Also revoke if opponent's rook is captured at its starting square
    Color enemy = (m_color == Color::White) ? Color::Black : Color::White;
    int eBackRank = (enemy == Color::White) ? 0 : 7;
    if (m_move.to.rank == eBackRank) {
        if (m_move.to.file == 7) board.revokeCastleRight(enemy, true);
        if (m_move.to.file == 0) board.revokeCastleRight(enemy, false);
    }
}

void MoveCommand::executeNormal(Board& board) {
    // Capture: remove target piece
    Piece* target = board.getPieceAt(m_move.to);
    if (target && target->getColor() != m_color) {
        m_capturedPiece = board.removePiece(m_move.to);
        board.resetHalfmoveClock();
    }

    // Halfmove clock: reset on pawn move
    Piece* mover = board.getPieceAt(m_move.from);
    if (mover && mover->getType() == PieceType::Pawn)
        board.resetHalfmoveClock();
    else if (!m_capturedPiece)
        board.setHalfmoveClock(board.getHalfmoveClock() + 1);

    // Pawn double push: set en passant target
    if (m_move.type == MoveType::PawnDouble) {
        int epRank = (m_color == Color::White) ? 2 : 5;
        board.setEnPassantTarget(Square(m_move.from.file, epRank));
    }

    board.movePieceInternal(m_move.from, m_move.to);
}

void MoveCommand::executeCastle(Board& board, bool kingside) {
    int rank    = (m_color == Color::White) ? 0 : 7;
    int rookFrom = kingside ? 7 : 0;
    int rookTo   = kingside ? 5 : 3;

    board.movePieceInternal(m_move.from, m_move.to); // king
    board.movePieceInternal(Square(rookFrom, rank), Square(rookTo, rank));

    board.setHalfmoveClock(board.getHalfmoveClock() + 1);
}

void MoveCommand::executeEnPassant(Board& board) {
    // Remove the captured pawn (beside us, not on destination)
    int capturedRank = (m_color == Color::White) ? m_move.to.rank - 1 : m_move.to.rank + 1;
    Square capturedSq(m_move.to.file, capturedRank);
    m_enPassantCapture = board.removePiece(capturedSq);

    board.movePieceInternal(m_move.from, m_move.to);
    board.resetHalfmoveClock();
}

void MoveCommand::executePromotion(Board& board) {
    // Capture if needed
    Piece* target = board.getPieceAt(m_move.to);
    if (target && target->getColor() != m_color) {
        m_capturedPiece = board.removePiece(m_move.to);
    }

    // Remove pawn
    board.removePiece(m_move.from);

    // Place promoted piece
    PieceType promoteType = (m_move.promoteTo != PieceType::None)
                              ? m_move.promoteTo
                              : PieceType::Queen;
    auto newPiece = PieceFactory::createPromotion(promoteType, m_color, m_move.to);
    board.placePiece(std::move(newPiece), m_move.to);

    m_wasPromotion = true;
    m_promotedTo   = promoteType;
    board.resetHalfmoveClock();
}

void MoveCommand::undo(Board& board) {
    // Restore state
    board.setEnPassantTarget(m_prevEnPassantTarget);
    board.setCastleRights(m_prevCastleRights);
    board.setHalfmoveClock(m_prevHalfmoveClock);

    switch (m_move.type) {
        case MoveType::CastleKingside:
        case MoveType::CastleQueenside: {
            bool kingside = (m_move.type == MoveType::CastleKingside);
            int rank     = (m_color == Color::White) ? 0 : 7;
            int rookFrom = kingside ? 7 : 0;
            int rookTo   = kingside ? 5 : 3;
            // Move king back
            auto king = board.removePiece(m_move.to);
            king->setHasMoved(false);
            board.placePiece(std::move(king), m_move.from);
            // Move rook back
            auto rook = board.removePiece(Square(rookTo, rank));
            rook->setHasMoved(false);
            board.placePiece(std::move(rook), Square(rookFrom, rank));
            break;
        }
        case MoveType::EnPassant: {
            auto pawn = board.removePiece(m_move.to);
            pawn->setHasMoved(m_prevHalfmoveClock > 0); // approximate
            board.placePiece(std::move(pawn), m_move.from);
            // Restore captured pawn
            if (m_enPassantCapture) {
                int capturedRank = (m_color == Color::White)
                                     ? m_move.to.rank - 1 : m_move.to.rank + 1;
                board.placePiece(std::move(m_enPassantCapture),
                                  Square(m_move.to.file, capturedRank));
            }
            break;
        }
        case MoveType::Promotion:
        case MoveType::PromotionCapture: {
            // Remove promoted piece, restore pawn
            board.removePiece(m_move.to);
            auto pawn = PieceFactory::create(PieceType::Pawn, m_color, m_move.from);
            pawn->setHasMoved(true);
            board.placePiece(std::move(pawn), m_move.from);
            if (m_capturedPiece)
                board.placePiece(std::move(m_capturedPiece), m_move.to);
            break;
        }
        default: {
            auto piece = board.removePiece(m_move.to);
            if (piece) {
                // Restore hasMoved if it was the first move
                if (m_wasFirstMove) piece->setHasMoved(false);
                board.placePiece(std::move(piece), m_move.from);
            }
            if (m_capturedPiece)
                board.placePiece(std::move(m_capturedPiece), m_move.to);
            break;
        }
    }
}

} // namespace Chess
