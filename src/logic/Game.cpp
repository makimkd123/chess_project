#include "Game.h"

#include <cstdlib>

#include "MoveGenerator.h"

Game::Game()
    : state_(GameState::startingPosition())
{
}

const Board& Game::board() const {
    return state_.board;
}

GameStatus Game::status() const {
    return state_.getStatus();
}

PieceColor Game::sideToMove() const {
    return state_.sideToMove;
}

const std::vector<Move>& Game::moveHistory() const {
    return moveHistory_;
}

const CastlingRights& Game::castlingRights() const {
    return state_.castlingRights;
}

std::optional<Square> Game::enPassantTarget() const {
    return state_.enPassantTarget;
}

const GameState& Game::state() const {
    return state_;
}

GameState& Game::state() {
    return state_;
}

bool Game::makeMove(const Move& move)
{
    const auto legalMoves =
        MoveGenerator::generateLegalMoves(
            state_.board,
            state_.sideToMove,
            state_.castlingRights,
            state_.enPassantTarget
        );

    bool foundLegalMove = false;

    for (const Move& legalMove : legalMoves) {
        if (
            legalMove.from.file == move.from.file &&
            legalMove.from.rank == move.from.rank &&
            legalMove.to.file == move.to.file &&
            legalMove.to.rank == move.to.rank &&
            legalMove.type == move.type &&
            legalMove.promotionType == move.promotionType
        ) {
            foundLegalMove = true;
            break;
        }
    }

    if (!foundLegalMove) {
        return false;
    }

    if (!state_.applyMove(move)) {
        return false;
    }

    moveHistory_.push_back(move);

    return true;
}