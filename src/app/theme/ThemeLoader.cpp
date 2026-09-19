#include "ThemeLoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <array>

namespace {

void setError(QString* destination, const QString& message){
    if(destination !=nullptr){
        *destination = message;
    }
}

bool readColor(const QJsonObject& object,const QString& key, QColor& destination, QString* errorMessage){
    if(!object.contains(key)){
        return true;
    }
    const QColor color(object.value(key).toString());

    if(!color.isValid()){
        setError(errorMessage,QStringLiteral("Invalid color for '%1'").arg(key));
        return false;
    }
    destination = color;
    return true;
}

QString findPieceImage(const QDir& piecesDirectory,const QString& baseName){
    const QString pgnPath=piecesDirectory.filePath(baseName+QStringLiteral(".png"));

    if(QFileInfo::exists(pgnPath)){
        return pgnPath;
    }

    const QString svgPath=piecesDirectory.filePath(baseName+QStringLiteral(".svg"));
    if(QFileInfo::exists(svgPath)){ 
        return svgPath;
    }
    return{};
}    
}//namespace

namespace ThemeLoader{

std::optional<ChessTheme>load(const QString& themeDirectory,QString* errorMessage){
    const QDir directory(themeDirectory);

    if(!directory.exists()){
        setError(errorMessage,QStringLiteral("Theme directory does not exist: %1").arg(themeDirectory));
        return std::nullopt;
    }

    QFile file(directory.filePath(QStringLiteral("theme.json")));

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        setError(errorMessage,QStringLiteral("Could not open theme.json in %1").arg(themeDirectory));
        return std::nullopt;
    }

    QJsonParseError parseError;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(),&parseError);

    if(parseError.error !=QJsonParseError::NoError){
        setError(errorMessage,QStringLiteral("Invalid theme JSON: %1").arg(parseError.errorString()));
        return std::nullopt;
    }

    if(!document.isObject()){
        setError(errorMessage,QStringLiteral("Theme JSON must contain an object"));
        return std::nullopt;
    }

    const QJsonObject object = document.object();

    ChessTheme theme = ChessTheme::builtIn();

    theme.id = directory.dirName();
    theme.name = object.value(QStringLiteral("name")).toString(theme.id);

    if(!readColor(object,QStringLiteral("lightSquare"),theme.lightSquare,errorMessage) ||
    !readColor(object,QStringLiteral("darkSquare"),theme.darkSquare,errorMessage) ||
    !readColor(object,QStringLiteral("selectedSquare"),theme.selectedSquare,errorMessage) ||
    !readColor(object,QStringLiteral("legalMoveSquare"),theme.legalMoveSquare,errorMessage)){
        return std::nullopt;
    }

    const QString piecesDirectoryName = object.value(QStringLiteral("piecesDirectory")).toString(QStringLiteral("pieces"));
    
    const QDir piecesDirectory(directory.filePath(piecesDirectoryName));

    constexpr std::array<const char*, 6> pieceNames{
        "pawn",
        "knight",
        "bishop",
        "rook",
        "queen",
        "king"
    };

    constexpr std::array<const char*, 2> colorNames{
        "white",
        "black"
    };
    for (std::size_t color = 0; color < colorNames.size(); ++color) {
        for (std::size_t type = 0; type < pieceNames.size(); ++type) {
            const QString baseName =
                QStringLiteral("%1_%2")
                    .arg(QString::fromLatin1(colorNames[color]))
                    .arg(QString::fromLatin1(pieceNames[type]));

            const QString imagePath =
                findPieceImage(piecesDirectory, baseName);

            if (!imagePath.isEmpty()) {
                const std::size_t index = color * 6 + type;
                theme.pieceIcons[index] = QIcon(imagePath);
            }
        }
    }

    return theme;
}

QStringList discover(const QString& themesRoot){
    const QDir root(themesRoot);

    QStringList results;

    if (!root.exists()) {
        return results;
    }

    const QStringList directoryNames = root.entryList(
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name
    );

    for (const QString& directoryName : directoryNames) {
        const QString themePath =
            root.filePath(directoryName);

        if (QFileInfo::exists(
                QDir(themePath).filePath(
                    QStringLiteral("theme.json")
                )
            )) {
            results.push_back(themePath);
        }
    }

    return results;
}

}//namespace themeloader
