#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include<QSettings>
#include <QFontDatabase>
#include <QString>
#include"Globals.h"
#include<QDir>

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

    //QCoreApplication::setOrganizationName("Alamahant");

#ifdef FLATHUB_BUILD
    QCoreApplication::setOrganizationName("");

#else
    QCoreApplication::setOrganizationName("Alamahant");

#endif

    QCoreApplication::setApplicationName("Asteria");
    QDir().mkpath(GlobalFlags::appDir);
    QCoreApplication::setApplicationVersion("2.4.6");



    MainWindow w;
    w.show();
    return a.exec();
}
