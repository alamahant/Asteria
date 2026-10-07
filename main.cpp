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
#include<QByteArray>
#include<QFont>

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

#ifdef FLATHUB_BUILD
    QCoreApplication::setOrganizationName("");

#else
    QCoreApplication::setOrganizationName("Alamahant");
#endif

    QCoreApplication::setApplicationName("Asteria");
    QCoreApplication::setApplicationVersion("2.5.1");

#ifdef Q_OS_WIN
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    QDir().mkpath(AsteriaFlags::appDir);
    QDir().mkpath(AsteriaFlags::sharesDirPath);
    QDir().mkpath(AsteriaFlags::synastryDir);

    QSettings settings;

    double factor = settings.value("ui/scaleFactor", 1.0).toDouble();
    qputenv("QT_SCALE_FACTOR", QByteArray::number(factor));
    AsteriaFlags::FONTSIZE = settings.value("ui/fontSize", AsteriaFlags::DEFAULTFONTSIZE).toReal();

    QApplication a(argc, argv);

#ifdef Q_OS_WIN

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
    a.setStyleSheet("QLineEdit { placeholder-text-color: #999999; }");
#endif

    int fontId = QFontDatabase::addApplicationFont(":/resources/AstromoonySans.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load Astromoony font";
    } else {
        g_astroFontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
    }



    if (AsteriaFlags::FONTSIZE > 0.0) {
            QFont appFont = a.font();
            appFont.setPointSizeF(AsteriaFlags::FONTSIZE);
            a.setFont(appFont);
        } else {
            AsteriaFlags::FONTSIZE = AsteriaFlags::DEFAULTFONTSIZE;
        }


    AsteriaFlags::chartSize      = settings.value("display/chartSize",      AsteriaFlags::chartSize).toInt();
    AsteriaFlags::wheelThickness = settings.value("display/wheelThickness", AsteriaFlags::wheelThickness).toInt();
    AsteriaFlags::planetSize     = settings.value("display/planetSize",     AsteriaFlags::planetSize).toInt();
    AsteriaFlags::pointSize      = settings.value("display/pointSize",      AsteriaFlags::pointSize).toInt();
    AsteriaFlags::uiFontSize     = settings.value("ui/fontSize",     AsteriaFlags::uiFontSize).toInt();


    MainWindow w;
    w.show();
    return a.exec();
}
