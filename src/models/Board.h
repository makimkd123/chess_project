#pragma once
#include <array>
#include <optional>
#include "Pieces.h"
#include "Square.h"
#include "Move.h"


class Board{
    public:
        std::optional<Piece> pieceAt(Square sq) const;
        bool isEmpty(Square sq) const;
        bool isInside(Square sq) const;
        void setPiece(Square sq, std::optional<Piece> piece);
        void setupStartingPosition();
        void clear();
        bool makeMove(const Move& move);
    private:
        int toIndex(Square sq) const;
        std::array<std::optional<Piece>,64> squares_;
};