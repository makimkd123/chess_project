#pragma once

#include <vector>
#include <optional>
#include "models/Board.h"
#include "models/Move.h"
#include "models/Pieces.h"
#include "models/Square.h"
#include "CastlingRights.h"

class MoveGenerator {
public:

    static std::vector<Move> generatePseudoLegalMoves(
        const Board& board,
        PieceColor sideToMove,
        const CastlingRights& castlingRights,
        std::optional<Square> enPassantTarget
    );

    static std::vector<Move> generateLegalMoves(
        const Board& board, 
        PieceColor sideToMove,
        const CastlingRights& castlingRights,
        std::optional<Square> enPassantTarget
    );
private:
    static void generatePawnMoves(
        const Board& board,
        Square from,
        Piece piece,
        std::optional<Square> enPassantTarget,
        std::vector<Move>& moves
    );
    
    static void generateKnightMoves(
        const Board& board,
        Square from,
        Piece piece,
        std::vector<Move>& moves
    );

    static void generateSlidingMoves(
        const Board& board,
        Square from,
        Piece piece,
        const std::vector<Square>& directions,
        std::vector<Move>& moves
    );

    static void generateBishopMoves(
        const Board& board,
        Square from,
        Piece piece,
        std::vector<Move>& moves
    );

    static void generateRookMoves(
        const Board& board,
        Square from,
        Piece piece,
        std::vector<Move>& moves
    );

    static void generateQueenMoves(
        const Board& board,
        Square from,
        Piece piece,
        std::vector<Move>& moves
    );

    static void generateKingMoves(
        const Board& board,
        Square from,
        Piece piece,
        std::vector<Move>& moves
    );
    static void generateCastlingMoves(
        const Board& board, 
        Square from, 
        Piece piece, 
        const CastlingRights& castlingRights,
        std::vector<Move>& moves);
};