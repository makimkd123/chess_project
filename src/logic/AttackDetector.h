#pragma once

#include <optional>

#include "models/Board.h"
#include "models/Pieces.h"
#include "models/Square.h"

class AttackDetector{
    public:
        static bool isSquareAttacked(
            const Board& board,
            Square square,
            PieceColor bySide
        );

        static bool isKingInCheck(
            const Board& board,
            PieceColor kingColor
        );
    private:
        static std::optional<Square> findKing(
            const Board& board,
            PieceColor kingColor
        );
};