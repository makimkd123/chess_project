#pragma once

#include "logic/Game.h"
#include "models/Result.h"
#include "logic/MoveGenerator.h"
#include "San.h"


#include <string>
#include <string_view>

enum class PgnResult {
    WhiteWin,
    BlackWin,
    Draw,
    Ongoing
};

struct PgnMetadata {
    std::string event = "?";
    std::string site = "?";
    std::string date = "????.??.??";
    std::string round = "?";
    std::string white = "White";
    std::string black = "Black";
    PgnResult result = PgnResult::Ongoing;
};
struct ImportedPgn {
    Game game;
    PgnMetadata metadata;
};

std::string toPgn(const Game& game,const PgnMetadata& metadata);



Result<ImportedPgn> fromPgn(std::string_view pgn);