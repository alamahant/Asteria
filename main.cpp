#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include<QSettings>
#include <QFontDatabase>
#include <QString>
QString g_astroFontFamily;


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Load custom astronomical font
    int fontId = QFontDatabase::addApplicationFont(":/resources/AstromoonySans.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load Astromoony font";
    } else {
        g_astroFontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        qDebug() << "Loaded astronomical font:" << g_astroFontFamily;
    }

    QCoreApplication::setOrganizationName("Asteria");  // Instead of "Your Organization"
    QCoreApplication::setApplicationName("Asteria");
    MainWindow w;
    w.show();
    return a.exec();
}
