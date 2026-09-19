#include "ChessBoardWidget.h"

#include <QFont>
#include <QGridLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QResizeEvent>
#include <algorithm>

ChessBoardWidget::ChessBoardWidget(QWidget* parent)
    : QWidget(parent)
{
    createBoard();
}

void ChessBoardWidget::createBoard(){

    auto* layout = new QGridLayout(this);

    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    for(int rank=0;rank<BoardSize;++rank){
        for(int file=0;file<BoardSize;++file){
            auto* square = new QPushButton(this);

            square->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            square->setMinimumSize(60,60);
            square->setFocusPolicy(Qt::NoFocus);
            QFont pieceFont = square->font();
            pieceFont.setPointSize(36);
            square->setFont(pieceFont);


            connect(square,&QPushButton::clicked,this,[this,file,rank](){
                handleSquareClick(Square{file,rank});
            });

            const int index = toIndex(file,rank);
            squares_[index] = square;
            const int visualRow = BoardSize -1 -rank;

            layout->addWidget(square,visualRow,file);
        }
    }
}

int ChessBoardWidget::toIndex(int file, int rank){
    return rank* BoardSize + file;
}

void ChessBoardWidget::setPosition(const Board& board,PieceColor sideToMove, std::span<const Move> legalMoves){

    displayedBoard_=board;
    sideToMove_=sideToMove;

    legalMoves_.assign(legalMoves.begin(),legalMoves.end());

    selectedSquare_.reset();

    refreshPieces();
    refreshSquareStyles();
}

QString ChessBoardWidget::symbolFor(const Piece& piece)
{
    if (piece.color == PieceColor::White) {
        switch (piece.type) {
        case PieceType::King:
            return QStringLiteral("♔");

        case PieceType::Queen:
            return QStringLiteral("♕");

        case PieceType::Rook:
            return QStringLiteral("♖");

        case PieceType::Bishop:
            return QStringLiteral("♗");

        case PieceType::Knight:
            return QStringLiteral("♘");

        case PieceType::Pawn:
            return QStringLiteral("♙");
        }
    }

    switch (piece.type) {
    case PieceType::King:
        return QStringLiteral("♚");

    case PieceType::Queen:
        return QStringLiteral("♛");

    case PieceType::Rook:
        return QStringLiteral("♜");

    case PieceType::Bishop:
        return QStringLiteral("♝");

    case PieceType::Knight:
        return QStringLiteral("♞");

    case PieceType::Pawn:
        return QStringLiteral("♟");
    }

    return {};
}

void ChessBoardWidget::handleSquareClick(Square square)
{
    if (!inputEnabled_) {
        return;
    }

    if (!displayedBoard_.has_value()) {
        return;
    }

    const std::vector<Move> requestedMoves =
        legalMovesTo(square);

    // Normal move: exactly one legal move matches.
    if (requestedMoves.size() == 1) {
        selectedSquare_.reset();
        refreshSquareStyles();

        emit moveRequested(requestedMoves.front());
        return;
    }

    // Promotion: four moves share the same origin and destination.
    if (requestedMoves.size() > 1) {
        const std::optional<Move> chosenMove =
            choosePromotionMove(requestedMoves);

        if (chosenMove.has_value()) {
            selectedSquare_.reset();
            refreshSquareStyles();

            emit moveRequested(*chosenMove);
        }

        // If the dialog was cancelled, retain the selected pawn.
        return;
    }

    const std::optional<Piece> piece =
        displayedBoard_->pieceAt(square);

    // Clicking the selected piece again deselects it.
    if (selectedSquare_.has_value() &&
        *selectedSquare_ == square) {
        selectedSquare_.reset();
        refreshSquareStyles();
        return;
    }

    // Select another piece belonging to the side to move.
    if (piece.has_value() &&
        piece->color == sideToMove_) {
        selectedSquare_ = square;
    } else {
        selectedSquare_.reset();
    }

    refreshSquareStyles();
}

void ChessBoardWidget::refreshSquareStyles()
{
    for (int rank = 0; rank < BoardSize; ++rank) {
        for (int file = 0; file < BoardSize; ++file) {
            const Square square{file, rank};

            QPushButton* button =
                squares_[toIndex(file, rank)];

            QColor color;

            if (selectedSquare_.has_value() &&
                *selectedSquare_ == square) {
                color = theme_.selectedSquare;
            } else if (isLegalDestination(square)) {
                color = theme_.legalMoveSquare;
            } else {
                const bool isLightSquare =
                    (file + rank) % 2 != 0;

                color = isLightSquare
                    ? theme_.lightSquare
                    : theme_.darkSquare;
            }

            button->setStyleSheet(
                QStringLiteral(
                    "background-color: %1;"
                    "border: none;"
                ).arg(color.name(QColor::HexRgb))
            );
        }
    }
}

bool ChessBoardWidget::isLegalDestination(Square square)const{
    return !legalMovesTo(square).empty();
}

std::vector<Move> ChessBoardWidget::legalMovesTo(Square destination) const{
    
    std::vector<Move> matches;

    if(!selectedSquare_.has_value()){
        return matches;
    }

    for(const Move& move : legalMoves_){
        if(move.from == *selectedSquare_ && move.to == destination){
            matches.push_back(move);
        }
    }
    return matches;
}

std::optional<Move> ChessBoardWidget::choosePromotionMove(const std::vector<Move>& promotionMoves){

    bool accepted = false;

    const QString selected = QInputDialog::getItem(
        this,
        QStringLiteral("Pawn Promotion"),
        QStringLiteral("Promote pawn to:"),
        QStringList{
            QStringLiteral("Queen"),
            QStringLiteral("Rook"),
            QStringLiteral("Bishop"),
            QStringLiteral("Knight"),
        },0,false,&accepted); 
  
    if (!accepted){
        return std::nullopt;
    }
    
    PieceType selectedType = PieceType::Queen;

    if(selected == QStringLiteral("Rook")){
        selectedType=PieceType::Rook;
    }else if(selected == QStringLiteral("Bishop")){
        selectedType = PieceType::Bishop;
    } else if(selected == QStringLiteral("Knight")){
        selectedType = PieceType::Knight;
    }

    for(const Move& move : promotionMoves){
        if(move.promotionType.has_value()&&
        *move.promotionType==selectedType){
            return move;
        }
    }

    return std::nullopt;

}

void ChessBoardWidget::setInputEnabled(bool enabled){
    inputEnabled_=enabled;

    if(!inputEnabled_){
        selectedSquare_.reset();
        refreshSquareStyles();
    }
}

void ChessBoardWidget::setTheme(const ChessTheme& theme){
    theme_ = theme;

    refreshPieces();
    refreshSquareStyles();
    updateIconSizes();
}

void ChessBoardWidget::refreshPieces(){
    if(!displayedBoard_.has_value()){
        return;
    }

    for(int rank=0;rank<BoardSize;++rank){
        for(int file=0;file<BoardSize;++file){
            const Square square{file,rank};

            QPushButton* button = squares_[toIndex(file,rank)];

            const std::optional<Piece> piece = displayedBoard_->pieceAt(square);

            button->setIcon(QIcon{});
            button->setText({});

            if(!piece.has_value()){
                continue;
            }

            const QIcon& icon = theme_.iconFor(*piece);

            if(!icon.isNull()){
                button->setIcon(icon);
            }else{
                button->setText(symbolFor(*piece));
            }
        }
    }
    updateIconSizes();
}

void ChessBoardWidget::updateIconSizes(){
    for(QPushButton* button : squares_){
        if(button==nullptr){
            continue;
        }

        const int side = static_cast<int>(std::min(button->width(),button->height())*0.82);

        button->setIconSize(QSize(side,side));
    }
}

void ChessBoardWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateIconSizes();
}