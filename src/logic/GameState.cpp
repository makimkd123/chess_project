#include "GameState.h"
#include "AttackDetector.h"
#include "MoveGenerator.h"

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