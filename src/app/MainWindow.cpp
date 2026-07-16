#include "MainWindow.h"

#include "ChessBoardWidget.h"
#include "logic/MoveGenerator.h"
#include "notation/San.h"
#include "notation/Pgn.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <QAction>
#include <QByteArray>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSaveFile>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent):
    QMainWindow(parent),
    game_{},
    chessBoard_(new ChessBoardWidget(this)),
    statusLabel_(new QLabel(this)),
    moveHistoryTable_(new QTableWidget(0,3,this))
{

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    auto* boardLayout = new QVBoxLayout();

    mainLayout->setContentsMargins(8,8,8,8);
    mainLayout->setSpacing(10);

    boardLayout->setSpacing(8);

    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setStyleSheet("font-size: 18px;"
    "font-weight: bold;");

    boardLayout->addWidget(statusLabel_);
    boardLayout->addWidget(chessBoard_,1);

    mainLayout->addLayout(boardLayout,1);
    mainLayout->addWidget(moveHistoryTable_);
    
    moveHistoryTable_->setHorizontalHeaderLabels(QStringList{
        QStringLiteral("#"),
        QStringLiteral("White"),
        QStringLiteral("Black")
    });

    moveHistoryTable_->verticalHeader()->setVisible(false);
    moveHistoryTable_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    moveHistoryTable_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    moveHistoryTable_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
    moveHistoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    moveHistoryTable_->setSelectionMode(QAbstractItemView::NoSelection);
    moveHistoryTable_->setFocusPolicy(Qt::NoFocus);
    moveHistoryTable_->setMinimumWidth(260);

    setCentralWidget(centralWidget);

    setWindowTitle("C++ Chess");
    resize(1000,760);

    QMenu* gameMenu = menuBar()->addMenu(
        QStringLiteral("&Game")
    );

    QAction* newGameAction = gameMenu->addAction(
        QStringLiteral("&New Game")
    );

    newGameAction->setShortcut(QKeySequence::New);

    connect(newGameAction,&QAction::triggered,this,&MainWindow::resetGame);

    gameMenu->addSeparator();

    QAction* savePgnAction = gameMenu->addAction(
        QStringLiteral("&Save PGN")
    );

    savePgnAction->setShortcut(QKeySequence::Save);

    connect(savePgnAction,&QAction::triggered,this,&MainWindow::savePgn);

    connect(
        chessBoard_,&ChessBoardWidget::moveRequested,this,[this](Move move){
            const GameState& stateBeforeMove = game_.state();

            const std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(
                stateBeforeMove.board,
                stateBeforeMove.sideToMove,
                stateBeforeMove.castlingRights,
                stateBeforeMove.enPassantTarget
            );

            const std::string san = toSan(stateBeforeMove,move,legalMoves);

            if(game_.makeMove(move)){
                appendSanMove(QString::fromStdString(san));
                refreshBoard();
            }
        }
    );

    refreshBoard();
}

void MainWindow::refreshBoard(){
    const GameState& state = game_.state();

    const std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(
        state.board,
        state.sideToMove,
        state.castlingRights,
        state.enPassantTarget
    );

    chessBoard_->setPosition(state.board,state.sideToMove,legalMoves);

    refreshStatus();
}

void MainWindow::refreshStatus(){
    const PieceColor sideToMove = game_.sideToMove();
    const GameStatus status = game_.status();

    switch(status){
        case GameStatus::Ongoing:
            if(sideToMove==PieceColor::White){
                statusLabel_->setText("White to move");
            }else {
                statusLabel_->setText("Black to move");
            }

            chessBoard_->setEnabled(true);
            break;
        case GameStatus::Check:
            if(sideToMove==PieceColor::White){
                statusLabel_->setText("White to move - check");
            }else{
                statusLabel_->setText("Black to move - check");
            }
            chessBoard_->setEnabled(true);
            break;
        case GameStatus::Checkmate:
            if (sideToMove == PieceColor::White) {
                statusLabel_->setText("Checkmate — Black wins");
            }else{
            statusLabel_->setText("Checkmate — White wins");
            }
            chessBoard_->setEnabled(false);
            break;

        case GameStatus::Stalemate:
            statusLabel_->setText("Stalemate — Draw");
            chessBoard_->setEnabled(false);
            break;
    }
}

void MainWindow::appendSanMove(const QString& san){
    const std::size_t plyCount = game_.moveHistory().size();
    if(plyCount == 0){
        return;
    }

    const int plyIndex = static_cast<int>(plyCount -1);

    const int row = plyIndex /2 ;

    const bool wasWhiteMove = plyIndex % 2 == 0;

    if(wasWhiteMove){
        moveHistoryTable_->insertRow(row);

        auto* moveNumberItem = new QTableWidgetItem(QString::number(row+1));

        moveHistoryTable_->setItem(row,0,moveNumberItem);
    }

    int column;

    if(wasWhiteMove){
        column=1;
    }else{
        column=2;
    }

    auto* sanItem = new QTableWidgetItem(san);

    sanItem->setTextAlignment(Qt::AlignCenter);

    moveHistoryTable_->setItem(row,column,sanItem);

    moveHistoryTable_->scrollToBottom();
}

void MainWindow::resetGame(){
    game_=Game{};

    moveHistoryTable_->setRowCount(0);

    chessBoard_->setInputEnabled(true);

    refreshBoard();
}

void MainWindow::savePgn(){
    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save PGN"),
        QStringLiteral("game.pgn"),
        QStringLiteral("PGN Files (*.pgn);;All Files (*)")
    );

    if(fileName.isEmpty()){
        return;
    }

    if(!fileName.endsWith(QStringLiteral(".pgn"),Qt::CaseInsensitive)){
        fileName+= QStringLiteral(".pgn");
    }

    PgnMetadata metadata;

    const GameState& state = game_.state();
    const GameStatus status = game_.status();
    
    
    switch (status) {
    case GameStatus::Checkmate:
        if (state.sideToMove == PieceColor::White) {
            // White is checkmated, so Black won.
            metadata.result = PgnResult::BlackWin;
        } else {
            // Black is checkmated, so White won.
            metadata.result = PgnResult::WhiteWin;
        }
        break;

    case GameStatus::Stalemate:
        metadata.result = PgnResult::Draw;
        break;

    case GameStatus::Ongoing:
    case GameStatus::Check:
        metadata.result = PgnResult::Ongoing;
        break;
    }

    const std::string pgn = toPgn(game_,metadata);
    const QByteArray pgnData=QByteArray::fromStdString(pgn);

    QSaveFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::critical(this,QStringLiteral("Save PGN Failed"),QStringLiteral("Could not open the selected file for writing"));
        return;
    }

    const qint64 bytesWritten = file.write(pgnData);

    if(bytesWritten != pgnData.size()){
        file.cancelWriting();

        QMessageBox::critical(this,QStringLiteral("Save PGN Failed"),QStringLiteral("The complet PGN could not be written"));
        return;
    }

    if(!file.commit()){
        QMessageBox::critical(this,QStringLiteral("Save PGN Failed"),QStringLiteral("The PGN file could not be committed to disk"));
        return;
    }

    statusBar()->showMessage(QStringLiteral("PGN saved successfully"),3000);
}