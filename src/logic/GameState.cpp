#include "GameState.h"
#include "AttackDetector.h"
#include "MoveGenerator.h"
#include <cstdlib>

void GameState::switchSideToMove()
{
    if (sideToMove == PieceColor::White) {
        sideToMove = PieceColor::Black;
    } else {
        sideToMove = PieceColor::White;
        ++fullmoveNumber;
    }
}

void GameState::updateCastlingRights(const Move& move)
{
    const auto movingPiece = board.pieceAt(move.from);

    if (!movingPiece.has_value()) {
        return;
    }

    if (movingPiece->type == PieceType::King) {
        if (movingPiece->color == PieceColor::White) {
            castlingRights.whiteKingside = false;
            castlingRights.whiteQueenside = false;
        } else {
            castlingRights.blackKingside = false;
            castlingRights.blackQueenside = false;
        }
    }

    if (movingPiece->type == PieceType::Rook) {
        if (movingPiece->color == PieceColor::White) {
            if (move.from.file == 0 && move.from.rank == 0) {
                castlingRights.whiteQueenside = false;
            }

            if (move.from.file == 7 && move.from.rank == 0) {
                castlingRights.whiteKingside = false;
            }
        } else {
            if (move.from.file == 0 && move.from.rank == 7) {
                castlingRights.blackQueenside = false;
            }

            if (move.from.file == 7 && move.from.rank == 7) {
                castlingRights.blackKingside = false;
            }
        }
    }

    const auto capturedPiece = board.pieceAt(move.to);

    if (!capturedPiece.has_value()) {
        return;
    }

    if (capturedPiece->type != PieceType::Rook) {
        return;
    }

    if (capturedPiece->color == PieceColor::White) {
        if (move.to.file == 0 && move.to.rank == 0) {
            castlingRights.whiteQueenside = false;
        }

        if (move.to.file == 7 && move.to.rank == 0) {
            castlingRights.whiteKingside = false;
        }
    } else {
        if (move.to.file == 0 && move.to.rank == 7) {
            castlingRights.blackQueenside = false;
        }

        if (move.to.file == 7 && move.to.rank == 7) {
            castlingRights.blackKingside = false;
        }
    }
}

void GameState::updateEnPassantTarget(const Move& move)
{
    enPassantTarget = std::nullopt;

    const auto movingPiece = board.pieceAt(move.from);

    if (!movingPiece.has_value()) {
        return;
    }

    if (movingPiece->type != PieceType::Pawn) {
        return;
    }

    const int rankDifference =
        move.to.rank - move.from.rank;

    if (std::abs(rankDifference) != 2) {
        return;
    }

    const int middleRank =
        (move.from.rank + move.to.rank) / 2;

    enPassantTarget =
        Square{move.from.file, middleRank};
}
void GameState::updateMoveCounters(const Move& move)
{
    const auto movingPiece = board.pieceAt(move.from);
    const auto capturedPiece = board.pieceAt(move.to);

    const bool isPawnMove =
        movingPiece.has_value() &&
        movingPiece->type == PieceType::Pawn;

    const bool isCapture =
        capturedPiece.has_value() ||
        move.type == MoveType::EnPassant;

    if (isPawnMove || isCapture) {
        halfmoveClock = 0;
    } else {
        ++halfmoveClock;
    }
}

GameStatus GameState::getStatus() const {
    bool inCheck = AttackDetector::isKingInCheck(board,sideToMove);

    auto legalMoves = MoveGenerator::generateLegalMoves(board,sideToMove,castlingRights,enPassantTarget);

    if (legalMoves.empty()){
        if (inCheck){
            return GameStatus::Checkmate;
        }
        return GameStatus::Stalemate;
    }

    if (inCheck){
        return GameStatus::Check;
    }
    return GameStatus::Ongoing;
}

GameState GameState::startingPosition(){
    GameState state;

    state.board.setupStartingPosition();
    state.sideToMove = PieceColor::White;
    state.castlingRights = CastlingRights{true,true,true,true};
    state.enPassantTarget = std::nullopt;
    state.halfmoveClock=0;
    state.fullmoveNumber=1;

    return state;
}

bool GameState::applyMove(const Move& move)
{
    updateCastlingRights(move);
    updateEnPassantTarget(move);
    updateMoveCounters(move);

    const bool moveWorked = board.makeMove(move);

    if (!moveWorked) {
        return false;
    }

    switchSideToMove();

    return true;
}
