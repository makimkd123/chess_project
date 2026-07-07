#include "Fen.h"
#include <cctype>
#include <sstream>
#include <string>



namespace{
    char pieceToFenChar(Piece piece){
        char c = '?';

        switch(piece.type){
            case PieceType::King:
            c='k';
            break;
            case PieceType::Queen:
            c='q';
            break;
            case PieceType::Rook:
            c='r';
            break;
            case PieceType::Bishop:
            c='b';
            break;
            case PieceType::Knight:
            c='n';
            break;
            case PieceType::Pawn:
            c='p';
            break;            
        }
        
        if(piece.color==PieceColor::White){
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return c;
    }

std::string castlingRightsToFen(const CastlingRights& rights){
    std::string result;

    if(rights.whiteKingside){
        result += 'K';
    }
    if(rights.whiteQueenside){
        result += 'Q';
    }
    if(rights.blackKingside){
        result += 'k';
    }
    if(rights.blackQueenside){
        result += 'q';
    }
    if(result.empty()){
        result = "-";
    }
    return result;
}

std::string squareToFen(const Square& square){
    std::string result;

    result += static_cast<char>('a'+square.file);
    result += static_cast<char>('1'+square.rank);

    return result;
}
} // end of namespace

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