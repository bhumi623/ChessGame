#include "BoardRenderer.h"
#include <iostream>
#include <cmath>

namespace Chess {

BoardRenderer::BoardRenderer(const RenderConfig& config)
    : m_config(config) {}

#ifdef HAS_SFML

bool BoardRenderer::loadTextures(const std::string& assetPath) {
    // Expected filenames: wK.png wQ.png wR.png wB.png wN.png wP.png
    //                      bK.png bQ.png bR.png bB.png bN.png bP.png
    const std::vector<std::pair<std::string, std::string>> pieces = {
        {"wK","wK"}, {"wQ","wQ"}, {"wR","wR"}, {"wB","wB"}, {"wN","wN"}, {"wP","wP"},
        {"bK","bK"}, {"bQ","bQ"}, {"bR","bR"}, {"bB","bB"}, {"bN","bN"}, {"bP","bP"}
    };

    bool allLoaded = true;
    for (auto& [key, file] : pieces) {
        sf::Texture tex;
        std::string path = assetPath + "/" + file + ".png";
        if (!tex.loadFromFile(path)) {
            std::cerr << "Warning: Could not load texture: " << path << "\n";
            allLoaded = false;
        } else {
            tex.setSmooth(true);
            m_textures[key] = std::move(tex);
        }
    }

    if (!m_font.loadFromFile(assetPath + "/font.ttf")) {
        // Try system fallback fonts
        if (!m_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
            m_font.loadFromFile("/System/Library/Fonts/Helvetica.ttc");
    }
    return allLoaded;
}

// ─── Main Render ─────────────────────────────────────────────────────────────

void BoardRenderer::render(sf::RenderWindow& window,
                            const Board& board,
                            const std::vector<Move>& legalMoves,
                            std::optional<Square> selectedSquare,
                            std::optional<Square> lastMoveFrom,
                            std::optional<Square> lastMoveTo,
                            std::optional<Square> kingInCheck) {
    drawBoard(window);
    drawHighlights(window, legalMoves, selectedSquare, lastMoveFrom, lastMoveTo, kingInCheck);
    drawPieces(window, board);
    drawCoordinates(window);
    drawDraggedPiece(window);
}

// ─── Board Squares ───────────────────────────────────────────────────────────

void BoardRenderer::drawBoard(sf::RenderWindow& window) {
    float sz  = m_config.squareSize;
    float off = m_config.boardOffset;

    for (int f = 0; f < 8; ++f) {
        for (int r = 0; r < 8; ++r) {
            bool light = (f + r) % 2 == 0;
            sf::RectangleShape sq(sf::Vector2f(sz, sz));

            int drawF = m_config.flipped ? (7 - f) : f;
            int drawR = m_config.flipped ? r       : (7 - r);
            sq.setPosition(off + drawF * sz, off + drawR * sz);
            sq.setFillColor(light ? m_config.lightSquare : m_config.darkSquare);
            window.draw(sq);
        }
    }

    // Board border
    sf::RectangleShape border(sf::Vector2f(sz * 8, sz * 8));
    border.setPosition(off, off);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(40, 40, 40, 200));
    border.setOutlineThickness(2.f);
    window.draw(border);
}

void BoardRenderer::drawCoordinates(sf::RenderWindow& window) {
    if (m_textures.empty()) return; // no font loaded if textures failed
    float sz  = m_config.squareSize;
    float off = m_config.boardOffset;
    unsigned int fontSize = static_cast<unsigned int>(sz * 0.18f);

    for (int i = 0; i < 8; ++i) {
        // File letters (a-h)
        sf::Text fileLabel;
        fileLabel.setFont(m_font);
        fileLabel.setCharacterSize(fontSize);
        int drawF = m_config.flipped ? (7 - i) : i;
        fileLabel.setString(std::string(1, static_cast<char>('a' + i)));
        fileLabel.setFillColor((i % 2 == 0) ? m_config.darkSquare : m_config.lightSquare);
        fileLabel.setPosition(off + drawF * sz + sz - fontSize * 0.8f,
                               off + sz * 8 - fontSize * 1.3f);
        window.draw(fileLabel);

        // Rank numbers (1-8)
        sf::Text rankLabel;
        rankLabel.setFont(m_font);
        rankLabel.setCharacterSize(fontSize);
        int drawR = m_config.flipped ? i : (7 - i);
        rankLabel.setString(std::to_string(i + 1));
        rankLabel.setFillColor((i % 2 == 0) ? m_config.lightSquare : m_config.darkSquare);
        rankLabel.setPosition(off + 2.f, off + drawR * sz + 2.f);
        window.draw(rankLabel);
    }
}

// ─── Highlights ──────────────────────────────────────────────────────────────

void BoardRenderer::drawHighlights(sf::RenderWindow& window,
                                    const std::vector<Move>& legalMoves,
                                    std::optional<Square> selected,
                                    std::optional<Square> lastFrom,
                                    std::optional<Square> lastTo,
                                    std::optional<Square> check) {
    float sz  = m_config.squareSize;
    float off = m_config.boardOffset;

    auto squareRect = [&](Square sq) -> sf::RectangleShape {
        sf::RectangleShape r(sf::Vector2f(sz, sz));
        int drawF = m_config.flipped ? (7 - sq.file) : sq.file;
        int drawR = m_config.flipped ? sq.rank        : (7 - sq.rank);
        r.setPosition(off + drawF * sz, off + drawR * sz);
        return r;
    };

    // Last move highlight
    if (lastFrom) {
        auto r = squareRect(*lastFrom);
        r.setFillColor(m_config.lastMoveFrom);
        window.draw(r);
    }
    if (lastTo) {
        auto r = squareRect(*lastTo);
        r.setFillColor(m_config.lastMoveTo);
        window.draw(r);
    }

    // King in check
    if (check) {
        auto r = squareRect(*check);
        r.setFillColor(m_config.checkSquare);
        window.draw(r);
    }

    // Selected square
    if (selected) {
        auto r = squareRect(*selected);
        r.setFillColor(m_config.highlight);
        window.draw(r);
    }

    // Legal move dots / capture rings
    for (const auto& move : legalMoves) {
        int drawF = m_config.flipped ? (7 - move.to.file) : move.to.file;
        int drawR = m_config.flipped ? move.to.rank        : (7 - move.to.rank);
        float cx  = off + drawF * sz + sz * 0.5f;
        float cy  = off + drawR * sz + sz * 0.5f;

        bool isCapture = (move.type == MoveType::Capture ||
                          move.type == MoveType::EnPassant ||
                          move.type == MoveType::PromotionCapture);

        if (isCapture) {
            // Hollow ring for captures
            sf::CircleShape ring(sz * 0.46f);
            ring.setOrigin(sz * 0.46f, sz * 0.46f);
            ring.setPosition(cx, cy);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(m_config.legalMove);
            ring.setOutlineThickness(sz * 0.08f);
            window.draw(ring);
        } else {
            // Filled dot for normal moves
            sf::CircleShape dot(sz * 0.15f);
            dot.setOrigin(sz * 0.15f, sz * 0.15f);
            dot.setPosition(cx, cy);
            dot.setFillColor(m_config.legalMove);
            window.draw(dot);
        }
    }
}

// ─── Pieces ──────────────────────────────────────────────────────────────────

void BoardRenderer::drawPieces(sf::RenderWindow& window, const Board& board) {
    for (int f = 0; f < 8; ++f) {
        for (int r = 0; r < 8; ++r) {
            Piece* p = board.getPieceAt(Square(f, r));
            if (!p) continue;
            if (m_dragPiece == p) continue; // drawn separately on top

            int drawF = m_config.flipped ? (7 - f) : f;
            int drawR = m_config.flipped ? r        : (7 - r);
            float x   = m_config.boardOffset + drawF * m_config.squareSize;
            float y   = m_config.boardOffset + drawR * m_config.squareSize;

            auto sprite = makePieceSprite(p, sf::Vector2f(x, y));
            window.draw(sprite);
        }
    }
}

void BoardRenderer::drawDraggedPiece(sf::RenderWindow& window) {
    if (!m_dragPiece) return;
    auto sprite = makePieceSprite(m_dragPiece, m_dragPos);
    // Center on cursor
    float sz = m_config.squareSize;
    sprite.setPosition(m_dragPos.x - sz * 0.5f, m_dragPos.y - sz * 0.5f);
    window.draw(sprite);
}

sf::Sprite BoardRenderer::makePieceSprite(const Piece* piece, sf::Vector2f pos) const {
    std::string key;
    key += (piece->getColor() == Color::White) ? 'w' : 'b';
    switch (piece->getType()) {
        case PieceType::King:   key += 'K'; break;
        case PieceType::Queen:  key += 'Q'; break;
        case PieceType::Rook:   key += 'R'; break;
        case PieceType::Bishop: key += 'B'; break;
        case PieceType::Knight: key += 'N'; break;
        case PieceType::Pawn:   key += 'P'; break;
        default: break;
    }

    sf::Sprite sprite;
    auto it = m_textures.find(key);
    if (it != m_textures.end()) {
        sprite.setTexture(it->second);
        float sz       = m_config.squareSize;
        auto  texSize  = it->second.getSize();
        float scaleX   = sz / texSize.x;
        float scaleY   = sz / texSize.y;
        sprite.setScale(scaleX * 0.92f, scaleY * 0.92f);
        sprite.setPosition(pos.x + sz * 0.04f, pos.y + sz * 0.04f);
    } else {
        // Fallback: colored circle with letter
        // (no sprite to draw — handled by text fallback in real impl)
    }
    return sprite;
}



// ─── Coordinate Conversion ───────────────────────────────────────────────────

std::optional<Square> BoardRenderer::pixelToSquare(sf::Vector2i pixel) const {
    float sz  = m_config.squareSize;
    float off = m_config.boardOffset;

    float relX = pixel.x - off;
    float relY = pixel.y - off;

    if (relX < 0 || relY < 0 || relX >= sz * 8 || relY >= sz * 8)
        return std::nullopt;

    int drawF = static_cast<int>(relX / sz);
    int drawR = static_cast<int>(relY / sz);

    int file = m_config.flipped ? (7 - drawF) : drawF;
    int rank = m_config.flipped ? drawR        : (7 - drawR);

    return Square(file, rank);
}

sf::Vector2f BoardRenderer::squareToPixel(Square sq) const {
    float sz  = m_config.squareSize;
    float off = m_config.boardOffset;
    int drawF = m_config.flipped ? (7 - sq.file) : sq.file;
    int drawR = m_config.flipped ? sq.rank        : (7 - sq.rank);
    return sf::Vector2f(off + drawF * sz + sz * 0.5f,
                         off + drawR * sz + sz * 0.5f);
}

// ─── Drag ────────────────────────────────────────────────────────────────────

void BoardRenderer::setDragPiece(Piece* piece, sf::Vector2f pos) {
    m_dragPiece = piece;
    m_dragPos   = pos;
}

void BoardRenderer::clearDragPiece() {
    m_dragPiece = nullptr;
}

#endif // HAS_SFML

} // namespace Chess
