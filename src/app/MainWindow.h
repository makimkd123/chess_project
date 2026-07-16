#pragma once

#include "logic/Game.h"

#include <QMainWindow>

class ChessBoardWidget;
class QLabel;
class QTableWidget;
class QString;

class MainWindow : public QMainWindow{
    public:
        explicit MainWindow(QWidget* parent = nullptr);
    private:
        Game game_;

        ChessBoardWidget* chessBoard_ = nullptr;
        QLabel* statusLabel_ = nullptr;
        QTableWidget* moveHistoryTable_=nullptr;


        void refreshBoard();
        void refreshStatus();
        void appendSanMove(const QString& san);

        void resetGame();

        void savePgn();
};