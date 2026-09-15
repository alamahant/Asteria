#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include<QSettings>
#include <QFontDatabase>
#include <QString>
#include"Globals.h"
#include<QDir>
#include<QPalette>
#include<QStyleFactory>

namespace {
double g_orbMax = 8.0; // Default orb value
}

double getOrbMax() {
    return g_orbMax;
}

void setOrbMax(double value) {
    g_orbMax = value;
}

QString g_astroFontFamily;


int main(int argc, char *argv[])
{

    QDir().mkpath(AsteriaFlags::sharesDirPath);

    QApplication a(argc, argv);

#ifndef FLATHUB_BUILD

    a.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, Qt::white);
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    a.setPalette(lightPalette);
#endif

#ifdef Q_OS_WIN
    a.setStyleSheet("QLineEdit { placeholder-text-color: #999999; }");
#endif

    int fontId = QFontDatabase::addApplicationFont(":/resources/AstromoonySans.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load Astromoony font";
    } else {
        g_astroFontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
    }


#ifdef FLATHUB_BUILD
    QCoreApplication::setOrganizationName("");

#else
    QCoreApplication::setOrganizationName("Alamahant");
#endif

    QCoreApplication::setApplicationName("Asteria");
    QDir().mkpath(AsteriaFlags::appDir);
    QCoreApplication::setApplicationVersion("2.4.9");

    QSettings settings;
    // Display settings
    AsteriaFlags::chartSize      = settings.value("display/chartSize",      AsteriaFlags::chartSize).toInt();
    AsteriaFlags::wheelThickness = settings.value("display/wheelThickness", AsteriaFlags::wheelThickness).toInt();
    AsteriaFlags::planetSize     = settings.value("display/planetSize",     AsteriaFlags::planetSize).toInt();
    AsteriaFlags::pointSize      = settings.value("display/pointSize",      AsteriaFlags::pointSize).toInt();
    AsteriaFlags::uiFontSize     = settings.value("display/uiFontSize",     AsteriaFlags::uiFontSize).toInt();


    MainWindow w;
    w.show();
    return a.exec();
}
