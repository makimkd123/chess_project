#include "AttackDetector.h"

namespace{

bool hasPiece(const Board& board, Square square, PieceColor color, PieceType type){
    auto piece = board.pieceAt(square);
    if(!piece.has_value()){
        return false;
    }

    return piece->color==color && piece->type==type;
}

bool rayContainsAttacker(const Board& board, Square from, Square direction, PieceColor bySide, PieceType type){
    for(int distance=1; distance<=7;distance++){
        Square current{from.file + direction.file*distance,
        from.rank + direction.rank*distance};

        if(!board.isInside(current)){
            break;
        }

        auto piece=board.pieceAt(current);

        if(!piece.has_value()){
            continue;
        }

        if (piece->color != bySide) {
            return false;
        }

            return piece->type == type ||
               piece->type == PieceType::Queen;
        }
    return false;
}
}



bool AttackDetector::isSquareAttacked(const Board& board,Square square,PieceColor bySide){
    // Pawns
    int pawnRankOffset = 0;

    if (bySide == PieceColor::White) {
        pawnRankOffset = -1;
    } else {
        pawnRankOffset = 1;
    }

    if (hasPiece(board,Square{square.file - 1, square.rank + pawnRankOffset},bySide,PieceType::Pawn)
     ||
        hasPiece(board,Square{square.file + 1, square.rank + pawnRankOffset},bySide,PieceType::Pawn)
    ) {
        return true;
    }

    // Knights
    const Square knightOffsets[] = {
        {1,2},
        {1,-2},
        {2,1},
        {2,-1},
        {-1,2},
        {-1,-2},
        {-2,1},
        {-2,-1}
    };

    for (Square offset : knightOffsets) {
        Square attackerSquare{
            square.file + offset.file,
            square.rank + offset.rank
        };

        if (hasPiece(board, attackerSquare, bySide, PieceType::Knight)) {
            return true;
        }
    }

    // Rooks and queens: horizontal / vertical rays
    const Square rookDirections[] = {
        {1,0},
        {-1,0},
        {0,1},
        {0,-1}
    };

    for (Square direction : rookDirections) {
        if (rayContainsAttacker(board,square,direction,bySide,PieceType::Rook)){
            return true;
        }
    }

    // Bishops and queens: diagonal rays
    const Square bishopDirections[] = {
        {1,1},
        {1,-1},
        {-1,1},
        {-1,-1}
    };

    for (Square direction : bishopDirections) {
        if (rayContainsAttacker(board,square,direction,bySide,PieceType::Bishop)) {
            return true;
        }
    }

    // Kings
    const Square kingOffsets[] = {
        {1,0},
        {1,1},
        {0,1},
        {-1,1},
        {-1,0},
        {-1,-1},
        {0,-1},
        {1,-1}
    };

    for (Square offset : kingOffsets) {
        Square attackerSquare{
            square.file + offset.file,
            square.rank + offset.rank
        };

        if (hasPiece(board, attackerSquare, bySide, PieceType::King)){
            return true;
        }
    }

    return false;
}

bool AttackDetector::isKingInCheck(
    const Board& board,
    PieceColor kingColor
) {
    auto kingSquare = findKing(board, kingColor);

    if(!kingSquare.has_value()){
        return false;
    }

    PieceColor enemyColor;

    if(kingColor==PieceColor::White){
        enemyColor=PieceColor::Black;
    }else{
        enemyColor=PieceColor::White;
    }

    return isSquareAttacked(board, *kingSquare, enemyColor);

}

std::optional<Square> AttackDetector::findKing(
    const Board& board,
    PieceColor kingColor
) {
    for(int file=0; file<8; file++){
        for(int rank=0; rank<8; rank++){
            Square square{file,rank};

            auto piece = board.pieceAt(square);

            if(!piece.has_value()){
                continue;
            }
            if(piece->type == PieceType::King && piece->color == kingColor){
                return square;
            }
        }
    }
    return std::nullopt;
}