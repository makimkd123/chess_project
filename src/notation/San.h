#pragma once

#include "logic/GameState.h"
#include "models/Move.h"
#include "models/Result.h"
#include "logic/MoveGenerator.h"
#include <span>
#include <string>
#include <string_view>

std::string toSan(const GameState& state,const Move& move,std::span<const Move> legalMoves);

Result<Move> fromSan(const GameState& sate, std::string_view san);