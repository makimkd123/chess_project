#include "Fen.h"

#include <cctype>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace {

char pieceToFenChar(Piece piece) {
    char c = '?';

    switch (piece.type) {
        case PieceType::King:
            c = 'k';
            break;
        case PieceType::Queen:
            c = 'q';
            break;
        case PieceType::Rook:
            c = 'r';
            break;
        case PieceType::Bishop:
            c = 'b';
            break;
        case PieceType::Knight:
            c = 'n';
            break;
        case PieceType::Pawn:
            c = 'p';
            break;
    }

    if (piece.color == PieceColor::White) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    return c;
}

std::optional<Piece> fenCharToPiece(char c) {
    PieceColor color =
        std::isupper(static_cast<unsigned char>(c))
            ? PieceColor::White
            : PieceColor::Black;

    char lower = static_cast<char>(
        std::tolower(static_cast<unsigned char>(c))
    );

    PieceType type;

    switch (lower) {
        case 'k':
            type = PieceType::King;
            break;
        case 'q':
            type = PieceType::Queen;
            break;
        case 'r':
            type = PieceType::Rook;
            break;
        case 'b':
            type = PieceType::Bishop;
            break;
        case 'n':
            type = PieceType::Knight;
            break;
        case 'p':
            type = PieceType::Pawn;
            break;
        default:
            return std::nullopt;
    }

    return Piece{type, color};
}

std::string castlingRightsToFen(const CastlingRights& rights) {
    std::string result;

    if (rights.whiteKingside) {
        result += 'K';
    }

    if (rights.whiteQueenside) {
        result += 'Q';
    }

    if (rights.blackKingside) {
        result += 'k';
    }

    if (rights.blackQueenside) {
        result += 'q';
    }

    if (result.empty()) {
        return "-";
    }

    return result;
}

std::string squareToFen(const Square& square) {
    std::string result;

    result += static_cast<char>('a' + square.file);
    result += static_cast<char>('1' + square.rank);

    return result;
}

Result<Board> parsePiecePlacement(const std::string& text) {
    Board board;
    board.clear();

    int file = 0;
    int rank = 7;

    for (char c : text) {
        if (c == '/') {
            if (file != 8) {
                return Result<Board>::Failure(
                    "Invalid FEN: each rank must contain exactly 8 squares."
                );
            }

            --rank;
            file = 0;

            if (rank < 0) {
                return Result<Board>::Failure(
                    "Invalid FEN: too many ranks in piece placement."
                );
            }

            continue;
        }

        if (c >= '1' && c <= '8') {
            file += c - '0';

            if (file > 8) {
                return Result<Board>::Failure(
                    "Invalid FEN: rank contains more than 8 squares."
                );
            }

            continue;
        }

        auto piece = fenCharToPiece(c);

        if (!piece.has_value()) {
            return Result<Board>::Failure(
                "Invalid FEN: unknown piece character."
            );
        }

        if (file >= 8) {
            return Result<Board>::Failure(
                "Invalid FEN: rank contains more than 8 squares."
            );
        }

        board.setPiece(Square{file, rank}, piece.value());
        ++file;
    }

    if (rank != 0 || file != 8) {
        return Result<Board>::Failure(
            "Invalid FEN: piece placement must contain exactly 8 ranks."
        );
    }

    return Result<Board>::Success(std::move(board));
}

Result<PieceColor> parseSideToMove(const std::string& text) {
    if (text == "w") {
        return Result<PieceColor>::Success(PieceColor::White);
    }

    if (text == "b") {
        return Result<PieceColor>::Success(PieceColor::Black);
    }

    return Result<PieceColor>::Failure(
        "Invalid FEN: side to move must be 'w' or 'b'."
    );
}

Result<CastlingRights> parseCastlingRights(const std::string& text) {
    CastlingRights rights{
        false, false,
        false, false
    };

    if (text == "-") {
        return Result<CastlingRights>::Success(rights);
    }

    bool seenK = false;
    bool seenQ = false;
    bool seenk = false;
    bool seenq = false;

    for (char c : text) {
        switch (c) {
            case 'K':
                if (seenK) {
                    return Result<CastlingRights>::Failure(
                        "Invalid FEN: duplicate white kingside castling right."
                    );
                }

                rights.whiteKingside = true;
                seenK = true;
                break;

            case 'Q':
                if (seenQ) {
                    return Result<CastlingRights>::Failure(
                        "Invalid FEN: duplicate white queenside castling right."
                    );
                }

                rights.whiteQueenside = true;
                seenQ = true;
                break;

            case 'k':
                if (seenk) {
                    return Result<CastlingRights>::Failure(
                        "Invalid FEN: duplicate black kingside castling right."
                    );
                }

                rights.blackKingside = true;
                seenk = true;
                break;

            case 'q':
                if (seenq) {
                    return Result<CastlingRights>::Failure(
                        "Invalid FEN: duplicate black queenside castling right."
                    );
                }

                rights.blackQueenside = true;
                seenq = true;
                break;

            default:
                return Result<CastlingRights>::Failure(
                    "Invalid FEN: castling rights must contain only KQkq or '-'."
                );
        }
    }

    return Result<CastlingRights>::Success(rights);
}

Result<std::optional<Square>> parseEnPassantTarget(const std::string& text) {
    if (text == "-") {
        return Result<std::optional<Square>>::Success(std::nullopt);
    }

    if (text.size() != 2) {
        return Result<std::optional<Square>>::Failure(
            "Invalid FEN: en passant target must be '-' or a square like e3."
        );
    }

    char fileChar = text[0];
    char rankChar = text[1];

    if (fileChar < 'a' || fileChar > 'h') {
        return Result<std::optional<Square>>::Failure(
            "Invalid FEN: en passant file must be between a and h."
        );
    }

    if (rankChar != '3' && rankChar != '6') {
        return Result<std::optional<Square>>::Failure(
            "Invalid FEN: en passant target rank must be 3 or 6."
        );
    }

    int file = fileChar - 'a';
    int rank = rankChar - '1';

    return Result<std::optional<Square>>::Success(Square{file, rank});
}

Result<int> parseInteger(
    const std::string& text,
    const std::string& fieldName,
    int minimum
) {
    if (text.empty()) {
        return Result<int>::Failure(
            "Invalid FEN: missing " + fieldName + "."
        );
    }

    int value = 0;

    for (char c : text) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return Result<int>::Failure(
                "Invalid FEN: " + fieldName + " must be an integer."
            );
        }

        int digit = c - '0';

        if (value > (std::numeric_limits<int>::max() - digit) / 10) {
            return Result<int>::Failure(
                "Invalid FEN: " + fieldName + " is too large."
            );
        }

        value = value * 10 + digit;
    }

    if (value < minimum) {
        return Result<int>::Failure(
            "Invalid FEN: " + fieldName + " is too small."
        );
    }

    return Result<int>::Success(value);
}

Result<int> parseNonNegativeInt(
    const std::string& text,
    const std::string& fieldName
) {
    return parseInteger(text, fieldName, 0);
}

Result<int> parsePositiveInt(
    const std::string& text,
    const std::string& fieldName
) {
    return parseInteger(text, fieldName, 1);
}

} // namespace

std::string toFen(const GameState& state) {
    std::ostringstream fen;

    for (int rank = 7; rank >= 0; --rank) {
        int emptySquares = 0;

        for (int file = 0; file < 8; ++file) {
            Square square{file, rank};

            auto piece = state.board.pieceAt(square);

            if (!piece.has_value()) {
                ++emptySquares;
                continue;
            }

            if (emptySquares > 0) {
                fen << emptySquares;
                emptySquares = 0;
            }

            fen << pieceToFenChar(piece.value());
        }

        if (emptySquares > 0) {
            fen << emptySquares;
        }

        if (rank > 0) {
            fen << '/';
        }
    }

    fen << ' ';
    fen << (state.sideToMove == PieceColor::White ? 'w' : 'b');

    fen << ' ';
    fen << castlingRightsToFen(state.castlingRights);

    fen << ' ';

    if (state.enPassantTarget.has_value()) {
        fen << squareToFen(state.enPassantTarget.value());
    } else {
        fen << '-';
    }

    fen << ' ';
    fen << state.halfmoveClock;

    fen << ' ';
    fen << state.fullmoveNumber;

    return fen.str();
}

Result<GameState> fromFen(const std::string& fen) {
    std::istringstream input(fen);

    std::string placement;
    std::string sideToMoveText;
    std::string castlingText;
    std::string enPassantText;
    std::string halfmoveText;
    std::string fullmoveText;

    if (!(input >> placement >> sideToMoveText >> castlingText
                >> enPassantText >> halfmoveText >> fullmoveText)) {
        return Result<GameState>::Failure(
            "Invalid FEN: expected exactly 6 fields."
        );
    }

    std::string extra;

    if (input >> extra) {
        return Result<GameState>::Failure(
            "Invalid FEN: too many fields."
        );
    }

    auto boardResult = parsePiecePlacement(placement);

    if (!boardResult.success) {
        return Result<GameState>::Failure(boardResult.message);
    }

    auto sideResult = parseSideToMove(sideToMoveText);

    if (!sideResult.success) {
        return Result<GameState>::Failure(sideResult.message);
    }

    auto castlingResult = parseCastlingRights(castlingText);

    if (!castlingResult.success) {
        return Result<GameState>::Failure(castlingResult.message);
    }

    auto enPassantResult = parseEnPassantTarget(enPassantText);

    if (!enPassantResult.success) {
        return Result<GameState>::Failure(enPassantResult.message);
    }

    auto halfmoveResult = parseNonNegativeInt(halfmoveText, "halfmove clock");

    if (!halfmoveResult.success) {
        return Result<GameState>::Failure(halfmoveResult.message);
    }

    auto fullmoveResult = parsePositiveInt(fullmoveText, "fullmove number");

    if (!fullmoveResult.success) {
        return Result<GameState>::Failure(fullmoveResult.message);
    }

    GameState state;

    state.board = std::move(boardResult.value);
    state.sideToMove = sideResult.value;
    state.castlingRights = castlingResult.value;
    state.enPassantTarget = enPassantResult.value;
    state.halfmoveClock = halfmoveResult.value;
    state.fullmoveNumber = fullmoveResult.value;

    return Result<GameState>::Success(
        std::move(state),
        "FEN parsed successfully."
    );
}