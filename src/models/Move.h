#pragma once
#include "Square.h"
#include "Pieces.h"
#include <optional>

enum class MoveType {
    Normal,
    Capture,
    Castle,
    EnPassant,
    Promotion
};

struct Move {
    Square from;
    Square to;
    MoveType type;
    std::optional<PieceType> promotionType = std::nullopt;
};