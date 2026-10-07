#include"Globals.h"
#include<QStandardPaths>
#include<QApplication>

namespace AsteriaFlags {
bool additionalBodiesEnabled = false;
QString lastGeneratedChartType = "Natal Birth";

const QString appDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Asteria";
const QString synastryDir = appDir + "/SynastryCharts";
bool activeModelLoaded = false;
QString sharesDirPath = appDir + "/shares";

#ifdef Q_OS_WIN
    const int chartSizeDefault      = 560;
    const int wheelThicknessDefault = 24;
    const int planetSizeDefault     = 28;
    const int pointSizeDefault      = 13;
    const int uiFontSizeDefault     = 11;
#else
    const int chartSizeDefault      = 700;
    const int wheelThicknessDefault = 30;
    const int planetSizeDefault     = 35;
    const int pointSizeDefault      = 16;
    const int uiFontSizeDefault     = 12;
#endif

    int chartSize      = chartSizeDefault;
    int wheelThickness = wheelThicknessDefault;
    int planetSize     = planetSizeDefault;
    int pointSize      = pointSizeDefault;
    int uiFontSize     = uiFontSizeDefault;

    bool tarotOverlayEnabled = false;
    int tarotCardHeight = tarotCardDefaultHeight;
    const int tarotCardDefaultHeight = 210;

#ifdef Q_OS_WIN
    const qreal DEFAULTFONTSIZE = 11;
#else
    const qreal DEFAULTFONTSIZE = 12;
#endif
qreal FONTSIZE = DEFAULTFONTSIZE;


}
