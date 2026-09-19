#pragma once

#include "ChessTheme.h"

#include <QString>
#include <QStringList>

#include <optional>

namespace ThemeLoader{

    [[nodiscard]]
    std::optional<ChessTheme> load(const QString& themeDirectory, QString* errorMessage = nullptr);

    [[nodiscard]]
    QStringList discover(const QString& themesRoot);
}