#pragma once

#include "models/Board.h"
#include "models/Square.h"

#include <QWidget>
#include <QString>
#include <array>
#include <optional>
#include <span>
#include <vector>
#include <QInputDialog>
#include <QStringList>


class QPushButton;

class ChessBoardWidget : public QWidget{
    Q_OBJECT

    public:
        explicit ChessBoardWidget(QWidget* parent = nullptr);
        void setPosition(const Board& board,PieceColor sideToMove, std::span<const Move> legalMoves);
        void setInputEnabled(bool enabled);
    
    signals:
        void moveRequested(Move move);
    private:
        static constexpr int BoardSize =8;
        bool inputEnabled_ = true;
        std::array<QPushButton*, BoardSize*BoardSize> squares_{};
        std::optional<Square> selectedSquare_;
        std::optional<Board> displayedBoard_;
        PieceColor sideToMove_ = PieceColor::White;
        std::vector<Move> legalMoves_;

        void createBoard();
        void handleSquareClick(Square square);
        void refreshSquareStyles();
        
        [[nodiscard]]
        std::vector<Move> legalMovesTo(Square destination) const;

        [[nodiscard]]
        std::optional<Move> choosePromotionMove(const std::vector<Move>& promotionMoves);

        [[nodiscard]]
        bool isLegalDestination(Square square) const;
        
        static int toIndex(int file, int rank);
        static QString symbolFor(const Piece& piece);

};