#include "ChessTheme.h"

namespace{

std::size_t pieceIndex(const Piece& piece){

    const std::size_t colorOffset = piece.color == PieceColor::White ? 0 : 6;
    return colorOffset+static_cast<std::size_t>(piece.type);
}

}//namespace


const QIcon& ChessTheme::iconFor(const Piece& piece)const{

    return pieceIcons[pieceIndex(piece)];
}

ChessTheme ChessTheme::builtIn(){
    return ChessTheme{};
}