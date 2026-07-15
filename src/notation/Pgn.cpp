#include "notation/Pgn.h"
#include <stdexcept>
#include <cctype>
#include <sstream>


namespace {

std::string resultToString(PgnResult result)
{
    switch (result) {
        case PgnResult::WhiteWin:
            return "1-0";

        case PgnResult::BlackWin:
            return "0-1";

        case PgnResult::Draw:
            return "1/2-1/2";

        case PgnResult::Ongoing:
            return "*";
    }

    return "*";
}

std::string escapeTagValue(std::string_view value)
{
    std::string result;

    for (char c : value) {
        if (c == '"') {
            result += "\\\"";
        } else if (c == '\\') {
            result += "\\\\";
        } else {
            result += c;
        }
    }

    return result;
}

std::string makeTagPair(std::string_view name,std::string_view value){
    return "[" +
           std::string(name) +
           " \"" +
           escapeTagValue(value) +
           "\"]";
}

std::string makeTagSection(const PgnMetadata& metadata)
{
    std::string result;

    result += makeTagPair("Event", metadata.event) + "\n";
    result += makeTagPair("Site", metadata.site) + "\n";
    result += makeTagPair("Date", metadata.date) + "\n";
    result += makeTagPair("Round", metadata.round) + "\n";
    result += makeTagPair("White", metadata.white) + "\n";
    result += makeTagPair("Black", metadata.black) + "\n";
    result += makeTagPair(
        "Result",
        resultToString(metadata.result)
    ) + "\n";

    return result;
}

std::string makeMoveText(const Game& game,PgnResult result){
    GameState replayState = GameState::startingPosition();

    std::string moveText;

    const std::vector<Move>& history = game.moveHistory();

    for (std::size_t i = 0; i < history.size(); ++i) {
        const Move& move = history[i];

        const std::vector<Move> legalMoves =
            MoveGenerator::generateLegalMoves(
                replayState.board,
                replayState.sideToMove,
                replayState.castlingRights,
                replayState.enPassantTarget
            );

        if (replayState.sideToMove == PieceColor::White) {
            moveText += std::to_string(
                replayState.fullmoveNumber
            );

            moveText += ". ";
        }

        moveText += toSan(replayState,move,legalMoves);

        moveText += ' ';

        if (!replayState.applyMove(move)) {
            throw std::logic_error(
                "Cannot export PGN: move history contains an invalid move"
            );
        }
    }

    moveText += resultToString(result);

    return moveText;
}
bool isMoveNumber(std::string_view token)
{
    if (token.empty() || token.back() != '.') {
        return false;
    }

    for (std::size_t i = 0; i + 1 < token.size(); ++i) {
        if (!std::isdigit(
                static_cast<unsigned char>(token[i]))) {
            return false;
        }
    }

    return true;
}
struct ParsedTag {
    std::string name;
    std::string value;
};
std::string unescapeTagValue(std::string_view value)
{
    std::string result;
    bool escaping = false;

    for (char c : value) {
        if (escaping) {
            if (c == '"' || c == '\\') {
                result += c;
            } else {
                result += '\\';
                result += c;
            }

            escaping = false;
            continue;
        }

        if (c == '\\') {
            escaping = true;
        } else {
            result += c;
        }
    }

    if (escaping) {
        result += '\\';
    }

    return result;
}
Result<ParsedTag> parseTagLine(std::string_view line)
{
    if (line.size() < 5) {
        return Result<ParsedTag>::Failure(
            "PGN tag line is too short"
        );
    }

    if (line.front() != '[' || line.back() != ']') {
        return Result<ParsedTag>::Failure(
            "PGN tag must begin with '[' and end with ']'"
        );
    }

    const std::size_t firstSpace = line.find(' ');

    if (firstSpace == std::string_view::npos) {
        return Result<ParsedTag>::Failure(
            "PGN tag has no value"
        );
    }

    const std::string name{
        line.substr(1, firstSpace - 1)
    };

    const std::size_t openingQuote =
        line.find('"', firstSpace);

    if (openingQuote == std::string_view::npos) {
        return Result<ParsedTag>::Failure(
            "PGN tag has no opening quote"
        );
    }

    const std::size_t closingQuote =
        line.rfind('"');

    if (closingQuote == std::string_view::npos ||
        closingQuote <= openingQuote) {
        return Result<ParsedTag>::Failure(
            "PGN tag has no closing quote"
        );
    }

    const std::string escapedValue{
        line.substr(
            openingQuote + 1,
            closingQuote - openingQuote - 1
        )
    };

    return Result<ParsedTag>::Success(
        ParsedTag{
            name,
            unescapeTagValue(escapedValue)
        }
    );
}
Result<bool> applyTag(
    PgnMetadata& metadata,
    const ParsedTag& tag)
{
    if (tag.name == "Event") {
        metadata.event = tag.value;
    }
    else if (tag.name == "Site") {
        metadata.site = tag.value;
    }
    else if (tag.name == "Date") {
        metadata.date = tag.value;
    }
    else if (tag.name == "Round") {
        metadata.round = tag.value;
    }
    else if (tag.name == "White") {
        metadata.white = tag.value;
    }
    else if (tag.name == "Black") {
        metadata.black = tag.value;
    }
    else if (tag.name == "Result") {
        if (tag.value == "1-0") {
            metadata.result = PgnResult::WhiteWin;
        }
        else if (tag.value == "0-1") {
            metadata.result = PgnResult::BlackWin;
        }
        else if (tag.value == "1/2-1/2") {
            metadata.result = PgnResult::Draw;
        }
        else if (tag.value == "*") {
            metadata.result = PgnResult::Ongoing;
        }
        else {
            return Result<bool>::Failure(
                "Invalid PGN result tag: " + tag.value
            );
        }
    }

    return Result<bool>::Success(true);
}
}//namespace

std::string toPgn(const Game& game,const PgnMetadata& metadata){
    std::string finalPgn;

    finalPgn+=makeTagSection(metadata);
    finalPgn += '\n';
    finalPgn+=makeMoveText(game,metadata.result);

    return finalPgn;    
}

Result<ImportedPgn> fromPgn(std::string_view pgn)
{
    ImportedPgn imported;

    std::istringstream input{std::string(pgn)};
    std::string line;
    std::string moveText;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            const Result<ParsedTag> parsedTag =
                parseTagLine(line);

            if (!parsedTag.success) {
                return Result<ImportedPgn>::Failure(
                    parsedTag.message
                );
            }

            const Result<bool> applied =
                applyTag(
                    imported.metadata,
                    parsedTag.value
                );

            if (!applied.success) {
                return Result<ImportedPgn>::Failure(
                    applied.message
                );
            }

            continue;
        }

        if (!moveText.empty()) {
            moveText += ' ';
        }

        moveText += line;
    }

    std::istringstream moveStream(moveText);
    std::string token;

    while (moveStream >> token) {
        if (token == "1-0" ||
            token == "0-1" ||
            token == "1/2-1/2" ||
            token == "*") {
            break;
        }

        if (isMoveNumber(token)) {
            continue;
        }

        const Result<Move> parsedMove =
            fromSan(imported.game.state(), token);

        if (!parsedMove.success) {
            return Result<ImportedPgn>::Failure(
                "Invalid SAN token '" +
                token +
                "': " +
                parsedMove.message
            );
        }

        if (!imported.game.makeMove(parsedMove.value)) {
            return Result<ImportedPgn>::Failure(
                "Illegal PGN move: " + token
            );
        }
    }

    return Result<ImportedPgn>::Success(
        std::move(imported)
    );
}