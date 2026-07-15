#include "San.h"
#include <stdexcept>
#include <vector>


namespace {

char pieceToSanChar(PieceType type)
{
    switch (type) {
        case PieceType::King:
            return 'K';
        case PieceType::Queen:
            return 'Q';
        case PieceType::Rook:
            return 'R';
        case PieceType::Bishop:
            return 'B';
        case PieceType::Knight:
            return 'N';
        case PieceType::Pawn:
            return '\0';
    }

    return '?';
}

std::optional<PieceType> sanCharToPieceType(char c)
{
    const char upper = static_cast<char>(
        std::toupper(static_cast<unsigned char>(c))
    );

    switch (upper) {
        case 'K':
            return PieceType::King;
        case 'Q':
            return PieceType::Queen;
        case 'R':
            return PieceType::Rook;
        case 'B':
            return PieceType::Bishop;
        case 'N':
            return PieceType::Knight;
        default:
            return std::nullopt;
    }
}

std::string squareToSan(Square square)
{
    const char file = static_cast<char>('a' + square.file);
    const char rank = static_cast<char>('1' + square.rank);

    return std::string{file, rank};
}

bool isCapture(const GameState& state, const Move& move){
    return state.board.pieceAt(move.to).has_value() ||
           move.type == MoveType::EnPassant;
}

bool isCastleKingSide(const Move& move)
{
    return move.type == MoveType::Castle &&
           move.to.file == 6;
}

bool isCastleQueenSide(const Move& move)
{
    return move.type == MoveType::Castle &&
           move.to.file == 2;
}

std::string castlingNotation(const Move& move){

    if(isCastleKingSide(move)){
        return "O-O";
    }
    if(isCastleQueenSide(move)){
        return "O-O-O";
    }
    return "";
}

std::string disambiguationFor(
    const GameState& state,
    const Move& move,
    std::span<const Move> legalMoves)
{
    const std::optional<Piece> movingPiece =
        state.board.pieceAt(move.from);

    if (!movingPiece.has_value()) {
        return "";
    }

    bool foundAlternative = false;
    bool sameFile = false;
    bool sameRank = false;

    for (const Move& candidate : legalMoves) {

        // Do not compare the move with itself.
        if (candidate.from.file == move.from.file &&
            candidate.from.rank == move.from.rank) {
            continue;
        }

        // The alternative move must have the same destination.
        if (candidate.to.file != move.to.file ||
            candidate.to.rank != move.to.rank) {
            continue;
        }

        const std::optional<Piece> candidatePiece =
            state.board.pieceAt(candidate.from);

        if (!candidatePiece.has_value()) {
            continue;
        }

        if (candidatePiece->type != movingPiece->type ||
            candidatePiece->color != movingPiece->color) {
            continue;
        }

        foundAlternative = true;

        if (candidate.from.file == move.from.file) {
            sameFile = true;
        }

        if (candidate.from.rank == move.from.rank) {
            sameRank = true;
        }
    }

    if (!foundAlternative) {
        return "";
    }

    const std::string origin = squareToSan(move.from);

    const char file = origin[0];
    const char rank = origin[1];

    if (!sameFile) {
        return std::string(1, file);
    }

    if (!sameRank) {
        return std::string(1, rank);
    }

    return std::string{file, rank};
}

std::string promotionSuffix(const Move& move)
{
    if (move.type != MoveType::Promotion) {
        return "";
    }

    if (!move.promotionType.has_value()) {
        throw std::logic_error(
            "Promotion move has no promotion piece type");
    }

    return std::string{"="} +
           pieceToSanChar(*move.promotionType);
}

std::string checkSuffix(
    const GameState& state,
    const Move& move)
{
    GameState nextState = state;

    if (!nextState.applyMove(move)) {
        throw std::logic_error(
            "Cannot determine check suffix: move failed");
    }

    const GameStatus status = nextState.getStatus();

    if (status == GameStatus::Checkmate) {
        return "#";
    }

    if (status == GameStatus::Check) {
        return "+";
    }

    return "";
}


}


std::string toSan(const GameState& state,const Move& move,std::span<const Move> legalMoves){
    const std::optional<Piece> movingPiece =
        state.board.pieceAt(move.from);

    if (!movingPiece.has_value()) {
        throw std::logic_error(
            "Cannot create SAN: no piece on move origin");
    }

    std::string san;

    // Castling is handled separately.
    if (move.type == MoveType::Castle) {
        san = castlingNotation(move);
        
        san += checkSuffix(state, move);

        return san;
    }

    const bool capture = isCapture(state, move);

    if (movingPiece->type == PieceType::Pawn) {
        // Pawn captures include their origin file.
        if (capture) {
            const std::string origin = squareToSan(move.from);
            san += origin[0];
        }
    } else {
        // Non-pawn piece letter.
        san += pieceToSanChar(movingPiece->type);

        // N, R, B, Q and possibly K disambiguation.
        san += disambiguationFor(state, move, legalMoves);
    }

    if (capture) {
        san += 'x';
    }

    san += squareToSan(move.to);
    san += promotionSuffix(move);
    san += checkSuffix(state, move);

    return san;
}

Result<Move> fromSan(const GameState& state, std::string_view san){
    const std::vector<Move> legalMoves =
    MoveGenerator::generateLegalMoves(state.board,state.sideToMove,state.castlingRights,state.enPassantTarget);

    for(const Move& move : legalMoves){
        const std::string generatedsan = toSan(state,move,legalMoves);
        if(generatedsan==san){
            return Result<Move>::Success(move);
        }

    }

    return Result<Move>::Failure("No legal moves match SAN "+ std::string(san));

}