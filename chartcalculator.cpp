#include "chartcalculator.h"
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QTemporaryFile>
#include <QResource>
ChartCalculator::ChartCalculator(QObject *parent)
    : QObject(parent)
{}


void ChartCalculator::setPythonVenvPath(const QString &path) {
#ifdef FLATHUB_BUILD
    // In Flatpak, always use the fixed path
    m_pythonVenvPath = "/app/python/venv";
#else
    // For non-Flatpak builds, use the provided path
    m_pythonVenvPath = path;
#endif
}


ChartData ChartCalculator::parseOutput(const QString &output) {
    ChartData data;
    // Split the output into sections
    QStringList lines = output.split("\n");
    enum Section { None, Planets, Houses, Angles, Aspects };
    Section currentSection = None;

    for (const QString &line : lines) {
        if (line.startsWith("PLANETS:")) {
            currentSection = Planets;
            continue;
        } else if (line.startsWith("HOUSES:")) {
            currentSection = Houses;
            continue;
        } else if (line.startsWith("ANGLES:")) {
            currentSection = Angles;
            continue;
        } else if (line.startsWith("ASPECTS:")) {
            currentSection = Aspects;
            continue;
        }

        if (line.trimmed().isEmpty()) {
            continue;
        }

        switch (currentSection) {
        case Planets: {
            // This regex captures:
            // 1. Planet name (e.g., "Venus")
            // 2. Zodiac sign (e.g., "Pisces")
            // 3. Degree (e.g., "354.63")
            // 4. Optional 'R' (retrograde marker right after degree)
            // 5. House name (e.g., "House8")
            QRegularExpression re("([\\w\\s]+): (\\w+) (\\d+\\.\\d+)°(R?) \\(House (\\w+\\d*)\\)");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                PlanetData planet;
                planet.id = match.captured(1).trimmed();
                planet.sign = match.captured(2);
                planet.longitude = match.captured(3).toDouble();
                QString retrogradeMarker = match.captured(4);
                planet.house = match.captured(5);
                planet.isRetrograde = (retrogradeMarker == "R");
                data.planets.append(planet);
            }
            break;
        }
        case Houses: {
            // Parse line like "House1: Cancer 97.93°"
            QRegularExpression re("(\\w+\\d*): (\\w+) (\\d+\\.\\d+)°");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                HouseData house;
                house.id = match.captured(1);
                house.sign = match.captured(2);
                house.longitude = match.captured(3).toDouble();
                data.houses.append(house);
            }
            break;
        }
        case Angles: {
            // Parse line like "Asc: Cancer 97.93°"
            QRegularExpression re("(\\w+): (\\w+) (\\d+\\.\\d+)°");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                AngleData angle;
                angle.id = match.captured(1);
                angle.sign = match.captured(2);
                angle.longitude = match.captured(3).toDouble();
                data.angles.append(angle);
            }
            break;
        }
        case Aspects: {
            // Parse line like "Sun TRI Moon (Orb: 0.23°)"
            // Using a more specific regex that knows the valid aspect types
            //QRegularExpression re("([\\w\\s]+) (\\w{3}) ([\\w\\s]+) \\(Orb: (\\d+\\.\\d+)°\\)");
            QRegularExpression re("([\\w\\s]+) (CON|OPP|TRI|SQR|SEX|QUI|SSQ|SSX) ([\\w\\s]+) \\(Orb: (\\d+\\.\\d+)°\\)");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                AspectData aspect;
                aspect.planet1 = match.captured(1).trimmed();
                aspect.aspectType = match.captured(2);
                aspect.planet2 = match.captured(3).trimmed();
                aspect.orb = match.captured(4).toDouble();
                data.aspects.append(aspect);
            } else {
            }
            break;
        }
        }
    }

    bool hasNorthNode = false;
    bool hasSouthNode = false;
    for (const PlanetData &planet : data.planets) {
        if (planet.id == "North Node") hasNorthNode = true;
        if (planet.id == "South Node") hasSouthNode = true;
    }

    return data;
}

QString ChartCalculator::getLastError() const
{
    return m_lastError;
}

////////////predictions

QString ChartCalculator::calculateTransits(const QDate &birthDate,
                                           const QTime &birthTime,
                                           const QString &utcOffset,
                                           const QString &latitude,
                                           const QString &longitude,
                                           const QDate &transitStartDate,
                                           int numberOfDays)
{
    if (!isAvailable()) {
        m_lastError = "Python environment is not properly set up";
        return QString();
    }

#ifdef FLATHUB_BUILD
    // In Flatpak, use the installed script
    QString scriptPath = "/app/python/venv/scripts/transit_calculator.py";
#else
    // Set Python executable
    QString pythonPath = QDir(m_pythonVenvPath).filePath("bin/python");
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/transit_calculator-local.py");
#endif

    // Build the command
    QString birthDateStr = birthDate.toString("yyyy/MM/dd");
    QString birthTimeStr = birthTime.toString("HH:mm");
    QString transitStartDateStr = transitStartDate.toString("yyyy/MM/dd");
    //set --orb-max 4.0 to reduce transits to be fed ta ai
    float orbmax = 4.0;

    QStringList args;
    args << scriptPath
         << "--birth-date" << birthDateStr
         << "--birth-time" << birthTimeStr;

    // Handle UTC offset specially to avoid issues with negative values
    if (utcOffset.startsWith("-")) {
        // Use equals sign format for negative offsets
        args << "--birth-utc-offset=" + utcOffset;
    } else {
        // Normal format for positive offsets
        args << "--birth-utc-offset" << utcOffset;
    }

    args << "--latitude" << latitude
         << "--longitude" << longitude
         << "--transit-start-date" << transitStartDateStr
         << "--number-of-days" << QString::number(numberOfDays)
         << "--orb-max" << QString::number(orbmax);

    // Run the command synchronously
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels); // Merge stdout and stderr

#ifdef FLATHUB_BUILD
    process.start("python3", args);
#else
    process.start(pythonPath, args);
#endif

    // Wait for the process to finish
    if (!process.waitForFinished(30000)) { // 30 second timeout for longer calculations
        m_lastError = "Transit calculation process timed out: " + process.errorString();
        return QString();
    }

    // Check for errors
    if (process.exitCode() != 0) {
        m_lastError = "Transit calculation process exited with code " + QString::number(process.exitCode()) +
                      ": " + QString::fromUtf8(process.readAllStandardOutput());
        return QString();
    }

    // Read and return the raw output
    return QString::fromUtf8(process.readAllStandardOutput());
}




ChartData ChartCalculator::calculateChart(const QDate &birthDate,
                                          const QTime &birthTime,
                                          const QString &utcOffset,
                                          const QString &latitude,
                                          const QString &longitude,
                                          const QString &houseSystem,
                                          double orbMax)
{
    ChartData emptyData;
    if (!isAvailable()) {
        m_lastError = "Python environment is not properly set up";
        return emptyData;
    }

#ifdef FLATHUB_BUILD
    // In Flatpak, use the installed script
    QString scriptPath = "/app/python/venv/scripts/chart_calculator.py";
#else
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/chart_calculator-local.py");
    // Set Python executable
    QString pythonPath = QDir(m_pythonVenvPath).filePath("bin/python");
#endif

    // Build the command
    QString dateStr = birthDate.toString("yyyy/MM/dd");
    QString timeStr = birthTime.toString("HH:mm");
    QStringList args;
    args << scriptPath
         << "--date" << dateStr
         << "--time" << timeStr;

    // Handle UTC offset specially to avoid issues with negative values
    if (utcOffset.startsWith("-")) {
        // Use equals sign format for negative offsets
        args << "--utc-offset=" + utcOffset;
    } else {
        // Normal format for positive offsets
        args << "--utc-offset" << utcOffset;
    }

    args << "--latitude" << latitude
         << "--longitude" << longitude
         << "--house-system" << houseSystem
         << "--orb-max" << QString::number(orbMax);


    // Run the command synchronously
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels); // Merge stdout and stderr

#ifdef FLATHUB_BUILD
    process.start("python3", args);
#else
    process.start(pythonPath, args);
#endif

    // Wait for the process to finish
    if (!process.waitForFinished(10000)) { // 10 second timeout
        m_lastError = "Process timed out: " + process.errorString();
        return emptyData;
    }

    // Check for errors
    if (process.exitCode() != 0) {
        m_lastError = "Process exited with code " + QString::number(process.exitCode()) +
                      ": " + QString::fromUtf8(process.readAllStandardOutput());
        return emptyData;
    }

    // Read the output
    QString output = QString::fromUtf8(process.readAllStandardOutput());

    // Parse the output
    return parseOutput(output);
}



bool ChartCalculator::isAvailable() const
{
#ifdef FLATHUB_BUILD
    // In Flatpak, check if the scripts exist in the filesystem
    QString scriptPath = "/app/python/venv/scripts/chart_calculator.py";
    if (!QFile::exists(scriptPath)) {
        return false;
    }
    return true;
#else
    if (m_pythonVenvPath.isEmpty()) {
        return false;
    }
    // Check if Python interpreter exists
    QString pythonPath = QDir(m_pythonVenvPath).filePath("bin/python");
    if (!QFile::exists(pythonPath)) {
        return false;
    }
    // Check if the script exists
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/chart_calculator-local.py");
    if (!QFile::exists(scriptPath)) {
        return false;
    }
    return true;
#endif
}
