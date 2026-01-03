#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

#include "MainWindow.h"
int main(int argc, char *argv[])
{
    // Enable High DPI Support
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
