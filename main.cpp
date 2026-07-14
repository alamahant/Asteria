#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QSettings>
#include <QFontDatabase>
#include <QString>
#include <QTranslator>
#include <QDir>
#include <QPalette>
#include <QStyleFactory>
#include "Globals.h"

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

namespace {

bool loadTranslator(QTranslator &translator, const QString &languageCode)
{
    QString normalizedCode = languageCode.toLower();
    if (normalizedCode.startsWith("es")) {
        normalizedCode = "es";
    } else if (normalizedCode.startsWith("en")) {
        normalizedCode = "en";
    } else {
        normalizedCode = "en";
    }

    QStringList searchDirs;
    const QString appDir = QCoreApplication::applicationDirPath();
    searchDirs << (appDir + "/translations")
               << (QDir::currentPath() + "/translations")
               << (appDir + "/../translations")
               << (appDir + "/../share/Asteria/translations");

    for (const QString &dir : searchDirs) {
        if (translator.load(QString("asteria_%1.qm").arg(normalizedCode), dir)) {
            return true;
        }
    }

    return false;
}

}

int main(int argc, char *argv[])
{

    QDir().mkpath(GlobalFlags::sharesDirPath);

    QApplication a(argc, argv);

    // Set organization/application name BEFORE any QSettings access so that
    // saved preferences (e.g. app/language) are read from the right location.
#ifdef FLATHUB_BUILD
    QCoreApplication::setOrganizationName("");
#else
    QCoreApplication::setOrganizationName("Alamahant");
#endif
    QCoreApplication::setApplicationName("Asteria");
    QDir().mkpath(GlobalFlags::appDir);
    QCoreApplication::setApplicationVersion("2.4.7");

    QString selectedLanguage = "en";
    for (int i = 1; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == "--lang" && i + 1 < argc) {
            selectedLanguage = QString::fromLocal8Bit(argv[i + 1]);
            break;
        }
    }

    QSettings settings;
    if (settings.contains("app/language")) {
        selectedLanguage = settings.value("app/language").toString();
    }

    QTranslator translator;
    if (loadTranslator(translator, selectedLanguage)) {
        a.installTranslator(&translator);
    }

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

    // Load custom astronomical font
    int fontId = QFontDatabase::addApplicationFont(":/resources/AstromoonySans.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load Astromoony font";
    } else {
        g_astroFontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
    }

    MainWindow w;
    w.show();
    return a.exec();
}
