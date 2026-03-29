#include "GameManager.h"
#include "BoardRenderer.h"
#include "UIManager.h"
#include "SoundManager.h"
#include "MoveValidator.h"
#include <iostream>
#include <string>

#ifdef HAS_SFML
#include <SFML/Graphics.hpp>

// ─── SFML Application ────────────────────────────────────────────────────────

static const float SQUARE_SIZE   = 80.f;
static const float BOARD_OFFSET  = 20.f;
static const float PANEL_WIDTH   = 240.f;
static const unsigned WINDOW_W   = static_cast<unsigned>(BOARD_OFFSET * 2 + SQUARE_SIZE * 8 + PANEL_WIDTH);
static const unsigned WINDOW_H   = static_cast<unsigned>(BOARD_OFFSET * 2 + SQUARE_SIZE * 8);

int runSFML() {
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_W, WINDOW_H),
        "Chess — C++ OOP",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(60);

    // ── Setup ──────────────────────────────────────────────────────────────
    Chess::RenderConfig cfg;
    cfg.squareSize  = SQUARE_SIZE;
    cfg.boardOffset = BOARD_OFFSET;

    Chess::BoardRenderer renderer(cfg);
    Chess::UIManager     ui;
    Chess::SoundManager  sounds;

    std::string assets = "assets";
    renderer.loadTextures(assets + "/pieces");
    ui.loadFont(assets + "/font.ttf");
    sounds.loadSounds(assets + "/sounds");

    auto& gm = Chess::GameManager::getInstance();
    gm.newGame();

    // ── Observer: forward events to SoundManager ───────────────────────────
    gm.subscribe([&](Chess::GameEvent evt, const Chess::Move&) {
        sounds.play(evt);
    });

    // ── Input state ────────────────────────────────────────────────────────
    std::optional<Chess::Square>          selectedSquare;
    std::vector<Chess::Move>              legalMoves;
    std::optional<Chess::Square>          lastMoveFrom, lastMoveTo;
    bool                                  dragging = false;
    Chess::Square                         dragOrigin;

    // ─── Main Loop ─────────────────────────────────────────────────────────
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {

            // ── Window Close ───────────────────────────────────────────────
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
                // Ctrl+Z = undo
                if (event.key.code == sf::Keyboard::Z &&
                    event.key.control) {
                    gm.undoLastMove();
                    selectedSquare = std::nullopt;
                    legalMoves.clear();
                    lastMoveFrom = lastMoveTo = std::nullopt;
                }
                // F = flip board
                if (event.key.code == sf::Keyboard::F)
                    renderer.flipBoard();
            }

            // ── Mouse Button Pressed ───────────────────────────────────────
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);

                // Promotion dialog active?
                if (gm.getGameState() == Chess::GameState::Promotion) {
                    Chess::PieceType choice = renderer.getPromotionChoice(
                        mousePos, gm.getActiveColor());
                    if (choice != Chess::PieceType::None) {
                        gm.tryPromote(choice);
                        selectedSquare = std::nullopt;
                        legalMoves.clear();
                    }
                    continue;
                }

                // UI panel buttons?
                auto btnResult = ui.handleClick(mousePos);
                if (btnResult.newGame) {
                    gm.newGame();
                    selectedSquare = std::nullopt;
                    legalMoves.clear();
                    lastMoveFrom = lastMoveTo = std::nullopt;
                    continue;
                }
                if (btnResult.undo) {
                    gm.undoLastMove();
                    selectedSquare = std::nullopt;
                    legalMoves.clear();
                    lastMoveFrom = lastMoveTo = std::nullopt;
                    continue;
                }
                if (btnResult.flip) { renderer.flipBoard(); continue; }
                if (btnResult.resign) { gm.resign(gm.getActiveColor()); continue; }

                // Board click?
                auto sq = renderer.pixelToSquare(mousePos);
                if (!sq) continue;

                Chess::Piece* piece = gm.getBoard().getPieceAt(*sq);

                if (selectedSquare) {
                    // Try to make the move
                    Chess::Move attemptedMove(*selectedSquare, *sq);

                    // Check if it's a legal move destination
                    bool moved = false;
                    for (auto& lm : legalMoves) {
                        if (lm.to == *sq) {
                            // For promotions with no explicit choice, default to Queen
                            // (promotion dialog will show if needed)
                            attemptedMove.promoteTo = lm.promoteTo;
                            if (gm.tryMove(attemptedMove)) {
                                lastMoveFrom = selectedSquare;
                                lastMoveTo   = sq;
                                moved = true;
                            }
                            break;
                        }
                    }
                    selectedSquare = std::nullopt;
                    legalMoves.clear();

                    if (!moved && piece && piece->getColor() == gm.getActiveColor()) {
                        // Re-select a different piece
                        selectedSquare = sq;
                        legalMoves     = gm.getLegalMovesForSquare(*sq);
                        // Start drag
                        dragging   = true;
                        dragOrigin = *sq;
                        renderer.setDragPiece(
                            gm.getBoard().getPieceAt(*sq),
                            sf::Vector2f(static_cast<float>(mousePos.x),
                                          static_cast<float>(mousePos.y)));
                    }
                } else {
                    if (piece && piece->getColor() == gm.getActiveColor() && !gm.isGameOver()) {
                        selectedSquare = sq;
                        legalMoves     = gm.getLegalMovesForSquare(*sq);
                        dragging       = true;
                        dragOrigin     = *sq;
                        renderer.setDragPiece(
                            piece,
                            sf::Vector2f(static_cast<float>(mousePos.x),
                                          static_cast<float>(mousePos.y)));
                    }
                }
            }

            // ── Mouse Move (drag) ──────────────────────────────────────────
            if (event.type == sf::Event::MouseMoved && dragging) {
                renderer.setDragPiece(
                    gm.getBoard().getPieceAt(dragOrigin),
                    sf::Vector2f(static_cast<float>(event.mouseMove.x),
                                  static_cast<float>(event.mouseMove.y)));
            }

            // ── Mouse Button Released (drop) ───────────────────────────────
            if (event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left && dragging)
            {
                dragging = false;
                renderer.clearDragPiece();

                if (gm.getGameState() == Chess::GameState::Promotion) continue;

                sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);
                auto dropSq = renderer.pixelToSquare(mousePos);

                if (dropSq && *dropSq != dragOrigin && selectedSquare) {
                    Chess::Move attemptedMove(dragOrigin, *dropSq);
                    for (auto& lm : legalMoves) {
                        if (lm.to == *dropSq) {
                            attemptedMove.promoteTo = lm.promoteTo;
                            if (gm.tryMove(attemptedMove)) {
                                lastMoveFrom = dragOrigin;
                                lastMoveTo   = dropSq;
                                selectedSquare = std::nullopt;
                                legalMoves.clear();
                            }
                            break;
                        }
                    }
                }
                // Keep selection if drop was invalid
            }
        }

        // ── Update ─────────────────────────────────────────────────────────
        ui.update(gm.getGameState(), gm.getActiveColor());

        // ── Find king in check ─────────────────────────────────────────────
        std::optional<Chess::Square> checkKing;
        if (gm.getGameState() == Chess::GameState::Check ||
            gm.getGameState() == Chess::GameState::Checkmate) {
            checkKing = gm.getBoard().findKing(gm.getActiveColor());
        }

        // ── Render ─────────────────────────────────────────────────────────
        window.clear(sf::Color(15, 15, 15));

        renderer.render(window, gm.getBoard(), legalMoves,
                         selectedSquare, lastMoveFrom, lastMoveTo, checkKing);

        ui.render(window, gm);

        if (gm.getGameState() == Chess::GameState::Promotion)
            renderer.renderPromotionDialog(window, gm.getActiveColor());

        window.display();
    }

    return 0;
}

#endif // HAS_SFML

// ─── Console Fallback ────────────────────────────────────────────────────────

int runConsole() {
    std::cout << "═══════════════════════════════════\n";
    std::cout << "         C++ Chess Engine\n";
    std::cout << "═══════════════════════════════════\n";
    std::cout << "Console mode (SFML not available)\n\n";

    auto& gm = Chess::GameManager::getInstance();
    gm.newGame();

    while (!gm.isGameOver()) {
        gm.getBoard().printToConsole();

        std::string activeStr = (gm.getActiveColor() == Chess::Color::White) ? "White" : "Black";

        switch (gm.getGameState()) {
            case Chess::GameState::Check:
                std::cout << activeStr << " is in CHECK!\n";
                break;
            case Chess::GameState::Checkmate:
                std::cout << "CHECKMATE! Game over.\n";
                return 0;
            case Chess::GameState::Stalemate:
                std::cout << "STALEMATE! Draw.\n";
                return 0;
            case Chess::GameState::Draw:
                std::cout << "DRAW!\n";
                return 0;
            default: break;
        }

        std::cout << activeStr << " to move.\n";
        std::cout << "Commands: <from><to> (e.g. e2e4)  |  'undo'  |  'pgn'  |  'fen'  |  'quit'\n> ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "quit" || input == "q") break;

        if (input == "undo") {
            gm.undoLastMove();
            continue;
        }
        if (input == "pgn") {
            std::cout << "\n" << gm.getPGN() << "\n\n";
            continue;
        }
        if (input == "fen") {
            std::cout << "\nFEN: " << gm.getCurrentFEN() << "\n\n";
            continue;
        }

        if (input.size() < 4) {
            std::cout << "Invalid input. Use format: e2e4\n";
            continue;
        }

        auto from = Chess::Square::fromAlgebraic(input.substr(0, 2));
        auto to   = Chess::Square::fromAlgebraic(input.substr(2, 2));

        if (!from || !to) {
            std::cout << "Invalid square. Use letters a-h and numbers 1-8.\n";
            continue;
        }

        Chess::Move move(*from, *to);

        // Handle promotion input (e.g., "e7e8q")
        if (input.size() >= 5) {
            char promo = std::tolower(input[4]);
            switch (promo) {
                case 'q': move.promoteTo = Chess::PieceType::Queen;  break;
                case 'r': move.promoteTo = Chess::PieceType::Rook;   break;
                case 'b': move.promoteTo = Chess::PieceType::Bishop; break;
                case 'n': move.promoteTo = Chess::PieceType::Knight; break;
                default:  break;
            }
        }

        if (!gm.tryMove(move)) {
            // If promotion pending, ask for piece
            if (gm.getGameState() == Chess::GameState::Promotion) {
                std::cout << "Promote to (q/r/b/n): ";
                std::string choice;
                std::getline(std::cin, choice);
                Chess::PieceType pt = Chess::PieceType::Queen;
                if (!choice.empty()) {
                    switch (std::tolower(choice[0])) {
                        case 'r': pt = Chess::PieceType::Rook;   break;
                        case 'b': pt = Chess::PieceType::Bishop; break;
                        case 'n': pt = Chess::PieceType::Knight; break;
                        default:  pt = Chess::PieceType::Queen;  break;
                    }
                }
                gm.tryPromote(pt);
            } else {
                std::cout << "Illegal move: " << input << "\n";
            }
        }
    }

    gm.getBoard().printToConsole();
    std::cout << "\nFinal PGN:\n" << gm.getPGN() << "\n";
    return 0;
}

// ─── Entry Point ─────────────────────────────────────────────────────────────

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
#ifdef HAS_SFML
    bool forceConsole = (argc > 1 && std::string(argv[1]) == "--console");
    if (!forceConsole)
        return runSFML();
#endif

    return runConsole();
}
