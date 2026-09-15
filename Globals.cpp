#include"Globals.h"
#include<QStandardPaths>
#include<QApplication>

namespace AsteriaFlags {
bool additionalBodiesEnabled = false;
QString lastGeneratedChartType = "Natal Birth";
QString appDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Asteria";
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
    const int uiFontSizeDefault     = 14;
#endif

    int chartSize      = chartSizeDefault;
    int wheelThickness = wheelThicknessDefault;
    int planetSize     = planetSizeDefault;
    int pointSize      = pointSizeDefault;
    int uiFontSize     = uiFontSizeDefault;
}
