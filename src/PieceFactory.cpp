#include "PieceFactory.h"
#include <stdexcept>

namespace Chess {

std::unique_ptr<Piece> PieceFactory::create(PieceType type, Color color, Square pos) {
    switch (type) {
        case PieceType::King:   return std::make_unique<King>(color, pos);
        case PieceType::Queen:  return std::make_unique<Queen>(color, pos);
        case PieceType::Rook:   return std::make_unique<Rook>(color, pos);
        case PieceType::Bishop: return std::make_unique<Bishop>(color, pos);
        case PieceType::Knight: return std::make_unique<Knight>(color, pos);
        case PieceType::Pawn:   return std::make_unique<Pawn>(color, pos);
        default: throw std::invalid_argument("Cannot create piece of type None");
    }
}

std::vector<std::unique_ptr<Piece>> PieceFactory::createStartingPieces(Color color) {
    std::vector<std::unique_ptr<Piece>> pieces;
    int backRank  = (color == Color::White) ? 0 : 7;
    int pawnRank  = (color == Color::White) ? 1 : 6;

    // Back rank: R N B Q K B N R
    pieces.push_back(create(PieceType::Rook,   color, {0, backRank}));
    pieces.push_back(create(PieceType::Knight, color, {1, backRank}));
    pieces.push_back(create(PieceType::Bishop, color, {2, backRank}));
    pieces.push_back(create(PieceType::Queen,  color, {3, backRank}));
    pieces.push_back(create(PieceType::King,   color, {4, backRank}));
    pieces.push_back(create(PieceType::Bishop, color, {5, backRank}));
    pieces.push_back(create(PieceType::Knight, color, {6, backRank}));
    pieces.push_back(create(PieceType::Rook,   color, {7, backRank}));

    // Pawns
    for (int f = 0; f < 8; ++f)
        pieces.push_back(create(PieceType::Pawn, color, {f, pawnRank}));

    return pieces;
}

std::unique_ptr<Piece> PieceFactory::createPromotion(PieceType type, Color color, Square pos) {
    if (type == PieceType::King || type == PieceType::Pawn || type == PieceType::None)
        throw std::invalid_argument("Invalid promotion piece type");
    auto p = create(type, color, pos);
    p->setHasMoved(true);
    return p;
}

} // namespace Chess
