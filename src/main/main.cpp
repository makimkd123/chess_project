#include <iostream>
#include <vector>
#include <string>
#include <optional>

#include "models/Board.h"
#include "models/Pieces.h"
#include "models/Square.h"
#include "models/Move.h"

#include "logic/AttackDetector.h"
#include "logic/MoveGenerator.h"
#include "logic/GameState.h"
#include "logic/Game.h"
#include "logic/CastlingRights.h"
#include "notation/Fen.h"


bool optionalSquareEquals(
    std::optional<Square> square,
    Square expected
) {
    if (!square.has_value()) {
        return false;
    }

    return square->file == expected.file &&
           square->rank == expected.rank;
}

bool containsPromotionMove(
    const std::vector<Move>& moves,
    Square from,
    Square to,
    PieceType promotionType
) {
    for (const Move& move : moves) {
        if (
            move.from.file == from.file &&
            move.from.rank == from.rank &&
            move.to.file == to.file &&
            move.to.rank == to.rank &&
            move.type == MoveType::Promotion &&
            move.promotionType.has_value() &&
            move.promotionType.value() == promotionType
        ) {
            return true;
        }
    }

    return false;
}

int countPromotionMoves(
    const std::vector<Move>& moves,
    Square from,
    Square to
) {
    int count = 0;

    for (const Move& move : moves) {
        if (
            move.from.file == from.file &&
            move.from.rank == from.rank &&
            move.to.file == to.file &&
            move.to.rank == to.rank &&
            move.type == MoveType::Promotion
        ) {
            count++;
        }
    }

    return count;
}
bool sameSquare(Square a, Square b) {
    return a.file == b.file && a.rank == b.rank;
}

bool sameMove(const Move& move, Square from, Square to) {
    return sameSquare(move.from, from) && sameSquare(move.to, to);
}

bool containsMove(
    const std::vector<Move>& moves,
    Square from,
    Square to
) {
    for (const Move& move : moves) {
        if (sameMove(move, from, to)) {
            return true;
        }
    }

    return false;
}

void expectTrue(bool value, const std::string& testName) {
    if (value) {
        std::cout << "[PASS] " << testName << '\n';
    } else {
        std::cout << "[FAIL] " << testName << '\n';
    }
}

void expectFalse(bool value, const std::string& testName) {
    expectTrue(!value, testName);
}

void expectEqualInt(int actual, int expected, const std::string& testName) {
    if (actual == expected) {
        std::cout << "[PASS] " << testName << '\n';
    } else {
        std::cout << "[FAIL] " << testName
                  << " expected " << expected
                  << " but got " << actual << '\n';
    }
}
bool containsMoveOfType(
    const std::vector<Move>& moves,
    Square from,
    Square to,
    MoveType type
) {
    for (const Move& move : moves) {
        if (
            move.from.file == from.file &&
            move.from.rank == from.rank &&
            move.to.file == to.file &&
            move.to.rank == to.rank &&
            move.type == type
        ) {
            return true;
        }
    }

    return false;
}
void testAllPiecesFen() {
    GameState state;
    state.board.clear();

    state.board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({1, 0}, Piece{PieceType::Queen, PieceColor::White});
    state.board.setPiece({2, 0}, Piece{PieceType::Rook, PieceColor::White});
    state.board.setPiece({3, 0}, Piece{PieceType::Bishop, PieceColor::White});
    state.board.setPiece({4, 0}, Piece{PieceType::Knight, PieceColor::White});
    state.board.setPiece({5, 0}, Piece{PieceType::Pawn, PieceColor::White});

    state.board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black});
    state.board.setPiece({1, 7}, Piece{PieceType::Queen, PieceColor::Black});
    state.board.setPiece({2, 7}, Piece{PieceType::Rook, PieceColor::Black});
    state.board.setPiece({3, 7}, Piece{PieceType::Bishop, PieceColor::Black});
    state.board.setPiece({4, 7}, Piece{PieceType::Knight, PieceColor::Black});
    state.board.setPiece({5, 7}, Piece{PieceType::Pawn, PieceColor::Black});

    state.sideToMove = PieceColor::White;
    state.castlingRights = CastlingRights{false, false, false, false};
    state.enPassantTarget = std::nullopt;
    state.halfmoveClock = 0;
    state.fullmoveNumber = 1;

    std::string expected = "kqrbnp2/8/8/8/8/8/8/KQRBNP2 w - - 0 1";

    expectTrue(
        toFen(state) == expected,
        "All piece types export to correct FEN letters"
    );
}
void testFenMetadataFields() {
    GameState state;
    state.board.clear();

    state.board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});

    state.sideToMove = PieceColor::Black;
    state.castlingRights = CastlingRights{true, false, false, true};
    state.enPassantTarget = Square{4, 2}; // e3
    state.halfmoveClock = 7;
    state.fullmoveNumber = 12;

    std::string expected = "4k3/8/8/8/8/8/8/4K3 b Kq e3 7 12";

    expectTrue(
        toFen(state) == expected,
        "FEN exports side, castling, en passant, and move counters"
    );
}
void testSimpleCustomFen() {
    GameState state;

    state.board.clear();

    state.board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});
    state.board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});

    state.sideToMove = PieceColor::White;
    state.castlingRights = CastlingRights{
        false, false,
        false, false
    };
    state.enPassantTarget = std::nullopt;
    state.halfmoveClock = 0;
    state.fullmoveNumber = 1;

    std::string fen = toFen(state);

    std::string expected =
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1";

    if (fen == expected) {
        std::cout << "[PASS] Simple king position exports to correct FEN\n";
    } else {
        std::cout << "[FAIL] Simple king position exports to correct FEN\n";
        std::cout << "Expected: " << expected << '\n';
        std::cout << "Actual:   " << fen << '\n';
    }
}
void testEnPassantFen() {
    GameState state = GameState::startingPosition();

    state.board.makeMove(Move{
        Square{4, 1},
        Square{4, 3},
        MoveType::Normal,
        std::nullopt
    });

    state.sideToMove = PieceColor::Black;
    state.enPassantTarget = Square{4, 2};
    state.halfmoveClock = 0;
    state.fullmoveNumber = 1;

    std::string fen = toFen(state);

    std::string expected =
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";

    if (fen == expected) {
        std::cout << "[PASS] En passant target exports to correct FEN\n";
    } else {
        std::cout << "[FAIL] En passant target exports to correct FEN\n";
        std::cout << "Expected: " << expected << '\n';
        std::cout << "Actual:   " << fen << '\n';
    }
}
void testStartingPositionFen() {
    GameState state = GameState::startingPosition();

    std::string fen = toFen(state);

    std::string expected =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    if (fen == expected) {
        std::cout << "[PASS] Starting position exports to correct FEN\n";
    } else {
        std::cout << "[FAIL] Starting position exports to correct FEN\n";
        std::cout << "Expected: " << expected << '\n';
        std::cout << "Actual:   " << fen << '\n';
    }
}
int main() {
    {
        Board board;
        board.setupStartingPosition();

        auto whiteLegalMoves = MoveGenerator::generateLegalMoves(
            board,
            PieceColor::White
        );

        auto blackLegalMoves = MoveGenerator::generateLegalMoves(
            board,
            PieceColor::Black
        );

        expectEqualInt(
            static_cast<int>(whiteLegalMoves.size()),
            20,
            "White has 20 legal moves in starting position"
        );

        expectEqualInt(
            static_cast<int>(blackLegalMoves.size()),
            20,
            "Black has 20 legal moves in starting position"
        );

        expectFalse(
            AttackDetector::isKingInCheck(board, PieceColor::White),
            "White is not in check in starting position"
        );

        expectFalse(
            AttackDetector::isKingInCheck(board, PieceColor::Black),
            "Black is not in check in starting position"
        );
    }

    {
        Board board;
        board.clear();

        board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
        board.setPiece({4, 7}, Piece{PieceType::Rook, PieceColor::Black}); // e8
        board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black}); // a8

        expectTrue(
            AttackDetector::isKingInCheck(board, PieceColor::White),
            "White king is in check from black rook"
        );
    }

    {
        Board board;
        board.clear();

        board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});   // e1
        board.setPiece({4, 3}, Piece{PieceType::Bishop, PieceColor::White}); // e4 blocker
        board.setPiece({4, 7}, Piece{PieceType::Rook, PieceColor::Black});   // e8
        board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black});   // a8

        expectFalse(
            AttackDetector::isKingInCheck(board, PieceColor::White),
            "White king is not in check when rook is blocked"
        );
    }

    {
        Board board;
        board.clear();

        board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});   // e1
        board.setPiece({5, 2}, Piece{PieceType::Knight, PieceColor::Black}); // f3
        board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black});   // a8

        expectTrue(
            AttackDetector::isKingInCheck(board, PieceColor::White),
            "White king is in check from black knight"
        );
    }

    {
        Board board;
        board.clear();

        board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
        board.setPiece({4, 3}, Piece{PieceType::Pawn, PieceColor::White}); // e4
        board.setPiece({3, 4}, Piece{PieceType::King, PieceColor::Black}); // d5

        expectTrue(
            AttackDetector::isKingInCheck(board, PieceColor::Black),
            "Black king is in check from white pawn"
        );
    }

    {
        Board board;
        board.clear();

        board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
        board.setPiece({4, 1}, Piece{PieceType::Rook, PieceColor::White}); // e2 pinned rook
        board.setPiece({4, 7}, Piece{PieceType::Rook, PieceColor::Black}); // e8
        board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black}); // a8

        auto legalMoves = MoveGenerator::generateLegalMoves(
            board,
            PieceColor::White
        );

        expectFalse(
            containsMove(legalMoves, Square{4, 1}, Square{3, 1}), // e2 to d2
            "Pinned white rook cannot move away and expose king"
        );

        expectTrue(
            containsMove(legalMoves, Square{4, 1}, Square{4, 7}), // e2 to e8
            "Pinned white rook can capture checking rook"
        );
    }
    {
        Board board;
        board.clear();

        board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
        board.setPiece({1, 1}, Piece{PieceType::Queen, PieceColor::Black}); // b2
        board.setPiece({2, 2}, Piece{PieceType::King, PieceColor::Black});  // c3

        GameState state;
        state.board = board;
        state.sideToMove = PieceColor::White;

        GameStatus status = state.getStatus();

        expectTrue(
            status == GameStatus::Checkmate,
            "White is checkmated"
        );
    }
    {
    Board board;
    board.clear();

    board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
    board.setPiece({2, 1}, Piece{PieceType::Queen, PieceColor::Black}); // c2
    board.setPiece({2, 2}, Piece{PieceType::King, PieceColor::Black});  // c3


    GameState state;
    state.board = board;
    state.sideToMove = PieceColor::White;

    GameStatus status = state.getStatus();

    expectTrue(
        status == GameStatus::Stalemate,
        "White is stalemated"
    );
}
{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({4, 7}, Piece{PieceType::Rook, PieceColor::Black}); // e8
    board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black}); // a8

    GameState state;
    state.board = board;
    state.sideToMove = PieceColor::White;

    GameStatus status = state.getStatus();

    expectTrue(
        status == GameStatus::Check,
        "White is in check but not checkmated"
    );
}
{
    Board board;
    board.setupStartingPosition();

    GameState state;
    state.board = board;
    state.sideToMove = PieceColor::White;

    GameStatus status = state.getStatus();

    expectTrue(
        status == GameStatus::Ongoing,
        "Starting position is ongoing"
    );
}

{
    Board board;
    board.clear();

    board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
    board.setPiece({7, 7}, Piece{PieceType::King, PieceColor::Black}); // h8

    board.setPiece({4, 6}, Piece{PieceType::Pawn, PieceColor::White}); // e7

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::White
    );

    expectEqualInt(
        countPromotionMoves(moves, Square{4, 6}, Square{4, 7}),
        4,
        "White pawn has 4 promotion choices on e8"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 6}, Square{4, 7}, PieceType::Queen),
        "White pawn can promote to queen"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 6}, Square{4, 7}, PieceType::Rook),
        "White pawn can promote to rook"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 6}, Square{4, 7}, PieceType::Bishop),
        "White pawn can promote to bishop"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 6}, Square{4, 7}, PieceType::Knight),
        "White pawn can promote to knight"
    );
}
{
    Board board;
    board.clear();

    board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
    board.setPiece({7, 7}, Piece{PieceType::King, PieceColor::Black}); // h8

    board.setPiece({4, 1}, Piece{PieceType::Pawn, PieceColor::Black}); // e2

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::Black
    );

    expectEqualInt(
        countPromotionMoves(moves, Square{4, 1}, Square{4, 0}),
        4,
        "Black pawn has 4 promotion choices on e1"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 1}, Square{4, 0}, PieceType::Queen),
        "Black pawn can promote to queen"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 1}, Square{4, 0}, PieceType::Rook),
        "Black pawn can promote to rook"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 1}, Square{4, 0}, PieceType::Bishop),
        "Black pawn can promote to bishop"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 1}, Square{4, 0}, PieceType::Knight),
        "Black pawn can promote to knight"
    );
}{
    Board board;
    board.clear();

    board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
    board.setPiece({7, 7}, Piece{PieceType::King, PieceColor::Black}); // h8

    board.setPiece({4, 6}, Piece{PieceType::Pawn, PieceColor::White}); // e7
    board.setPiece({3, 7}, Piece{PieceType::Rook, PieceColor::Black}); // d8

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::White
    );

    expectEqualInt(
        countPromotionMoves(moves, Square{4, 6}, Square{3, 7}),
        4,
        "White pawn has 4 capture-promotion choices on d8"
    );

    expectTrue(
        containsPromotionMove(moves, Square{4, 6}, Square{3, 7}, PieceType::Queen),
        "White pawn can capture-promote to queen"
    );
}{
    Board board;
    board.clear();

    board.setPiece({0, 0}, Piece{PieceType::King, PieceColor::White}); // a1
    board.setPiece({7, 7}, Piece{PieceType::King, PieceColor::Black}); // h8

    board.setPiece({4, 6}, Piece{PieceType::Pawn, PieceColor::White}); // e7

    Move promotionMove{
        Square{4, 6},
        Square{4, 7},
        MoveType::Promotion,
        PieceType::Queen
    };

    bool moveWorked = board.makeMove(promotionMove);

    auto promotedPiece = board.pieceAt({4, 7});
    auto oldSquare = board.pieceAt({4, 6});

    expectTrue(
        moveWorked,
        "Promotion move is accepted by Board::makeMove"
    );

    expectTrue(
        promotedPiece.has_value() &&
        promotedPiece->type == PieceType::Queen &&
        promotedPiece->color == PieceColor::White,
        "White pawn promotes to queen on e8"
    );

    expectTrue(
        !oldSquare.has_value(),
        "Original pawn square is empty after promotion"
    );
}
{
    Game game;

    expectTrue(
        game.sideToMove() == PieceColor::White,
        "New game starts with White to move"
    );

    expectEqualInt(
        static_cast<int>(game.moveHistory().size()),
        0,
        "New game starts with empty move history"
    );

    expectTrue(
        game.status() == GameStatus::Ongoing,
        "New game status is ongoing"
    );

    expectTrue(
        game.castlingRights().whiteKingside,
        "White starts with kingside castling rights"
    );

    expectTrue(
        game.castlingRights().whiteQueenside,
        "White starts with queenside castling rights"
    );

    expectTrue(
        game.castlingRights().blackKingside,
        "Black starts with kingside castling rights"
    );

    expectTrue(
        game.castlingRights().blackQueenside,
        "Black starts with queenside castling rights"
    );

    expectTrue(
        !game.enPassantTarget().has_value(),
        "New game starts with no en passant target"
    );
}{
    Game game;

    Move move{
        Square{4, 1}, // e2
        Square{4, 3}, // e4
        MoveType::Normal
    };

    bool moveWorked = game.makeMove(move);

    auto pieceOnE4 = game.board().pieceAt({4, 3});
    auto pieceOnE2 = game.board().pieceAt({4, 1});

    expectTrue(
        moveWorked,
        "Game accepts legal move e2 to e4"
    );

    expectTrue(
        pieceOnE4.has_value() &&
        pieceOnE4->type == PieceType::Pawn &&
        pieceOnE4->color == PieceColor::White,
        "White pawn is on e4 after e2 to e4"
    );

    expectTrue(
        !pieceOnE2.has_value(),
        "e2 is empty after e2 to e4"
    );

    expectTrue(
        game.sideToMove() == PieceColor::Black,
        "Side to move switches to Black after White move"
    );

    expectEqualInt(
        static_cast<int>(game.moveHistory().size()),
        1,
        "Move history contains one move after e2 to e4"
    );

    expectTrue(
        optionalSquareEquals(game.enPassantTarget(), Square{4, 2}),
        "e2 to e4 sets en passant target to e3"
    );
}{
    Game game;

    Move illegalMove{
        Square{4, 1}, // e2
        Square{4, 4}, // e5
        MoveType::Normal
    };

    bool moveWorked = game.makeMove(illegalMove);

    expectFalse(
        moveWorked,
        "Game rejects illegal pawn move e2 to e5"
    );

    expectTrue(
        game.sideToMove() == PieceColor::White,
        "Side to move does not change after illegal move"
    );

    expectEqualInt(
        static_cast<int>(game.moveHistory().size()),
        0,
        "Move history does not change after illegal move"
    );
}{
    Game game;

    bool whiteMoveWorked = game.makeMove(
        Move{
            Square{4, 1}, // e2
            Square{4, 3}, // e4
            MoveType::Normal
        }
    );

    bool secondWhiteMoveWorked = game.makeMove(
        Move{
            Square{6, 0}, // g1
            Square{5, 2}, // f3
            MoveType::Normal
        }
    );

    expectTrue(
        whiteMoveWorked,
        "First White move e2 to e4 works"
    );

    expectFalse(
        secondWhiteMoveWorked,
        "White cannot move twice in a row"
    );

    expectTrue(
        game.sideToMove() == PieceColor::Black,
        "Side remains Black after rejected second White move"
    );

    expectEqualInt(
        static_cast<int>(game.moveHistory().size()),
        1,
        "Rejected wrong-side move is not added to history"
    );
}{
    Game game;

    game.makeMove(
        Move{
            Square{4, 1}, // e2
            Square{4, 3}, // e4
            MoveType::Normal
        }
    );

    expectTrue(
        optionalSquareEquals(game.enPassantTarget(), Square{4, 2}),
        "e2 to e4 creates en passant target e3"
    );

    game.makeMove(
        Move{
            Square{6, 7}, // g8
            Square{5, 5}, // f6
            MoveType::Normal
        }
    );

    expectTrue(
        !game.enPassantTarget().has_value(),
        "En passant target clears after non-double-pawn move"
    );
}{
    Game game;

    game.makeMove(
        Move{
            Square{4, 1}, // e2
            Square{4, 3}, // e4
            MoveType::Normal
        }
    );

    game.makeMove(
        Move{
            Square{6, 7}, // g8
            Square{5, 5}, // f6
            MoveType::Normal
        }
    );

    bool kingMoveWorked = game.makeMove(
        Move{
            Square{4, 0}, // e1
            Square{4, 1}, // e2
            MoveType::Normal
        }
    );

    expectTrue(
        kingMoveWorked,
        "White king can move from e1 to e2 after e-pawn moves"
    );

    expectFalse(
        game.castlingRights().whiteKingside,
        "White loses kingside castling rights after king moves"
    );

    expectFalse(
        game.castlingRights().whiteQueenside,
        "White loses queenside castling rights after king moves"
    );
}{
    Game game;

    bool knightMoveWorked = game.makeMove(
        Move{
            Square{6, 0}, // g1
            Square{5, 2}, // f3
            MoveType::Normal
        }
    );

    bool blackMoveWorked = game.makeMove(
        Move{
            Square{6, 7}, // g8
            Square{5, 5}, // f6
            MoveType::Normal
        }
    );

    bool rookMoveWorked = game.makeMove(
        Move{
            Square{7, 0}, // h1
            Square{6, 0}, // g1
            MoveType::Normal
        }
    );

    expectTrue(
        knightMoveWorked,
        "White knight moves from g1 to f3"
    );

    expectTrue(
        blackMoveWorked,
        "Black knight moves from g8 to f6"
    );

    expectTrue(
        rookMoveWorked,
        "White rook moves from h1 to g1"
    );

    expectFalse(
        game.castlingRights().whiteKingside,
        "White loses kingside castling rights after h1 rook moves"
    );

    expectTrue(
        game.castlingRights().whiteQueenside,
        "White keeps queenside castling rights after h1 rook moves"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({7, 0}, Piece{PieceType::Rook, PieceColor::White}); // h1
    board.setPiece({0, 0}, Piece{PieceType::Rook, PieceColor::White}); // a1

    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8

    CastlingRights rights;
    rights.whiteKingside = true;
    rights.whiteQueenside = true;
    rights.blackKingside = false;
    rights.blackQueenside = false;

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::White,
        rights
    );

    expectTrue(
        containsMoveOfType(moves, Square{4, 0}, Square{6, 0}, MoveType::Castle),
        "White kingside castling move is generated"
    );

    expectTrue(
        containsMoveOfType(moves, Square{4, 0}, Square{2, 0}, MoveType::Castle),
        "White queenside castling move is generated"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8
    board.setPiece({7, 7}, Piece{PieceType::Rook, PieceColor::Black}); // h8
    board.setPiece({0, 7}, Piece{PieceType::Rook, PieceColor::Black}); // a8

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1

    CastlingRights rights;
    rights.whiteKingside = false;
    rights.whiteQueenside = false;
    rights.blackKingside = true;
    rights.blackQueenside = true;

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::Black,
        rights
    );

    expectTrue(
        containsMoveOfType(moves, Square{4, 7}, Square{6, 7}, MoveType::Castle),
        "Black kingside castling move is generated"
    );

    expectTrue(
        containsMoveOfType(moves, Square{4, 7}, Square{2, 7}, MoveType::Castle),
        "Black queenside castling move is generated"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White});   // e1
    board.setPiece({7, 0}, Piece{PieceType::Rook, PieceColor::White});   // h1
    board.setPiece({5, 0}, Piece{PieceType::Bishop, PieceColor::White}); // f1 blocker

    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black});   // e8

    CastlingRights rights;
    rights.whiteKingside = true;
    rights.whiteQueenside = false;
    rights.blackKingside = false;
    rights.blackQueenside = false;

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::White,
        rights
    );

    expectFalse(
        containsMoveOfType(moves, Square{4, 0}, Square{6, 0}, MoveType::Castle),
        "White cannot castle kingside when f1 is occupied"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({7, 0}, Piece{PieceType::Rook, PieceColor::White}); // h1

    board.setPiece({0, 7}, Piece{PieceType::King, PieceColor::Black}); // a8
    board.setPiece({5, 7}, Piece{PieceType::Rook, PieceColor::Black}); // f8 attacks f1

    CastlingRights rights;
    rights.whiteKingside = true;
    rights.whiteQueenside = false;
    rights.blackKingside = false;
    rights.blackQueenside = false;

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::White,
        rights
    );

    expectFalse(
        containsMoveOfType(moves, Square{4, 0}, Square{6, 0}, MoveType::Castle),
        "White cannot castle through attacked f1 square"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({7, 0}, Piece{PieceType::Rook, PieceColor::White}); // h1
    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8

    Move castleMove{
        Square{4, 0},
        Square{6, 0},
        MoveType::Castle
    };

    bool moveWorked = board.makeMove(castleMove);

    auto kingOnG1 = board.pieceAt({6, 0});
    auto rookOnF1 = board.pieceAt({5, 0});
    auto oldKingSquare = board.pieceAt({4, 0});
    auto oldRookSquare = board.pieceAt({7, 0});

    expectTrue(
        moveWorked,
        "Board accepts white kingside castling move"
    );

    expectTrue(
        kingOnG1.has_value() &&
        kingOnG1->type == PieceType::King &&
        kingOnG1->color == PieceColor::White,
        "White king is on g1 after castling"
    );

    expectTrue(
        rookOnF1.has_value() &&
        rookOnF1->type == PieceType::Rook &&
        rookOnF1->color == PieceColor::White,
        "White rook is on f1 after castling"
    );

    expectTrue(
        !oldKingSquare.has_value(),
        "e1 is empty after castling"
    );

    expectTrue(
        !oldRookSquare.has_value(),
        "h1 is empty after castling"
    );
}{
    Game game;

    bool move1 = game.makeMove(
        Move{
            Square{4, 1}, // e2
            Square{4, 3}, // e4
            MoveType::Normal
        }
    );

    bool move2 = game.makeMove(
        Move{
            Square{4, 6}, // e7
            Square{4, 4}, // e5
            MoveType::Normal
        }
    );

    bool move3 = game.makeMove(
        Move{
            Square{6, 0}, // g1
            Square{5, 2}, // f3
            MoveType::Normal
        }
    );

    bool move4 = game.makeMove(
        Move{
            Square{1, 7}, // b8
            Square{2, 5}, // c6
            MoveType::Normal
        }
    );

    bool move5 = game.makeMove(
        Move{
            Square{5, 0}, // f1
            Square{4, 1}, // e2
            MoveType::Normal
        }
    );

    bool move6 = game.makeMove(
        Move{
            Square{6, 7}, // g8
            Square{5, 5}, // f6
            MoveType::Normal
        }
    );

    bool castleWorked = game.makeMove(
        Move{
            Square{4, 0}, // e1
            Square{6, 0}, // g1
            MoveType::Castle
        }
    );

    auto kingOnG1 = game.board().pieceAt({6, 0});
    auto rookOnF1 = game.board().pieceAt({5, 0});

    expectTrue(
        move1 && move2 && move3 && move4 && move5 && move6,
        "Setup moves before castling all work"
    );

    expectTrue(
        castleWorked,
        "Game accepts white kingside castling after path is clear"
    );

    expectTrue(
        kingOnG1.has_value() &&
        kingOnG1->type == PieceType::King &&
        kingOnG1->color == PieceColor::White,
        "Game places white king on g1 after castling"
    );

    expectTrue(
        rookOnF1.has_value() &&
        rookOnF1->type == PieceType::Rook &&
        rookOnF1->color == PieceColor::White,
        "Game places white rook on f1 after castling"
    );

    expectFalse(
        game.castlingRights().whiteKingside,
        "White loses kingside castling rights after castling"
    );

    expectFalse(
        game.castlingRights().whiteQueenside,
        "White loses queenside castling rights after castling"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8

    board.setPiece({4, 4}, Piece{PieceType::Pawn, PieceColor::White}); // e5
    board.setPiece({3, 4}, Piece{PieceType::Pawn, PieceColor::Black}); // d5

    CastlingRights rights{
        false,
        false,
        false,
        false
    };

    std::optional<Square> enPassantTarget = Square{3, 5}; // d6

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::White,
        rights,
        enPassantTarget
    );

    expectTrue(
        containsMoveOfType(moves, Square{4, 4}, Square{3, 5}, MoveType::EnPassant),
        "White en passant move e5 to d6 is generated"
    );
}
{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8

    board.setPiece({4, 3}, Piece{PieceType::Pawn, PieceColor::Black}); // e4
    board.setPiece({3, 3}, Piece{PieceType::Pawn, PieceColor::White}); // d4

    CastlingRights rights{
        false,
        false,
        false,
        false
    };

    std::optional<Square> enPassantTarget = Square{3, 2}; // d3

    auto moves = MoveGenerator::generateLegalMoves(
        board,
        PieceColor::Black,
        rights,
        enPassantTarget
    );

    expectTrue(
        containsMoveOfType(moves, Square{4, 3}, Square{3, 2}, MoveType::EnPassant),
        "Black en passant move e4 to d3 is generated"
    );
}
{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8

    board.setPiece({4, 4}, Piece{PieceType::Pawn, PieceColor::White}); // e5
    board.setPiece({3, 4}, Piece{PieceType::Pawn, PieceColor::Black}); // d5

    Move enPassantMove{
        Square{4, 4}, // e5
        Square{3, 5}, // d6
        MoveType::EnPassant
    };

    bool moveWorked = board.makeMove(enPassantMove);

    auto whitePawnOnD6 = board.pieceAt({3, 5});
    auto oldWhitePawnSquare = board.pieceAt({4, 4});
    auto capturedBlackPawnSquare = board.pieceAt({3, 4});

    expectTrue(
        moveWorked,
        "Board accepts white en passant move e5 to d6"
    );

    expectTrue(
        whitePawnOnD6.has_value() &&
        whitePawnOnD6->type == PieceType::Pawn &&
        whitePawnOnD6->color == PieceColor::White,
        "White pawn is on d6 after en passant"
    );

    expectTrue(
        !oldWhitePawnSquare.has_value(),
        "e5 is empty after white en passant"
    );

    expectTrue(
        !capturedBlackPawnSquare.has_value(),
        "Black pawn on d5 is removed by en passant"
    );
}{
    Board board;
    board.clear();

    board.setPiece({4, 0}, Piece{PieceType::King, PieceColor::White}); // e1
    board.setPiece({4, 7}, Piece{PieceType::King, PieceColor::Black}); // e8

    board.setPiece({4, 3}, Piece{PieceType::Pawn, PieceColor::Black}); // e4
    board.setPiece({3, 3}, Piece{PieceType::Pawn, PieceColor::White}); // d4

    Move enPassantMove{
        Square{4, 3}, // e4
        Square{3, 2}, // d3
        MoveType::EnPassant
    };

    bool moveWorked = board.makeMove(enPassantMove);

    auto blackPawnOnD3 = board.pieceAt({3, 2});
    auto oldBlackPawnSquare = board.pieceAt({4, 3});
    auto capturedWhitePawnSquare = board.pieceAt({3, 3});

    expectTrue(
        moveWorked,
        "Board accepts black en passant move e4 to d3"
    );

    expectTrue(
        blackPawnOnD3.has_value() &&
        blackPawnOnD3->type == PieceType::Pawn &&
        blackPawnOnD3->color == PieceColor::Black,
        "Black pawn is on d3 after en passant"
    );

    expectTrue(
        !oldBlackPawnSquare.has_value(),
        "e4 is empty after black en passant"
    );

    expectTrue(
        !capturedWhitePawnSquare.has_value(),
        "White pawn on d4 is removed by en passant"
    );
}{
    Game game;

    bool move1 = game.makeMove(
        Move{
            Square{4, 1}, // e2
            Square{4, 3}, // e4
            MoveType::Normal
        }
    );

    bool move2 = game.makeMove(
        Move{
            Square{0, 6}, // a7
            Square{0, 5}, // a6
            MoveType::Normal
        }
    );

    bool move3 = game.makeMove(
        Move{
            Square{4, 3}, // e4
            Square{4, 4}, // e5
            MoveType::Normal
        }
    );

    bool move4 = game.makeMove(
        Move{
            Square{3, 6}, // d7
            Square{3, 4}, // d5
            MoveType::Normal
        }
    );

    expectTrue(
        optionalSquareEquals(game.enPassantTarget(), Square{3, 5}),
        "d7 to d5 creates en passant target d6"
    );

    bool enPassantWorked = game.makeMove(
        Move{
            Square{4, 4}, // e5
            Square{3, 5}, // d6
            MoveType::EnPassant
        }
    );

    auto whitePawnOnD6 = game.board().pieceAt({3, 5});
    auto blackPawnOnD5 = game.board().pieceAt({3, 4});

    expectTrue(
        move1 && move2 && move3 && move4,
        "Setup moves before white en passant all work"
    );

    expectTrue(
        enPassantWorked,
        "Game accepts white en passant e5 to d6"
    );

    expectTrue(
        whitePawnOnD6.has_value() &&
        whitePawnOnD6->type == PieceType::Pawn &&
        whitePawnOnD6->color == PieceColor::White,
        "White pawn is on d6 after Game en passant"
    );

    expectTrue(
        !blackPawnOnD5.has_value(),
        "Black pawn on d5 is removed after Game en passant"
    );

    expectTrue(
        game.sideToMove() == PieceColor::Black,
        "Side switches to Black after white en passant"
    );
}
{
    Game game;

    bool move1 = game.makeMove(
        Move{
            Square{4, 1}, // e2
            Square{4, 3}, // e4
            MoveType::Normal
        }
    );

    bool move2 = game.makeMove(
        Move{
            Square{0, 6}, // a7
            Square{0, 5}, // a6
            MoveType::Normal
        }
    );

    bool move3 = game.makeMove(
        Move{
            Square{4, 3}, // e4
            Square{4, 4}, // e5
            MoveType::Normal
        }
    );

    bool move4 = game.makeMove(
        Move{
            Square{3, 6}, // d7
            Square{3, 4}, // d5
            MoveType::Normal
        }
    );

    bool waitingMove = game.makeMove(
        Move{
            Square{6, 0}, // g1
            Square{5, 2}, // f3
            MoveType::Normal
        }
    );

    bool blackReply = game.makeMove(
        Move{
            Square{6, 7}, // g8
            Square{5, 5}, // f6
            MoveType::Normal
        }
    );

    bool lateEnPassant = game.makeMove(
        Move{
            Square{4, 4}, // e5
            Square{3, 5}, // d6
            MoveType::EnPassant
        }
    );

    expectTrue(
        move1 && move2 && move3 && move4,
        "Setup moves before missed en passant all work"
    );

    expectTrue(
        waitingMove && blackReply,
        "Both waiting moves after en passant opportunity work"
    );

    expectFalse(
        lateEnPassant,
        "En passant is not allowed after one move has passed"
    );
}testStartingPositionFen();
testSimpleCustomFen();
testFenMetadataFields();
testAllPiecesFen();
return 0; }