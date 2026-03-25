#include "Board.h"
#include "PieceFactory.h"
#include <iostream>
#include <sstream>
#include <cassert>

namespace Chess {

Board::Board() {
    clear();
}

Board::Board(const Board& other) {
    *this = other;
}

Board& Board::operator=(const Board& other) {
    if (this == &other) return *this;
    for (int f = 0; f < 8; ++f)
        for (int r = 0; r < 8; ++r) {
            if (other.m_grid[f][r])
                m_grid[f][r] = other.m_grid[f][r]->clone();
            else
                m_grid[f][r] = nullptr;
        }
    m_enPassantTarget = other.m_enPassantTarget;
    m_castleRights    = other.m_castleRights;
    m_halfmoveClock   = other.m_halfmoveClock;
    return *this;
}

void Board::clear() {
    for (int f = 0; f < 8; ++f)
        for (int r = 0; r < 8; ++r)
            m_grid[f][r] = nullptr;
    m_enPassantTarget = std::nullopt;
    m_castleRights    = {};
    m_halfmoveClock   = 0;
}

void Board::setupStartPosition() {
    clear();
    // White pieces (rank 0 and 1)
    auto wp = PieceFactory::createStartingPieces(Color::White);
    for (auto& p : wp) {
        Square sq = p->getPosition();
        m_grid[sq.file][sq.rank] = std::move(p);
    }
    // Black pieces (rank 6 and 7)
    auto bp = PieceFactory::createStartingPieces(Color::Black);
    for (auto& p : bp) {
        Square sq = p->getPosition();
        m_grid[sq.file][sq.rank] = std::move(p);
    }
    m_castleRights = { true, true, true, true };
}

Piece* Board::getPieceAt(Square sq) const {
    if (!sq.isValid()) return nullptr;
    return m_grid[sq.file][sq.rank].get();
}

Piece* Board::getPieceAt(int file, int rank) const {
    return getPieceAt(Square(file, rank));
}

bool Board::isEmpty(Square sq) const {
    return getPieceAt(sq) == nullptr;
}

bool Board::isOccupiedByColor(Square sq, Color c) const {
    Piece* p = getPieceAt(sq);
    return p && p->getColor() == c;
}

void Board::placePiece(std::unique_ptr<Piece> piece, Square sq) {
    if (!sq.isValid()) return;
    piece->setPosition(sq);
    m_grid[sq.file][sq.rank] = std::move(piece);
}

std::unique_ptr<Piece> Board::removePiece(Square sq) {
    if (!sq.isValid()) return nullptr;
    return std::move(m_grid[sq.file][sq.rank]);
}

void Board::movePieceInternal(Square from, Square to) {
    if (!from.isValid() || !to.isValid()) return;
    m_grid[to.file][to.rank]   = std::move(m_grid[from.file][from.rank]);
    m_grid[to.file][to.rank]->setPosition(to);
    m_grid[to.file][to.rank]->setHasMoved(true);
}

void Board::revokeCastleRight(Color c, bool kingside) {
    if (c == Color::White) {
        if (kingside) m_castleRights.whiteKingside  = false;
        else          m_castleRights.whiteQueenside = false;
    } else {
        if (kingside) m_castleRights.blackKingside  = false;
        else          m_castleRights.blackQueenside = false;
    }
}

Square Board::findKing(Color color) const {
    for (int f = 0; f < 8; ++f)
        for (int r = 0; r < 8; ++r)
            if (m_grid[f][r] &&
                m_grid[f][r]->getType() == PieceType::King &&
                m_grid[f][r]->getColor() == color)
                return Square(f, r);
    return Square(-1, -1); // should never happen in valid game
}

std::vector<Piece*> Board::getPiecesOfColor(Color color) const {
    std::vector<Piece*> pieces;
    for (int f = 0; f < 8; ++f)
        for (int r = 0; r < 8; ++r)
            if (m_grid[f][r] && m_grid[f][r]->getColor() == color)
                pieces.push_back(m_grid[f][r].get());
    return pieces;
}

std::string Board::toFEN(Color activeColor, int halfmoveClock, int fullmoveNumber) const {
    std::ostringstream fen;

    // Piece placement
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece* p = m_grid[f][r].get();
            if (!p) {
                ++empty;
            } else {
                if (empty > 0) { fen << empty; empty = 0; }
                fen << p->getSymbol();
            }
        }
        if (empty > 0) fen << empty;
        if (r > 0) fen << '/';
    }

    // Active color
    fen << ' ' << (activeColor == Color::White ? 'w' : 'b');

    // Castling
    std::string castle;
    if (m_castleRights.whiteKingside)  castle += 'K';
    if (m_castleRights.whiteQueenside) castle += 'Q';
    if (m_castleRights.blackKingside)  castle += 'k';
    if (m_castleRights.blackQueenside) castle += 'q';
    fen << ' ' << (castle.empty() ? "-" : castle);

    // En passant
    fen << ' ' << (m_enPassantTarget ? m_enPassantTarget->toAlgebraic() : "-");

    fen << ' ' << halfmoveClock << ' ' << fullmoveNumber;
    return fen.str();
}

void Board::printToConsole() const {
    std::cout << "\n  a b c d e f g h\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << ' ';
        for (int f = 0; f < 8; ++f) {
            Piece* p = m_grid[f][r].get();
            std::cout << (p ? p->getSymbol() : '.') << ' ';
        }
        std::cout << (r + 1) << '\n';
    }
    std::cout << "  a b c d e f g h\n\n";
}

} // namespace Chess
