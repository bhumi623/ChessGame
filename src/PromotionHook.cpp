// ============================================================
//  PASTE THIS INTO YOUR EVENT LOOP  (main.cpp or UIManager.cpp)
//  inside the sf::Event::MouseButtonPressed handler
// ============================================================
//
// The full pattern looks like this:
//
//   while (window.pollEvent(event)) {
//       if (event.type == sf::Event::MouseButtonPressed
//           && event.mouseButton.button == sf::Mouse::Left)
//       {
//           handleMouseClick(sf::Mouse::getPosition(window));
//       }
//   }
//   // And in your render loop, AFTER drawing pieces:
//   if (game.getGameState() == GameState::Promotion) {
//       renderer.renderPromotionDialog(window, game.getActiveColor());
//   }
//
// ──────────────────────────────────────────────────────────────

void handleMouseClick(sf::Vector2i mousePos)
{
    auto& game     = GameManager::getInstance();
    auto& renderer = /* your BoardRenderer instance */;

    // ── PROMOTION STATE: only route clicks to the dialog ──────────────────
    if (game.getGameState() == GameState::Promotion)
    {
        PieceType chosen = renderer.getPromotionChoice(
            mousePos,
            game.getActiveColor()   // needed to look up correct texture key
        );

        if (chosen != PieceType::None)
            game.tryPromote(chosen);

        return; // swallow the click — don't fall through to normal move logic
    }

    // ── NORMAL CLICK: select / deselect / move ─────────────────────────────
    auto clickedSq = renderer.pixelToSquare(mousePos);
    if (!clickedSq) return;

    if (!m_selectedSquare)
    {
        // First click — select a piece that belongs to the active player
        auto moves = game.getLegalMovesForSquare(*clickedSq);
        if (!moves.empty()) {
            m_selectedSquare = clickedSq;
            m_legalMoves     = moves;
        }
    }
    else
    {
        // Second click — attempt the move
        Move attempt(*m_selectedSquare, *clickedSq);
        bool accepted = game.tryMove(attempt);

        // If tryMove() returned true AND state is now Promotion,
        // the game is waiting for the player to pick a piece.
        // renderPromotionDialog() will be called automatically
        // in the render loop (see above).

        m_selectedSquare = std::nullopt;
        m_legalMoves.clear();

        // If the move was rejected, check if the player clicked
        // a different friendly piece (re-select)
        if (!accepted) {
            auto moves = game.getLegalMovesForSquare(*clickedSq);
            if (!moves.empty()) {
                m_selectedSquare = clickedSq;
                m_legalMoves     = moves;
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────
// RENDER LOOP — call this every frame after drawPieces():
// ──────────────────────────────────────────────────────────────

void renderFrame(sf::RenderWindow& window)
{
    auto& game     = GameManager::getInstance();
    auto& renderer = /* your BoardRenderer instance */;

    renderer.render(
        window,
        game.getBoard(),
        m_legalMoves,
        m_selectedSquare,
        m_lastMoveFrom,
        m_lastMoveTo,
        /* kingInCheck square if any */
    );

    // Draw promotion UI on top of everything else
    if (game.getGameState() == GameState::Promotion)
        renderer.renderPromotionDialog(window, game.getActiveColor());

    window.display();
}
