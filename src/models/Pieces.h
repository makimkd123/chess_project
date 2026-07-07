#pragma once


enum class PieceType{
    Pawn, Knight, Bishop, Rook, Queen, King
};

enum class PieceColor{
    White, Black
};

struct Piece{
    PieceType type;
    PieceColor color;
};