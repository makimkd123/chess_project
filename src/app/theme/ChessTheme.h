#pragma once

#include "models/Pieces.h"

#include <QColor>
#include <QIcon>
#include <QString>

#include <array>
#include <cstddef>

struct ChessTheme{
    static constexpr std::size_t PieceIconCount = 12;

    QString id = QStringLiteral("built-in");
    QString name = QStringLiteral("Built-in");

    QColor lightSquare{240,217,181};
    QColor darkSquare{181,136,99};
    QColor selectedSquare{246,246,105};
    QColor legalMoveSquare{169,209,142};

    std::array<QIcon, PieceIconCount> pieceIcons{};

    [[nodiscard]]
    const QIcon& iconFor(const Piece& piece)const;

    [[nodiscard]]
    static ChessTheme builtIn();
};