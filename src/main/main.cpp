#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "logic/Game.h"
#include "logic/GameState.h"
#include "logic/MoveGenerator.h"
#include "models/Move.h"
#include "models/Pieces.h"
#include "notation/San.h"
#include "notation/Pgn.h"

namespace {

std::vector<Move> legalMovesFor(const GameState& state)
{
    return MoveGenerator::generateLegalMoves(
        state.board,
        state.sideToMove,
        state.castlingRights,
        state.enPassantTarget
    );
}

void expectSan(
    const GameState& state,
    const Move& move,
    const std::string& expected,
    const std::string& testName)
{
    const std::vector<Move> legalMoves = legalMovesFor(state);

    try {
        const std::string actual = toSan(state, move, legalMoves);

        if (actual == expected) {
            std::cout << "[PASS] " << testName << '\n';
        } else {
            std::cout << "[FAIL] " << testName << '\n';
            std::cout << "       Expected: " << expected << '\n';
            std::cout << "       Actual:   " << actual << '\n';
        }
    }
    catch (const std::exception& error) {
        std::cout << "[FAIL] " << testName << '\n';
        std::cout << "       Exception: " << error.what() << '\n';
    }
}

GameState emptyState(PieceColor sideToMove = PieceColor::White)
{
    GameState state;
    state.board.clear();
    state.sideToMove = sideToMove;
    state.castlingRights = CastlingRights{false, false, false, false};
    state.enPassantTarget = std::nullopt;
    state.halfmoveClock = 0;
    state.fullmoveNumber = 1;
    return state;
}

void testPawnMove()
{
    const GameState state = GameState::startingPosition();

    const Move move{
        Square{4, 1}, // e2
        Square{4, 3}, // e4
        MoveType::Normal
    };

    expectSan(state, move, "e4", "Pawn move e2-e4 is e4");
}

void testKnightMove()
{
    const GameState state = GameState::startingPosition();

    const Move move{
        Square{6, 0}, // g1
        Square{5, 2}, // f3
        MoveType::Normal
    };

    expectSan(state, move, "Nf3", "Knight move g1-f3 is Nf3");
}

void testPawnCapture()
{
    GameState state = emptyState();

    state.board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});
    state.board.setPiece({4, 3}, Piece{PieceType::Pawn, PieceColor::White}); // e4
    state.board.setPiece({3, 4}, Piece{PieceType::Pawn, PieceColor::Black}); // d5

    const Move move{
        Square{4, 3}, // e4
        Square{3, 4}, // d5
        MoveType::Capture
    };

    expectSan(state, move, "exd5", "Pawn capture e4xd5 is exd5");
}

void testPieceCapture()
{
    GameState state = emptyState();

    state.board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});
    state.board.setPiece({5, 2}, Piece{PieceType::Knight, PieceColor::White}); // f3
    state.board.setPiece({4, 4}, Piece{PieceType::Pawn, PieceColor::Black});   // e5

    const Move move{
        Square{5, 2}, // f3
        Square{4, 4}, // e5
        MoveType::Capture
    };

    expectSan(state, move, "Nxe5", "Knight capture f3xe5 is Nxe5");
}

void testFileDisambiguation()
{
    GameState state = emptyState();

    state.board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});
    state.board.setPiece({1, 0}, Piece{PieceType::Knight, PieceColor::White}); // b1
    state.board.setPiece({5, 0}, Piece{PieceType::Knight, PieceColor::White}); // f1

    const Move move{
        Square{1, 0}, // b1
        Square{3, 1}, // d2
        MoveType::Normal
    };

    expectSan(state, move, "Nbd2", "Two knights require file disambiguation");
}

void testKingsideCastling()
{
    GameState state = emptyState();

    state.board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({7, 0}, Piece{PieceType::Rook, PieceColor::White});
    state.board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});
    state.castlingRights.whiteKingside = true;

    const Move move{
        Square{4, 0}, // e1
        Square{6, 0}, // g1
        MoveType::Castle
    };

    expectSan(state, move, "O-O", "Kingside castling is O-O");
}

void testPromotion()
{
    GameState state = emptyState();

    state.board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({7, 7}, Piece{PieceType::King, PieceColor::Black});
    state.board.setPiece({4, 6}, Piece{PieceType::Pawn, PieceColor::White}); // e7

    const Move move{
        Square{4, 6}, // e7
        Square{4, 7}, // e8
        MoveType::Promotion,
        PieceType::Queen
    };

    expectSan(state, move, "e8=Q+", "Promotion with check is e8=Q+");
}

void testCheckmate()
{
    Game game;

    game.makeMove(Move{{5, 1}, {5, 2}, MoveType::Normal}); // f2-f3
    game.makeMove(Move{{4, 6}, {4, 4}, MoveType::Normal}); // e7-e5
    game.makeMove(Move{{6, 1}, {6, 3}, MoveType::Normal}); // g2-g4

    const Move move{
        Square{3, 7}, // d8
        Square{7, 3}, // h4
        MoveType::Normal
    };

    expectSan(game.state(), move, "Qh4#", "Fool's mate ends with Qh4#");
}
void testRankDisambiguation()
{
    GameState state = emptyState();

    state.board.setPiece(
        {0, 0},
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {7, 7},
        Piece{PieceType::King, PieceColor::Black}
    );

    // Both rooks can move to e2.
    state.board.setPiece(
        {4, 0}, // e1
        Piece{PieceType::Rook, PieceColor::White}
    );

    state.board.setPiece(
        {4, 2}, // e3
        Piece{PieceType::Rook, PieceColor::White}
    );

    const Move move{
        Square{4, 0}, // e1
        Square{4, 1}, // e2
        MoveType::Normal
    };

    expectSan(
        state,
        move,
        "R1e2",
        "Two rooks on the same file require rank disambiguation"
    );
}
void testFileAndRankDisambiguation()
{
    GameState state = emptyState();

    state.board.setPiece(
        {0, 0},
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {7, 7},
        Piece{PieceType::King, PieceColor::Black}
    );

    /*
        All three knights can move to d2:

        b1 -> d2  selected move
        b3 -> d2  same file as b1
        f1 -> d2  same rank as b1
    */

    state.board.setPiece(
        {1, 0}, // b1
        Piece{PieceType::Knight, PieceColor::White}
    );

    state.board.setPiece(
        {1, 2}, // b3
        Piece{PieceType::Knight, PieceColor::White}
    );

    state.board.setPiece(
        {5, 0}, // f1
        Piece{PieceType::Knight, PieceColor::White}
    );

    const Move move{
        Square{1, 0}, // b1
        Square{3, 1}, // d2
        MoveType::Normal
    };

    expectSan(
        state,
        move,
        "Nb1d2",
        "Three knights require file-and-rank disambiguation"
    );
}
void testQueensideCastling()
{
    GameState state = emptyState();

    state.board.setPiece(
        {4, 0}, // e1
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {0, 0}, // a1
        Piece{PieceType::Rook, PieceColor::White}
    );

    state.board.setPiece(
        {4, 7}, // e8
        Piece{PieceType::King, PieceColor::Black}
    );

    state.castlingRights.whiteQueenside = true;

    const Move move{
        Square{4, 0}, // e1
        Square{2, 0}, // c1
        MoveType::Castle
    };

    expectSan(
        state,
        move,
        "O-O-O",
        "Queenside castling is O-O-O"
    );
}
void testCapturePromotion()
{
    GameState state = emptyState();

    state.board.setPiece(
        {0, 0},
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {7, 5},
        Piece{PieceType::King, PieceColor::Black}
    );

    state.board.setPiece(
        {4, 6}, // e7
        Piece{PieceType::Pawn, PieceColor::White}
    );

    state.board.setPiece(
        {3, 7}, // d8
        Piece{PieceType::Rook, PieceColor::Black}
    );

    const Move move{
        Square{4, 6}, // e7
        Square{3, 7}, // d8
        MoveType::Promotion,
        PieceType::Queen
    };

    expectSan(
        state,
        move,
        "exd8=Q",
        "Pawn capture promotion is exd8=Q"
    );
}
void testOrdinaryCheck()
{
    GameState state = emptyState();

    state.board.setPiece(
        {0, 0}, // a1
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {4, 7}, // e8
        Piece{PieceType::King, PieceColor::Black}
    );

    state.board.setPiece(
        {3, 0}, // d1
        Piece{PieceType::Queen, PieceColor::White}
    );

    const Move move{
        Square{3, 0}, // d1
        Square{7, 4}, // h5
        MoveType::Normal
    };

    expectSan(
        state,
        move,
        "Qh5+",
        "Queen move giving check is Qh5+"
    );
}
void testEnPassant()
{
    GameState state = emptyState();

    state.board.setPiece(
        {0, 0},
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {7, 7},
        Piece{PieceType::King, PieceColor::Black}
    );

    state.board.setPiece(
        {4, 4}, // e5
        Piece{PieceType::Pawn, PieceColor::White}
    );

    state.board.setPiece(
        {3, 4}, // d5
        Piece{PieceType::Pawn, PieceColor::Black}
    );

    state.enPassantTarget = Square{3, 5}; // d6

    const Move move{
        Square{4, 4}, // e5
        Square{3, 5}, // d6
        MoveType::EnPassant
    };

    expectSan(
        state,
        move,
        "exd6",
        "En passant capture is written as exd6"
    );
}
void expectSanException(
    const GameState& state,
    const Move& move,
    const std::string& testName)
{
    const std::vector<Move> legalMoves =
        legalMovesFor(state);

    try {
        const std::string san =
            toSan(state, move, legalMoves);

        std::cout << "[FAIL] " << testName << '\n';
        std::cout << "       Expected an exception\n";
        std::cout << "       Actual SAN: " << san << '\n';
    }
    catch (const std::logic_error&) {
        std::cout << "[PASS] " << testName << '\n';
    }
    catch (const std::exception& error) {
        std::cout << "[FAIL] " << testName << '\n';
        std::cout
            << "       Unexpected exception: "
            << error.what()
            << '\n';
    }
}
void testPromotionWithoutPieceType()
{
    GameState state = emptyState();

    state.board.setPiece(
        {0, 0},
        Piece{PieceType::King, PieceColor::White}
    );

    state.board.setPiece(
        {7, 7},
        Piece{PieceType::King, PieceColor::Black}
    );

    state.board.setPiece(
        {4, 6}, // e7
        Piece{PieceType::Pawn, PieceColor::White}
    );

    const Move move{
        Square{4, 6}, // e7
        Square{4, 7}, // e8
        MoveType::Promotion,
        std::nullopt
    };

    expectSanException(
        state,
        move,
        "Promotion without promotionType throws"
    );
}
void testMoveFromEmptySquare()
{
    const GameState state = emptyState();

    const Move move{
        Square{4, 1}, // e2 is empty
        Square{4, 3}, // e4
        MoveType::Normal
    };

    expectSanException(
        state,
        move,
        "SAN generation from an empty square throws"
    );
}
void expectFromSan(
    const GameState& state,
    std::string_view san,
    const Move& expectedMove,
    const std::string& testName)
{
    const Result<Move> result = fromSan(state, san);

    if (!result.success) {
        std::cout << "[FAIL] " << testName << '\n';
        std::cout << "       " << result.message << '\n';
        return;
    }

    const Move& actual = result.value;

    const bool matches =
        actual.from.file == expectedMove.from.file &&
        actual.from.rank == expectedMove.from.rank &&
        actual.to.file == expectedMove.to.file &&
        actual.to.rank == expectedMove.to.rank &&
        actual.type == expectedMove.type &&
        actual.promotionType == expectedMove.promotionType;

    if (matches) {
        std::cout << "[PASS] " << testName << '\n';
    } else {
        std::cout << "[FAIL] " << testName << '\n';
    }
}
void expectFromSanFailure(
    const GameState& state,
    std::string_view san,
    const std::string& testName)
{
    const Result<Move> result = fromSan(state, san);

    if (!result.success) {
        std::cout << "[PASS] " << testName << '\n';
    } else {
        std::cout << "[FAIL] " << testName << '\n';
        std::cout << "       Expected failure for SAN: "
                  << san << '\n';
    }
}
void testFromSanPawnMove()
{
    const GameState state = GameState::startingPosition();

    const Move expected{
        Square{4, 1}, // e2
        Square{4, 3}, // e4
        MoveType::Normal
    };

    expectFromSan(
        state,
        "e4",
        expected,
        "fromSan parses e4"
    );
}
void testFromSanKnightMove()
{
    const GameState state = GameState::startingPosition();

    const Move expected{
        Square{6, 0}, // g1
        Square{5, 2}, // f3
        MoveType::Normal
    };

    expectFromSan(
        state,
        "Nf3",
        expected,
        "fromSan parses Nf3"
    );
}
void testFromSanInvalid()
{
    const GameState state = GameState::startingPosition();

    expectFromSanFailure(
        state,
        "Banana",
        "fromSan rejects invalid SAN"
    );
}
void testFromSanIllegalMove()
{
    const GameState state = GameState::startingPosition();

    expectFromSanFailure(
        state,
        "Qh5",
        "fromSan rejects illegal SAN"
    );
}
void testFromSanIllegal()
{
    GameState state = GameState::startingPosition();

    expectFromSanFailure(
        state,
        "Qh5",
        "fromSan rejects illegal SAN"
    );
}
void testSanRoundTrip()
{
    const GameState state = GameState::startingPosition();

    const std::vector<Move> legalMoves =
        MoveGenerator::generateLegalMoves(
            state.board,
            state.sideToMove,
            state.castlingRights,
            state.enPassantTarget
        );

    for (const Move& move : legalMoves) {
        const std::string san =
            toSan(state, move, legalMoves);

        const Result<Move> parsed =
            fromSan(state, san);

        if (!parsed.success) {
            std::cout
                << "[FAIL] SAN round-trip failed for "
                << san
                << '\n';
            return;
        }
    }

    std::cout
        << "[PASS] Every legal starting move survives SAN round-trip\n";
}
bool movesEqual(const Move& first, const Move& second)
{
    return first.from.file == second.from.file &&
           first.from.rank == second.from.rank &&
           first.to.file == second.to.file &&
           first.to.rank == second.to.rank &&
           first.type == second.type &&
           first.promotionType == second.promotionType;
}

void testSanRoundTripStartingPosition()
{
    const GameState state = GameState::startingPosition();

    const std::vector<Move> legalMoves =
        MoveGenerator::generateLegalMoves(
            state.board,
            state.sideToMove,
            state.castlingRights,
            state.enPassantTarget
        );

    for (const Move& originalMove : legalMoves) {
        const std::string san =
            toSan(state, originalMove, legalMoves);

        const Result<Move> parsedMove =
            fromSan(state, san);

        if (!parsedMove.success) {
            std::cout
                << "[FAIL] SAN round-trip failed to parse "
                << san << '\n';
            return;
        }

        if (!movesEqual(originalMove, parsedMove.value)) {
            std::cout
                << "[FAIL] SAN round-trip returned a different move for "
                << san << '\n';
            return;
        }
    }

    std::cout
        << "[PASS] Every legal starting move survives SAN round-trip\n";
}void expectPgn(
    const Game& game,
    const PgnMetadata& metadata,
    const std::string& expected,
    const std::string& testName)
{
    try {
        const std::string actual =
            toPgn(game, metadata);

        if (actual == expected) {
            std::cout << "[PASS] " << testName << '\n';
        }
        else {
            std::cout << "[FAIL] " << testName << '\n';
            std::cout << "Expected:\n" << expected << '\n';
            std::cout << "Actual:\n" << actual << '\n';
        }
    }
    catch (const std::exception& error) {
        std::cout << "[FAIL] " << testName << '\n';
        std::cout << "Exception: " << error.what() << '\n';
    }
}
void testEmptyGamePgn()
{
    Game game;

    PgnMetadata metadata;
    metadata.event = "Test";
    metadata.site = "Skopje";
    metadata.date = "2026.07.15";
    metadata.round = "1";
    metadata.white = "Alice";
    metadata.black = "Bob";
    metadata.result = PgnResult::Ongoing;

    const std::string expected =
        "[Event \"Test\"]\n"
        "[Site \"Skopje\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"1\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"*\"]\n"
        "\n"
        "*";

    expectPgn(
        game,
        metadata,
        expected,
        "Empty game exports valid PGN"
    );
}
void testBasicPgnMoves()
{
    Game game;

    game.makeMove(Move{
        Square{4, 1},
        Square{4, 3},
        MoveType::Normal
    });

    game.makeMove(Move{
        Square{4, 6},
        Square{4, 4},
        MoveType::Normal
    });

    game.makeMove(Move{
        Square{6, 0},
        Square{5, 2},
        MoveType::Normal
    });

    PgnMetadata metadata;
    metadata.event = "Test Game";
    metadata.site = "Skopje";
    metadata.date = "2026.07.15";
    metadata.round = "1";
    metadata.white = "Alice";
    metadata.black = "Bob";
    metadata.result = PgnResult::Ongoing;

    const std::string expected =
        "[Event \"Test Game\"]\n"
        "[Site \"Skopje\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"1\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"*\"]\n"
        "\n"
        "1. e4 e5 2. Nf3 *";

    expectPgn(
        game,
        metadata,
        expected,
        "PGN exports numbered SAN moves"
    );
}
void testCheckmatePgn()
{
    Game game;

    game.makeMove(Move{
        Square{5, 1},
        Square{5, 2},
        MoveType::Normal
    });

    game.makeMove(Move{
        Square{4, 6},
        Square{4, 4},
        MoveType::Normal
    });

    game.makeMove(Move{
        Square{6, 1},
        Square{6, 3},
        MoveType::Normal
    });

    game.makeMove(Move{
        Square{3, 7},
        Square{7, 3},
        MoveType::Normal
    });

    PgnMetadata metadata;
    metadata.event = "Fool's Mate";
    metadata.site = "Skopje";
    metadata.date = "2026.07.15";
    metadata.round = "1";
    metadata.white = "Alice";
    metadata.black = "Bob";
    metadata.result = PgnResult::BlackWin;

    const std::string expected =
        "[Event \"Fool's Mate\"]\n"
        "[Site \"Skopje\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"1\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"0-1\"]\n"
        "\n"
        "1. f3 e5 2. g4 Qh4# 0-1";

    expectPgn(
        game,
        metadata,
        expected,
        "PGN exports checkmate and black-win result"
    );
}
void testEscapedPgnTags()
{
    Game game;

    PgnMetadata metadata;
    metadata.event = "John \"The Hammer\"";
    metadata.site = R"(C:\Chess)";
    metadata.date = "2026.07.15";
    metadata.round = "1";
    metadata.white = "Alice";
    metadata.black = "Bob";
    metadata.result = PgnResult::Ongoing;

    const std::string expected =
        "[Event \"John \\\"The Hammer\\\"\"]\n"
        "[Site \"C:\\\\Chess\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"1\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"*\"]\n"
        "\n"
        "*";

    expectPgn(
        game,
        metadata,
        expected,
        "PGN escapes tag values"
    );
}
void testBasicPgnImport()
{
    const std::string pgn =
        "[Event \"Test Game\"]\n"
        "[Site \"Skopje\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"1\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"*\"]\n"
        "\n"
        "1. e4 e5 2. Nf3 Nc6 *";

    const Result<ImportedPgn> result =
        fromPgn(pgn);

    if (!result.success) {
        std::cout << "[FAIL] Basic PGN import\n";
        std::cout << "       " << result.message << '\n';
        return;
    }

    if (result.value.game.moveHistory().size() != 4) {
        std::cout << "[FAIL] Basic PGN import\n";
        std::cout << "       Expected 4 moves\n";
        return;
    }

    std::cout << "[PASS] Basic PGN import\n";
}
void testPgnMetadataImport()
{
    const std::string pgn =
        "[Event \"Test Game\"]\n"
        "[Site \"Skopje\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"3\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"1-0\"]\n"
        "\n"
        "1. e4 e5 2. Nf3 Nc6 1-0";

    const Result<ImportedPgn> result =
        fromPgn(pgn);

    if (!result.success) {
        std::cout << "[FAIL] PGN metadata import\n";
        std::cout << "       " << result.message << '\n';
        return;
    }

    const PgnMetadata& metadata =
        result.value.metadata;

    const bool matches =
        metadata.event == "Test Game" &&
        metadata.site == "Skopje" &&
        metadata.date == "2026.07.15" &&
        metadata.round == "3" &&
        metadata.white == "Alice" &&
        metadata.black == "Bob" &&
        metadata.result == PgnResult::WhiteWin;

    if (matches) {
        std::cout << "[PASS] PGN metadata import\n";
    } else {
        std::cout << "[FAIL] PGN metadata import\n";
    }
}
void testEscapedPgnImport()
{
    const std::string pgn =
        "[Event \"John \\\"The Hammer\\\"\"]\n"
        "[Site \"C:\\\\Chess\"]\n"
        "[Date \"2026.07.15\"]\n"
        "[Round \"1\"]\n"
        "[White \"Alice\"]\n"
        "[Black \"Bob\"]\n"
        "[Result \"*\"]\n"
        "\n"
        "*";

    const Result<ImportedPgn> result =
        fromPgn(pgn);

    if (!result.success) {
        std::cout << "[FAIL] PGN escaped tag import\n";
        std::cout << "       " << result.message << '\n';
        return;
    }

    const bool matches =
        result.value.metadata.event ==
            "John \"The Hammer\"" &&
        result.value.metadata.site ==
            R"(C:\Chess)";

    if (matches) {
        std::cout << "[PASS] PGN escaped tag import\n";
    } else {
        std::cout << "[FAIL] PGN escaped tag import\n";
    }
}
void testInvalidSanInPgn()
{
    const std::string pgn =
        "[Event \"Invalid\"]\n"
        "[Result \"*\"]\n"
        "\n"
        "1. e4 Banana *";

    const Result<ImportedPgn> result =
        fromPgn(pgn);

    if (!result.success) {
        std::cout << "[PASS] PGN rejects invalid SAN\n";
    } else {
        std::cout << "[FAIL] PGN rejects invalid SAN\n";
    }
}
void testInvalidPgnResultTag()
{
    const std::string pgn =
        "[Event \"Invalid Result\"]\n"
        "[Result \"2-0\"]\n"
        "\n"
        "*";

    const Result<ImportedPgn> result =
        fromPgn(pgn);

    if (!result.success) {
        std::cout << "[PASS] PGN rejects invalid result tag\n";
    } else {
        std::cout << "[FAIL] PGN rejects invalid result tag\n";
    }
}
void testPgnRoundTrip()
{
    Game originalGame;

    originalGame.makeMove(Move{
        Square{4, 1},
        Square{4, 3},
        MoveType::Normal
    });

    originalGame.makeMove(Move{
        Square{4, 6},
        Square{4, 4},
        MoveType::Normal
    });

    originalGame.makeMove(Move{
        Square{6, 0},
        Square{5, 2},
        MoveType::Normal
    });

    originalGame.makeMove(Move{
        Square{1, 7},
        Square{2, 5},
        MoveType::Normal
    });

    PgnMetadata metadata;
    metadata.event = "Round Trip";
    metadata.site = "Skopje";
    metadata.date = "2026.07.15";
    metadata.round = "1";
    metadata.white = "Alice";
    metadata.black = "Bob";
    metadata.result = PgnResult::Ongoing;

    const std::string exported =
        toPgn(originalGame, metadata);

    const Result<ImportedPgn> imported =
        fromPgn(exported);

    if (!imported.success) {
        std::cout << "[FAIL] PGN round-trip\n";
        std::cout << "       " << imported.message << '\n';
        return;
    }

    const auto& originalHistory =
        originalGame.moveHistory();

    const auto& importedHistory =
        imported.value.game.moveHistory();

    if (originalHistory.size() != importedHistory.size()) {
        std::cout << "[FAIL] PGN round-trip\n";
        std::cout << "       Move history sizes differ\n";
        return;
    }

    for (std::size_t i = 0;
         i < originalHistory.size();
         ++i) {
        if (!movesEqual(
                originalHistory[i],
                importedHistory[i])) {
            std::cout << "[FAIL] PGN round-trip\n";
            std::cout
                << "       Move differs at index "
                << i
                << '\n';
            return;
        }
    }

    std::cout << "[PASS] PGN round-trip\n";
}
} // namespace

int main()
{
    // ----- toSan tests -----

    testPawnMove();
    testKnightMove();
    testPawnCapture();
    testPieceCapture();
    testFileDisambiguation();
    testRankDisambiguation();
    testFileAndRankDisambiguation();
    testKingsideCastling();
    testQueensideCastling();
    testPromotion();
    testCapturePromotion();
    testOrdinaryCheck();
    testCheckmate();
    testEnPassant();
    testPromotionWithoutPieceType();
    testMoveFromEmptySquare();

    // ----- fromSan tests -----

    testFromSanPawnMove();
    testFromSanKnightMove();
    testFromSanInvalid();
    testFromSanIllegal();
    testSanRoundTripStartingPosition();


    testEmptyGamePgn();
    testBasicPgnMoves();
    testCheckmatePgn();
    testEscapedPgnTags();

    testBasicPgnImport();
    testPgnMetadataImport();
    testEscapedPgnImport();
    testInvalidSanInPgn();
    testInvalidPgnResultTag();
    testPgnRoundTrip();
    return 0;
}