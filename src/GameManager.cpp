#include "GameManager.h"
#include "MoveValidator.h"
#include <algorithm>
#include <sstream>

namespace Chess {

// ─── Singleton ───────────────────────────────────────────────────────────────

GameManager& GameManager::getInstance() {
    static GameManager instance;
    return instance;
}

GameManager::GameManager() {
    newGame();
}

// ─── Game Lifecycle ──────────────────────────────────────────────────────────

void GameManager::newGame() {
    m_board.setupStartPosition();
    m_activeColor    = Color::White;
    m_gameState      = GameState::Playing;
    m_fullmoveNumber = 1;
    m_history.clear();
    m_positionHistory.clear();
    m_pendingPromotionSquare = std::nullopt;
    m_pendingPromoMove       = std::nullopt;
    m_positionHistory.push_back(getCurrentFEN());

    Move dummy;
    notify(GameEvent::BoardChanged, dummy);
}

void GameManager::resign([[maybe_unused]] Color color) {
    m_gameState = GameState::Checkmate; // treated as loss
    Move dummy;
    notify(GameEvent::CheckmateDetected, dummy);
}

// ─── Move Interface ──────────────────────────────────────────────────────────

std::vector<Move> GameManager::getLegalMovesForSquare(Square sq) const {
    Piece* p = m_board.getPieceAt(sq);
    if (!p || p->getColor() != m_activeColor) return {};
    return MoveValidator::getLegalMovesForPiece(m_board, p);
}

bool GameManager::tryMove(Move move) {
    if (m_gameState == GameState::Checkmate ||
        m_gameState == GameState::Stalemate ||
        m_gameState == GameState::Draw)
        return false;

    if (m_gameState == GameState::Promotion) return false;

    // Find the matching legal move (to get full MoveType etc.)
    auto legal = getLegalMovesForSquare(move.from);
    Move* matched = nullptr;
    for (auto& lm : legal) {
        if (lm.to == move.to) {
            // If a specific promoteTo was requested, match it
            if ((lm.type == MoveType::Promotion || lm.type == MoveType::PromotionCapture) &&
                move.promoteTo != PieceType::None) {
                if (lm.promoteTo == move.promoteTo) { matched = &lm; break; }
            } else {
                matched = &lm;
                break;
            }
        }
    }
    if (!matched) return false;

    // Build SAN before applying
    matched->san = MoveValidator::buildSAN(m_board, *matched, m_activeColor);

    // Handle promotion — if promoteTo is None, we need to ask UI
    if ((matched->type == MoveType::Promotion ||
         matched->type == MoveType::PromotionCapture) &&
         matched->promoteTo == PieceType::Queen) // default, or let UI override
    {
        if (move.promoteTo == PieceType::None) {
            // Store the move and switch to Promotion state for UI to pick
            m_pendingPromoMove       = *matched;
            m_pendingPromotionSquare = matched->to;
            m_gameState              = GameState::Promotion;
            Move dummy; notify(GameEvent::BoardChanged, dummy);
            return true; // "accepted" but pending
        }
        matched->promoteTo = move.promoteTo;
    }

    applyMove(*matched);
    return true;
}

bool GameManager::tryPromote(PieceType type) {
    if (m_gameState != GameState::Promotion) return false;
    if (!m_pendingPromoMove) return false;

    m_pendingPromoMove->promoteTo = type;
    m_pendingPromoMove->san = MoveValidator::buildSAN(m_board, *m_pendingPromoMove, m_activeColor);
    applyMove(*m_pendingPromoMove);

    m_pendingPromoMove       = std::nullopt;
    m_pendingPromotionSquare = std::nullopt;
    return true;
}

// Private helper
void GameManager::applyMove(const Move& move) {
    auto cmd = std::make_unique<MoveCommand>(move, m_activeColor);
    GameEvent evt = GameEvent::BoardChanged;
    if (move.type == MoveType::Capture || move.type == MoveType::PromotionCapture)
        evt = GameEvent::PieceCapture;
    else if (move.type == MoveType::EnPassant)
        evt = GameEvent::EnPassant;
    else if (move.type == MoveType::CastleKingside || move.type == MoveType::CastleQueenside)
        evt = GameEvent::Castled;
    else if (move.type == MoveType::Promotion)
        evt = GameEvent::PawnPromoted;

    cmd->execute(m_board);
    m_history.push_back(std::move(cmd));

    if (m_activeColor == Color::Black) ++m_fullmoveNumber;
    switchTurn();
    m_positionHistory.push_back(getCurrentFEN());
    updateGameState();
    notify(evt, move);
}

bool GameManager::undoLastMove() {
    if (m_history.empty()) return false;

    // Undo two half-moves (full move) unless only one exists
    auto undoOne = [&]() {
        m_history.back()->undo(m_board);
        m_history.pop_back();
        m_positionHistory.pop_back();
        switchTurn();
        if (m_activeColor == Color::Black && m_fullmoveNumber > 1)
            --m_fullmoveNumber;
    };

    undoOne();
    // If it was black's turn (we just undid white's move), undo one more
    if (!m_history.empty() && m_activeColor == Color::Black)
        undoOne();

    m_gameState              = MoveValidator::detectGameState(m_board, m_activeColor);
    m_pendingPromotionSquare = std::nullopt;
    m_pendingPromoMove       = std::nullopt;

    Move dummy; notify(GameEvent::BoardChanged, dummy);
    return true;
}

// ─── Observer ────────────────────────────────────────────────────────────────

void GameManager::subscribe(EventCallback cb) {
    m_subscribers.push_back(cb);
}

void GameManager::unsubscribe() {
    m_subscribers.clear();
}

void GameManager::notify(GameEvent event, const Move& move) {
    for (auto& cb : m_subscribers)
        cb(event, move);
}

// ─── State Updates ───────────────────────────────────────────────────────────

void GameManager::switchTurn() {
    m_activeColor = (m_activeColor == Color::White) ? Color::Black : Color::White;
    notify(GameEvent::TurnChanged, Move{});
}

void GameManager::updateGameState() {
    // Check repetition / 50-move
    if (isThreefoldRepetition()) {
        m_gameState = GameState::Draw;
        notify(GameEvent::DrawDetected, Move{});
        return;
    }
    if (m_board.getHalfmoveClock() >= 100) {
        m_gameState = GameState::Draw;
        notify(GameEvent::DrawDetected, Move{});
        return;
    }
    if (isInsufficientMaterial()) {
        m_gameState = GameState::Draw;
        notify(GameEvent::DrawDetected, Move{});
        return;
    }

    m_gameState = MoveValidator::detectGameState(m_board, m_activeColor);
    if (m_gameState == GameState::Check)
        notify(GameEvent::CheckDetected, Move{});
    else if (m_gameState == GameState::Checkmate)
        notify(GameEvent::CheckmateDetected, Move{});
    else if (m_gameState == GameState::Stalemate)
        notify(GameEvent::StalemateDetected, Move{});
}

bool GameManager::isGameOver() const {
    return m_gameState == GameState::Checkmate ||
           m_gameState == GameState::Stalemate ||
           m_gameState == GameState::Draw;
}

bool GameManager::isThreefoldRepetition() const {
    if (m_positionHistory.size() < 9) return false;
    const auto& cur = m_positionHistory.back();
    int count = 0;
    for (const auto& pos : m_positionHistory)
        if (pos == cur && ++count >= 3) return true;
    return false;
}

bool GameManager::isInsufficientMaterial() const {
    auto whites = m_board.getPiecesOfColor(Color::White);
    auto blacks = m_board.getPiecesOfColor(Color::Black);

    // KvK
    if (whites.size() == 1 && blacks.size() == 1) return true;

    // KBvK or KNvK
    auto onlyMinor = [](const std::vector<Piece*>& pieces) {
        if (pieces.size() != 2) return false;
        for (Piece* p : pieces)
            if (p->getType() != PieceType::King &&
                p->getType() != PieceType::Bishop &&
                p->getType() != PieceType::Knight) return false;
        return true;
    };
    if (onlyMinor(whites) && blacks.size() == 1) return true;
    if (onlyMinor(blacks) && whites.size() == 1) return true;

    return false;
}

// ─── PGN / FEN ───────────────────────────────────────────────────────────────

std::string GameManager::getCurrentFEN() const {
    return m_board.toFEN(m_activeColor,
                          m_board.getHalfmoveClock(),
                          m_fullmoveNumber);
}

std::string GameManager::getPGN() const {
    std::ostringstream pgn;
    pgn << "[Event \"Chess Game\"]\n";
    pgn << "[Date \"????.??.??\"]\n";
    pgn << "[White \"Player 1\"]\n";
    pgn << "[Black \"Player 2\"]\n";
    pgn << "[Result \"*\"]\n\n";

    int moveNum = 1;
    bool isWhite = true;
    for (const auto& cmd : m_history) {
        if (isWhite) pgn << moveNum++ << ". ";
        pgn << cmd->getSAN() << " ";
        isWhite = !isWhite;
    }
    pgn << "*";
    return pgn.str();
}

int GameManager::getPositionRepetitionCount(const std::string& fen) const {
    int count = 0;
    for (const auto& pos : m_positionHistory)
        if (pos == fen) ++count;
    return count;
}

// Private applyMove trampoline (declared in header via lambda usage)
// We need to define it as a member — declared inline via header already.

} // namespace Chess
