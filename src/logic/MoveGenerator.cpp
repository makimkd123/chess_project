#include "MoveGenerator.h"
#include "AttackDetector.h"

namespace{ 

    bool isPromotionRank(Square square, PieceColor color){
    if(color == PieceColor::White){
        return square.rank==7;
    }
    return square.rank==0;
}

void addPromotionMoves(Square from,Square to,std::vector<Move>& moves){
    moves.push_back(Move{from, to, MoveType::Promotion, PieceType::Queen});
    moves.push_back(Move{from, to, MoveType::Promotion, PieceType::Rook});
    moves.push_back(Move{from, to, MoveType::Promotion, PieceType::Bishop});
    moves.push_back(Move{from, to, MoveType::Promotion, PieceType::Knight});
}

PieceColor oppositeColor(PieceColor color) {
    if (color == PieceColor::White){
        return PieceColor::Black;
    }

    return PieceColor::White;
}

bool hasRook(const Board& board,Square square,PieceColor color){
    auto piece = board.pieceAt(square);

    return piece.has_value() &&
           piece->type == PieceType::Rook &&
           piece->color == color;
}


}
std::vector<Move> MoveGenerator::generatePseudoLegalMoves(const Board& board,PieceColor sideToMove, const CastlingRights& castlingRights, std::optional<Square> enPassantTarget){
    std::vector<Move> moves;

    for(int rank=0; rank<8; rank++){
        for(int file=0; file<8; file++){
            Square from(file,rank);
            auto piece=board.pieceAt(from);

            if(!piece.has_value()){
                continue;
            }

            if(piece->color !=sideToMove){
                continue;
            }

            switch(piece->type){
                case PieceType::Pawn:
                generatePawnMoves(board, from, *piece, enPassantTarget, moves);
                break;
                case PieceType::Knight:
                generateKnightMoves(board, from, *piece,moves);
                break;
                case PieceType::Bishop:
                generateBishopMoves(board, from, *piece,moves);
                break;
                case PieceType::Rook:
                generateRookMoves(board, from, *piece,moves);
                break;
                case PieceType::Queen:
                generateQueenMoves(board, from ,*piece,moves);
                break;
                case PieceType::King:
                generateKingMoves(board, from, *piece,moves);
                generateCastlingMoves(board, from, *piece, castlingRights, moves);
                break;

            }
        }
    }
    return moves;
}

void MoveGenerator::generatePawnMoves(
    const Board& board,
    Square from,
    Piece piece,
    std::optional<Square> enPassantTarget,
    std::vector<Move>& moves
) {
    int direction = 0;
    int startRank = 0;

    if (piece.color == PieceColor::White) {
        direction = 1;
        startRank = 1;
    } else {
        direction = -1;
        startRank = 6;
    }

    Square oneForward{from.file, from.rank + direction};

    if (board.isInside(oneForward) && board.isEmpty(oneForward)) {
        if (isPromotionRank(oneForward, piece.color)) {
            addPromotionMoves(from, oneForward, moves);
        } else {
            moves.push_back(Move{from, oneForward, MoveType::Normal});

            Square twoForward{from.file, from.rank + 2 * direction};

            if (
                from.rank == startRank &&
                board.isInside(twoForward) &&
                board.isEmpty(twoForward)
            ) {
                moves.push_back(Move{from, twoForward, MoveType::Normal});
            }
        }
    }

    for (int fileOffset : {-1, 1}) {
        Square target{from.file + fileOffset, from.rank + direction};

        if (!board.isInside(target)) {
            continue;
        }

        auto targetPiece = board.pieceAt(target);

        if (targetPiece.has_value() && targetPiece->color != piece.color) {
            if (isPromotionRank(target, piece.color)) {
                addPromotionMoves(from, target, moves);
            } else {
                moves.push_back(Move{from, target, MoveType::Capture});
            }
        }

        if (
            enPassantTarget.has_value() &&
            target.file == enPassantTarget->file &&
            target.rank == enPassantTarget->rank
        ) {
            Square capturedPawnSquare{target.file, from.rank};

            auto capturedPawn = board.pieceAt(capturedPawnSquare);

            if (
                capturedPawn.has_value() &&
                capturedPawn->type == PieceType::Pawn &&
                capturedPawn->color != piece.color
            ) {
                moves.push_back(Move{from, target, MoveType::EnPassant});
            }
        }
    }
}

void MoveGenerator::generateKnightMoves(const Board& board, Square from, Piece piece, std::vector<Move>& moves){

    std::vector<Square> offsets{
        {1,2},
        {1,-2},
        {2,1},
        {2,-1},
        {-1,2},
        {-1,-2},
        {-2,1},
        {-2,-1},
    };
    for(Square offset:offsets){
        Square target(from.file + offset.file, from.rank + offset.rank);

        if(!board.isInside(target)){
            continue;
        }

        auto targetPiece = board.pieceAt(target);

        if(!targetPiece.has_value()){
            moves.push_back(Move{from, target, MoveType::Normal});
            continue;
        }

        if(targetPiece->color !=piece.color){
            moves.push_back(Move{from, target, MoveType::Capture});
        }
    }
}

void MoveGenerator::generateSlidingMoves(const Board& board,Square from,Piece piece,const std::vector<Square>& directions,std::vector<Move>& moves){
    for(Square direction:directions){
        for(int distance=1; distance<=7; distance++){

        Square target{from.file + direction.file*distance,
            from.rank + direction.rank*distance,};

        if(!board.isInside(target)){
            break;
        }
        auto targetPiece=board.pieceAt(target);

        if(!targetPiece.has_value()){
            moves.push_back(Move{from, target, MoveType::Normal});
            continue;
        }

        if(targetPiece->color !=piece.color){
            moves.push_back(Move{from, target, MoveType::Capture});
        }
        break;
        }
    }
}
   

void MoveGenerator::generateBishopMoves(const Board& board,Square from,Piece piece,std::vector<Move>& moves){
    const std::vector<Square> bishopDirections {
        {1, 1},
        {1,-1},
        {-1,1},
        {-1,-1}
    };
    generateSlidingMoves(board,from,piece,bishopDirections,moves);

}

void MoveGenerator::generateRookMoves(const Board& board, Square from,Piece piece,std::vector<Move>& moves){
    const std::vector<Square> rookDirections{
        {1,0},
        {0,1},
        {-1,0},
        {0,-1}
    };
    generateSlidingMoves(board,from,piece,rookDirections,moves);
}

void MoveGenerator::generateQueenMoves(const Board& board, Square from,Piece piece,std::vector<Move>& moves){
    const std::vector<Square> queenDirections{
        {1, 1},
        {1,-1},
        {-1,1},
        {-1,-1},
        {1,0},
        {0,1},
        {-1,0},
        {0,-1}
    };
    generateSlidingMoves(board,from,piece,queenDirections,moves);
}

void MoveGenerator::generateKingMoves(const Board& board,Square from,Piece piece,std::vector<Move>& moves){
    const std::vector<Square> kingDirections{
        {1,0},
        {1,1},
        {0,1},
        {-1,1},
        {-1,0},
        {-1,-1},
        {0,-1},
        {1,-1}
    };

    for(Square direction:kingDirections){
        Square target{from.file + direction.file,
            from.rank + direction.rank};

        if(!board.isInside(target)){
            continue;
        }
        auto targetPiece=board.pieceAt(target);

        if(!targetPiece.has_value()){
            moves.push_back(Move{from, target, MoveType::Normal});
            continue;
        }

        if(targetPiece->color !=piece.color){
            moves.push_back(Move{from, target, MoveType::Capture});
        }
    }
}

std::vector<Move> MoveGenerator::generateLegalMoves(const Board& board, PieceColor sideToMove, const CastlingRights& castlingRights, std::optional<Square> enPassantTarget){
    std::vector<Move> pseudoLegalMoves = generatePseudoLegalMoves(board,sideToMove,castlingRights,enPassantTarget);
    std::vector<Move> legalMoves;
    
    for(const Move& move : pseudoLegalMoves){
        Board boardAfterMove = board;
        bool moveWasApplied = boardAfterMove.makeMove(move);

        if(!moveWasApplied){
            continue;
        }
        if(!AttackDetector::isKingInCheck(boardAfterMove,sideToMove)){
            legalMoves.push_back(move);
        }
    }
    return legalMoves;
}

void MoveGenerator::generateCastlingMoves(const Board& board, Square from, Piece piece, const CastlingRights& castlingRights,std::vector<Move>& moves){
    if(piece.type!=PieceType::King){
        return;
    }

    PieceColor enemyColor = oppositeColor(piece.color);

    if(AttackDetector::isSquareAttacked(board, from, enemyColor)){
        return;
    }

    if(piece.color==PieceColor::White){
        if(from.file!=4 || from.rank!=0){
            return;
        }
        if(castlingRights.whiteKingside && 
            hasRook(board,Square{7,0},PieceColor::White)&&
            board.isEmpty(Square{6,0})&&
            board.isEmpty(Square{5,0})&&
            !AttackDetector::isSquareAttacked(board,Square{6,0},enemyColor)&&
            !AttackDetector::isSquareAttacked(board,Square{5,0},enemyColor)           
        ){
            moves.push_back(Move{from,Square{6,0},MoveType::Castle});
        }
        if(castlingRights.whiteQueenside && 
            hasRook(board,Square{0,0},PieceColor::White)&&
            board.isEmpty(Square{1,0})&&
            board.isEmpty(Square{2,0})&&
            board.isEmpty(Square{3,0})&&
            !AttackDetector::isSquareAttacked(board,Square{2,0},enemyColor)&&            
            !AttackDetector::isSquareAttacked(board,Square{3,0},enemyColor)           
        ){
            moves.push_back(Move{from,Square{2,0},MoveType::Castle});
        }
        return;
    }


    if(piece.color==PieceColor::Black){
        if(from.file!=4 || from.rank!=7){
            return;
        }
        if(castlingRights.blackKingside && 
            hasRook(board,Square{7,7},PieceColor::Black)&&
            board.isEmpty(Square{6,7})&&
            board.isEmpty(Square{5,7})&&
            !AttackDetector::isSquareAttacked(board,Square{6,7},enemyColor)&&
            !AttackDetector::isSquareAttacked(board,Square{5,7},enemyColor)           
        ){
            moves.push_back(Move{from,Square{6,7},MoveType::Castle});
        }
        if(castlingRights.blackQueenside && 
            hasRook(board,Square{0,7},PieceColor::Black)&&
            board.isEmpty(Square{1,7})&&
            board.isEmpty(Square{2,7})&&
            board.isEmpty(Square{3,7})&&
            !AttackDetector::isSquareAttacked(board,Square{2,7},enemyColor)&&            
            !AttackDetector::isSquareAttacked(board,Square{3,7},enemyColor)           
        ){
            moves.push_back(Move{from,Square{2,7},MoveType::Castle});
        }
        return;
    }
}