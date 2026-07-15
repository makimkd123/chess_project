#pragma once

#include "logic/GameState.h"
#include "models/Result.h"

#include <string>


std::string toFen(const GameState& state);

Result<GameState> fromFen(const std::string& fen);