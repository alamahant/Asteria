#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include<QSettings>
#include <QFontDatabase>
#include <QString>
// At the top of your main.cpp file, after includes
namespace {
double g_orbMax = 8.0; // Default orb value
}

// Add a getter/setter function
double getOrbMax() {
    return g_orbMax;
}

void setOrbMax(double value) {
    g_orbMax = value;
}

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
    }

    QCoreApplication::setOrganizationName("");  // Instead of "Your Organization"
    QCoreApplication::setApplicationName("Asteria");
    MainWindow w;
    w.show();
    return a.exec();
}
