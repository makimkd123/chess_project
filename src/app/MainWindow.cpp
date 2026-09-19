#include "MainWindow.h"

#include "ChessBoardWidget.h"
#include "logic/MoveGenerator.h"
#include "notation/San.h"
#include "theme/ThemeLoader.h"

#include <QActionGroup>
#include <QCoreApplication>
#include <QDebug>
#include <QSettings>

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
#include <QFile>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSaveFile>
#include <QStatusBar>
#include <QTimer>


#include <exception>
#include <string>
#include <utility>
#include <vector>


MainWindow::MainWindow(QWidget* parent):
    QMainWindow(parent),
    game_{},
    chessBoard_(new ChessBoardWidget(this)),
    statusLabel_(new QLabel(this)),
    moveHistoryTable_(new QTableWidget(0,3,this))
{
    statusBar()->hide();
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
    
    createThemeMenu();

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

    gameMenu->addSeparator();

    QAction* openPgnAction = gameMenu->addAction(
        QStringLiteral("&Open PGN")
    );

    openPgnAction->setShortcut(QKeySequence::Open);

    connect(openPgnAction,&QAction::triggered,this,&MainWindow::loadPgn);

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
                const std::size_t plyIndex = game_.moveHistory().size() - 1;
                appendSanMove(QString::fromStdString(san),plyIndex);
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

void MainWindow::refreshStatus()
{
    const PieceColor sideToMove = game_.sideToMove();
    const GameStatus status = game_.status();

    switch (status) {
    case GameStatus::Ongoing:
        if (sideToMove == PieceColor::White) {
            statusLabel_->setText("White to move");
        } else {
            statusLabel_->setText("Black to move");
        }
        break;

    case GameStatus::Check:
        if (sideToMove == PieceColor::White) {
            statusLabel_->setText("White to move - check");
        } else {
            statusLabel_->setText("Black to move - check");
        }
        break;

    case GameStatus::Checkmate:
        if (sideToMove == PieceColor::White) {
            statusLabel_->setText("Checkmate — Black wins");
        } else {
            statusLabel_->setText("Checkmate — White wins");
        }
        break;

    case GameStatus::Stalemate:
        statusLabel_->setText("Stalemate — Draw");
        break;
    }

    const bool gameAcceptsInput =
        status == GameStatus::Ongoing ||
        status == GameStatus::Check;

    chessBoard_->setInputEnabled(gameAcceptsInput);
}

void MainWindow::appendSanMove(const QString& san,std::size_t plyIndex){

    const int index =static_cast<int>(plyIndex);

    const int row = index /2 ;

    const bool wasWhiteMove = index % 2 == 0;

    if(wasWhiteMove){
        moveHistoryTable_->insertRow(row);

        auto* moveNumberItem = new QTableWidgetItem(QString::number(row+1));

        moveNumberItem->setTextAlignment(Qt::AlignCenter);

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
    pgnMetadata_=PgnMetadata{};

    moveHistoryTable_->setRowCount(0);

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

    pgnMetadata_.result = PgnResult::Ongoing;
    const GameStatus status = game_.status();

    switch (status) {
    case GameStatus::Checkmate:
        if (game_.sideToMove() == PieceColor::White) {
            pgnMetadata_.result =
                PgnResult::BlackWin;
        } else {
            pgnMetadata_.result =
                PgnResult::WhiteWin;
        }
        break;

    case GameStatus::Stalemate:
        pgnMetadata_.result =
            PgnResult::Draw;
        break;

    case GameStatus::Ongoing:
    case GameStatus::Check:
        pgnMetadata_.result =
            PgnResult::Ongoing;
        break;
    }

    const std::string pgn = toPgn(game_,pgnMetadata_);
    const QByteArray pgnData=QByteArray::fromStdString(pgn);

    QSaveFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::critical(this,QStringLiteral("Save PGN Failed"),QStringLiteral("Could not open the selected file for writing"));
        return;
    }

    const qint64 bytesWritten = file.write(pgnData);

    if(bytesWritten != pgnData.size()){
        file.cancelWriting();

        QMessageBox::critical(this,QStringLiteral("Save PGN Failed"),QStringLiteral("The complete PGN could not be written"));
        return;
    }

    if(!file.commit()){
        QMessageBox::critical(this,QStringLiteral("Save PGN Failed"),QStringLiteral("The PGN file could not be committed to disk"));
        return;
    }

    QStatusBar* bar = statusBar();

    bar->show();
    bar->showMessage(QStringLiteral("PGN saved successfully"));

    QTimer::singleShot(3000,bar,&QStatusBar::hide);
}

void MainWindow::loadPgn(){
    const QString fileName=QFileDialog::getOpenFileName(this,QStringLiteral("Open PGN"),QString{},QStringLiteral("PGN Files (*.pgn);;All Files (*)"));

    if(fileName.isEmpty()){
        return;
    }

    QFile file(fileName);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::critical(this,QStringLiteral("Open PGN Failed"),QStringLiteral("Could not open the selected PGN file"));
        return;
    }

    const QByteArray fileData = file.readAll();
    const std::string pgnText = fileData.toStdString();

    Result<ImportedPgn> imported = fromPgn(pgnText);

    if(!imported.success){
        QMessageBox::critical(this,QStringLiteral("Invalid PGN"),QString::fromStdString(imported.message));
        return;
    }

    game_=std::move(imported.value.game);

    pgnMetadata_=std::move(imported.value.metadata);

    rebuildMoveHistoryTable();
    refreshBoard();
    QStatusBar* bar = statusBar();

    bar->show();
    bar->showMessage(QStringLiteral("PGN loaded successfully"));

    QTimer::singleShot(3000,bar,&QStatusBar::hide);
}

void MainWindow::rebuildMoveHistoryTable(){
    moveHistoryTable_->setRowCount(0);

    GameState replayState=GameState::startingPosition();

    const std::vector<Move>& history = game_.moveHistory();

    for(std::size_t i=0; i<history.size();++i){
        const Move& move = history[i];
        
        const std::vector<Move> legalMoves = MoveGenerator::generateLegalMoves(
            replayState.board,
            replayState.sideToMove,
            replayState.castlingRights,
            replayState.enPassantTarget
        );

        const std::string san = toSan(replayState,move,legalMoves);
        appendSanMove(QString::fromStdString(san),i);
        if(!replayState.applyMove(move)){
            QMessageBox::critical(this,QStringLiteral("PGN History Error"),QStringLiteral("The loaded move history could not be replayed"));
            moveHistoryTable_->setRowCount(0);
            return;
        }
    }
}

void MainWindow::createThemeMenu(){
    QMenu* appearanceMenu = menuBar()->addMenu(QStringLiteral("&Appearance"));
    QMenu* themeMenu = appearanceMenu->addMenu(QStringLiteral("&Theme"));
    auto* actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);

    const QString themesRoot=QCoreApplication::applicationDirPath()+QStringLiteral("/themes");

    const QStringList themeDirectories = ThemeLoader::discover(themesRoot);

    QSettings settings;

    const QString savedThemeId = settings.value(QStringLiteral("appearance/theme"),QStringLiteral("classic")).toString();

    QAction* firstThemeAction = nullptr;

    bool restoredSavedTheme = false;

    for (const QString& themeDirectory : themeDirectories){
        QString errorMessage;

        const std::optional<ChessTheme> loadedTheme = ThemeLoader::load(themeDirectory,&errorMessage);
        if(!loadedTheme.has_value()){
            qWarning().noquote()<<"Theme could not be loaded:"<< errorMessage;
            continue;
        }

        const ChessTheme theme= *loadedTheme;

        QAction* action = themeMenu->addAction(theme.name);

        action->setCheckable(true);
        actionGroup->addAction(action);

        if(firstThemeAction==nullptr){
            firstThemeAction=action;
        }

        connect(action, &QAction::triggered, this, [this, theme]() {
            chessBoard_->setTheme(theme);

            QSettings settings;
            settings.setValue(QStringLiteral("appearance/theme"),theme.id);
        });

        if(theme.id==savedThemeId){
            action->setChecked(true);
            chessBoard_->setTheme(theme);
            restoredSavedTheme=true;
        }
    }

    if(!restoredSavedTheme && firstThemeAction!=nullptr){
        firstThemeAction->setChecked(true);
        firstThemeAction->trigger();
    }

    if(firstThemeAction==nullptr){
        QAction* unavailableAction = themeMenu->addAction(QStringLiteral("No themes found"));
        unavailableAction->setEnabled(false);
    }

}