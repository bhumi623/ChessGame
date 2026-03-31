#pragma once
#include "Types.h"
#include "Piece.h"
#include <memory>

namespace Chess {

// ─── PieceFactory ────────────────────────────────────────────────────────────
// Factory Pattern: centralises piece creation.
// Clients never call "new King(...)" directly — always go through the factory.

class PieceFactory {
public:
    // Create any piece by type
    static std::unique_ptr<Piece> create(PieceType type, Color color, Square pos);

    // Convenience: create a full starting set for one color
    // Returns pieces in standard starting positions
    static std::vector<std::unique_ptr<Piece>> createStartingPieces(Color color);

    // Create a promotion piece (Queen/Rook/Bishop/Knight only)
    static std::unique_ptr<Piece> createPromotion(PieceType type, Color color, Square pos);

private:
    PieceFactory() = delete; // static-only class
};

} // namespace Chess
