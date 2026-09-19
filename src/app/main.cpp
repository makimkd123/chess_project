#include "app/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("makimkd123"));

    QCoreApplication::setApplicationName(QStringLiteral("CPP_CHESS"));
    MainWindow window;
    window.show();

    return application.exec();
}