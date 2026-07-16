#pragma once
#include <optional>
#include <vector>


#include "models/Board.h"
#include "models/Move.h"
#include "models/Pieces.h"
#include "models/Square.h"
#include "GameState.h"
#include "CastlingRights.h"

class Game {
    public:
        Game();

        const Board& board() const;
        PieceColor sideToMove() const;
        GameStatus status() const;
        const std::vector<Move>& moveHistory() const;
        const CastlingRights& castlingRights() const;
        std::optional<Square> enPassantTarget() const;
        const GameState& state() const;
        GameState& state();
        bool makeMove (const Move& move);
        explicit Game(GameState initialState);
        void reset(GameState initialState);

    private:
        GameState state_;
        std::vector<Move> moveHistory_;
};