#include "Board.h"


std::optional<Piece> Board::pieceAt(Square sq) const{
    if(!isInside(sq)){
        return std::nullopt;
    }

    return squares_[toIndex(sq)];
}


bool Board::isEmpty(Square sq) const{
    return !pieceAt(sq).has_value();
}

bool Board::isInside(Square sq) const {
    return sq.file >= 0 &&
           sq.file < 8 &&
           sq.rank >= 0 &&
           sq.rank < 8;
}


void Board::setPiece(Square sq, std::optional<Piece> piece){
    if(!isInside(sq)){
        return;
    }
    squares_[toIndex(sq)]=piece;
}
 
int Board::toIndex(Square sq) const{
    return sq.rank * 8 +sq.file;
}

void Board::setupStartingPosition() {
    clear();


    setPiece({0, 0}, Piece{PieceType::Rook,   PieceColor::White});
    setPiece({1, 0}, Piece{PieceType::Knight, PieceColor::White});
    setPiece({2, 0}, Piece{PieceType::Bishop, PieceColor::White});
    setPiece({3, 0}, Piece{PieceType::Queen,  PieceColor::White});
    setPiece({4, 0}, Piece{PieceType::King,   PieceColor::White});
    setPiece({5, 0}, Piece{PieceType::Bishop, PieceColor::White});
    setPiece({6, 0}, Piece{PieceType::Knight, PieceColor::White});
    setPiece({7, 0}, Piece{PieceType::Rook,   PieceColor::White});

    for (int file = 0; file < 8; ++file) {
        setPiece({file, 1}, Piece{PieceType::Pawn, PieceColor::White});
    }


    setPiece({0, 7}, Piece{PieceType::Rook,   PieceColor::Black});
    setPiece({1, 7}, Piece{PieceType::Knight, PieceColor::Black});
    setPiece({2, 7}, Piece{PieceType::Bishop, PieceColor::Black});
    setPiece({3, 7}, Piece{PieceType::Queen,  PieceColor::Black});
    setPiece({4, 7}, Piece{PieceType::King,   PieceColor::Black});
    setPiece({5, 7}, Piece{PieceType::Bishop, PieceColor::Black});
    setPiece({6, 7}, Piece{PieceType::Knight, PieceColor::Black});
    setPiece({7, 7}, Piece{PieceType::Rook,   PieceColor::Black});

    for (int file = 0; file < 8; ++file) {
        setPiece({file, 6}, Piece{PieceType::Pawn, PieceColor::Black});
    }
}

void Board::clear(){
    squares_.fill(std::nullopt);
}

bool Board::makeMove(const Move& move) {
    if (!isInside(move.from) || !isInside(move.to)) {
        return false;
    }

    auto movingPiece = pieceAt(move.from);

    if (!movingPiece.has_value()) {
        return false;
    }

    if(move.type == MoveType::Promotion){
        if(!move.promotionType.has_value()){
            return false;
        }
        setPiece(move.to,Piece{move.promotionType.value(),movingPiece->color });
        setPiece(move.from,std::nullopt);
        return true;
    }

    if(move.type == MoveType::Castle){
        if (!movingPiece.has_value()) {
            return false;
        }

        if (movingPiece->type != PieceType::King) {
            return false;
        }

        int rank = move.from.rank;

        Square rookFrom{};
        Square rookTo{};

        if (move.to.file == 6) {
            rookFrom = Square{7, rank};
            rookTo = Square{5, rank};
        }else if(move.to.file == 2){
            rookFrom = Square{0, rank};
            rookTo = Square{3, rank};
        }else{
            return false;
        }
        auto rook = pieceAt(rookFrom);
        if(!rook.has_value()){
            return false;
        }
        if(rook->type != PieceType::Rook || rook->color != movingPiece->color){
            return false;
        }

        setPiece(move.to, movingPiece);
        setPiece(move.from, std::nullopt);

        setPiece(rookTo, rook);
        setPiece(rookFrom, std::nullopt);

        return true;
    }
    
    if (move.type == MoveType::EnPassant) {
        if (movingPiece->type != PieceType::Pawn) {
            return false;
        }

        Square capturedPawnSquare{
            move.to.file,
            move.from.rank
        };

        auto capturedPawn = pieceAt(capturedPawnSquare);

        if (!capturedPawn.has_value()) {
            return false;
        }

        if (
            capturedPawn->type != PieceType::Pawn ||
            capturedPawn->color == movingPiece->color
        ) {
            return false;
        }

        setPiece(move.to, movingPiece);
        setPiece(move.from, std::nullopt);
        setPiece(capturedPawnSquare, std::nullopt);

        return true;
    }
    setPiece(move.to, movingPiece);
    setPiece(move.from, std::nullopt);

    return true;
}