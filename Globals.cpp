#include"Globals.h"
#include<QStandardPaths>
#include<QApplication>

namespace GlobalFlags {
bool additionalBodiesEnabled = false;
QString lastGeneratedChartType = "Natal Birth";
QString appDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Asteria";

}
