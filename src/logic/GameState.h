#pragma once

#include "models/Board.h"
#include "models/Pieces.h"
#include "models/Move.h"
#include "models/Square.h"
#include "CastlingRights.h"

#include <optional>

enum class GameStatus {
    Ongoing,
    Check,
    Checkmate,
    Stalemate
};

class GameState {
public:
    Board board;
    PieceColor sideToMove = PieceColor::White;
    CastlingRights castlingRights{};

    std::optional<Square> enPassantTarget = std::nullopt;

    int halfmoveClock = 0;
    int fullmoveNumber = 1;

    GameStatus getStatus() const;

    static GameState startingPosition();

    bool applyMove(const Move& move);

private:
    void switchSideToMove();
    void updateCastlingRights(const Move& move);
    void updateEnPassantTarget(const Move& move);
    void updateMoveCounters(const Move& move);
};