#pragma once
#include "Types.h"
#include "Board.h"
#include <vector>
#include <optional>
#include <string>
#include <memory>
#include <map>

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>
#endif

namespace Chess {

struct RenderConfig {
    float squareSize  = 80.f;
    float boardOffset = 20.f;
    bool  flipped     = false;
#ifdef HAS_SFML
    sf::Color lightSquare   = sf::Color(240, 217, 181);
    sf::Color darkSquare    = sf::Color(181, 136,  99);
    sf::Color highlight     = sf::Color(205, 210,  50, 180);
    sf::Color legalMove     = sf::Color( 20, 180,  20, 120);
    sf::Color lastMoveFrom  = sf::Color(155, 199,   0, 160);
    sf::Color lastMoveTo    = sf::Color(155, 199,   0, 160);
    sf::Color checkSquare   = sf::Color(220,  50,  50, 180);
#endif
};

class BoardRenderer {
public:
    explicit BoardRenderer(const RenderConfig& config = {});

    void flipBoard() { m_config.flipped = !m_config.flipped; }

#ifdef HAS_SFML
    bool loadTextures(const std::string& assetPath);

    void render(sf::RenderWindow& window,
                const Board& board,
                const std::vector<Move>& legalMoves,
                std::optional<Square> selectedSquare,
                std::optional<Square> lastMoveFrom,
                std::optional<Square> lastMoveTo,
                std::optional<Square> kingInCheck);

    std::optional<Square> pixelToSquare(sf::Vector2i pixel) const;
    sf::Vector2f          squareToPixel(Square sq) const;

    void setDragPiece(Piece* piece, sf::Vector2f pos);
    void clearDragPiece();
#endif

private:
    RenderConfig m_config;

#ifdef HAS_SFML
    void drawBoard(sf::RenderWindow& window);
    void drawCoordinates(sf::RenderWindow& window);
    void drawHighlights(sf::RenderWindow& window,
                        const std::vector<Move>& legalMoves,
                        std::optional<Square> selected,
                        std::optional<Square> lastFrom,
                        std::optional<Square> lastTo,
                        std::optional<Square> check);
    void drawPieces(sf::RenderWindow& window, const Board& board);
    void drawDraggedPiece(sf::RenderWindow& window);
    sf::Sprite makePieceSprite(const Piece* piece, sf::Vector2f pos) const;

    std::map<std::string, sf::Texture>   m_textures;
    sf::Font                             m_font;
    Piece*                               m_dragPiece = nullptr;
    sf::Vector2f                         m_dragPos;
#else
    Piece* m_dragPiece = nullptr;
#endif
};

} // namespace Chess
