#pragma once

#include "logic/Game.h"
#include "notation/Pgn.h"

#include <QMainWindow>

#include <cstddef>

class ChessBoardWidget;
class QLabel;
class QTableWidget;
class QString;

class MainWindow : public QMainWindow{
    public:
        explicit MainWindow(QWidget* parent = nullptr);
    private:
        Game game_;
        PgnMetadata pgnMetadata_;

        ChessBoardWidget* chessBoard_ = nullptr;
        QLabel* statusLabel_ = nullptr;
        QTableWidget* moveHistoryTable_=nullptr;


        void refreshBoard();
        void refreshStatus();
        void appendSanMove(const QString& san,std::size_t plyIndex);
        void rebuildMoveHistoryTable();
        void resetGame();

        void savePgn();
        void loadPgn();
        void createThemeMenu();
};