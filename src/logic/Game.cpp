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

void Game::switchSideToMove() {
    if (state_.sideToMove == PieceColor::White) {
        state_.sideToMove = PieceColor::Black;
    } else {
        state_.sideToMove = PieceColor::White;
        ++state_.fullmoveNumber;
    }
}

void Game::updateCastlingRights(const Move& move) {
    auto movingPiece = state_.board.pieceAt(move.from);

    if (!movingPiece.has_value()) {
        return;
    }

    if (movingPiece->type == PieceType::King) {
        if (movingPiece->color == PieceColor::White) {
            state_.castlingRights.whiteKingside = false;
            state_.castlingRights.whiteQueenside = false;
        } else {
            state_.castlingRights.blackKingside = false;
            state_.castlingRights.blackQueenside = false;
        }
    }

    if (movingPiece->type == PieceType::Rook) {
        if (movingPiece->color == PieceColor::White) {
            if (move.from.file == 0 && move.from.rank == 0) {
                state_.castlingRights.whiteQueenside = false;
            }

            if (move.from.file == 7 && move.from.rank == 0) {
                state_.castlingRights.whiteKingside = false;
            }
        } else {
            if (move.from.file == 0 && move.from.rank == 7) {
                state_.castlingRights.blackQueenside = false;
            }

            if (move.from.file == 7 && move.from.rank == 7) {
                state_.castlingRights.blackKingside = false;
            }
        }
    }

    auto capturedPiece = state_.board.pieceAt(move.to);

    if (!capturedPiece.has_value()) {
        return;
    }

    if (capturedPiece->type != PieceType::Rook) {
        return;
    }

    if (capturedPiece->color == PieceColor::White) {
        if (move.to.file == 0 && move.to.rank == 0) {
            state_.castlingRights.whiteQueenside = false;
        }

        if (move.to.file == 7 && move.to.rank == 0) {
            state_.castlingRights.whiteKingside = false;
        }
    } else {
        if (move.to.file == 0 && move.to.rank == 7) {
            state_.castlingRights.blackQueenside = false;
        }

        if (move.to.file == 7 && move.to.rank == 7) {
            state_.castlingRights.blackKingside = false;
        }
    }
}

void Game::updateEnPassantTarget(const Move& move) {
    state_.enPassantTarget = std::nullopt;

    auto movingPiece = state_.board.pieceAt(move.from);

    if (!movingPiece.has_value()) {
        return;
    }

    if (movingPiece->type != PieceType::Pawn) {
        return;
    }

    int rankDifference = move.to.rank - move.from.rank;

    if (std::abs(rankDifference) != 2) {
        return;
    }

    int middleRank = (move.from.rank + move.to.rank) / 2;

    state_.enPassantTarget = Square{move.from.file, middleRank};
}

void Game::updateMoveCounters(const Move& move) {
    auto movingPiece = state_.board.pieceAt(move.from);
    auto capturedPiece = state_.board.pieceAt(move.to);

    bool isPawnMove =
        movingPiece.has_value() &&
        movingPiece->type == PieceType::Pawn;

    bool isCapture =
        capturedPiece.has_value() ||
        move.type == MoveType::EnPassant;

    if (isPawnMove || isCapture) {
        state_.halfmoveClock = 0;
    } else {
        ++state_.halfmoveClock;
    }
}