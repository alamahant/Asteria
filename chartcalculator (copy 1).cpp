

#include "chartcalculator.h"
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>

ChartCalculator::ChartCalculator(QObject *parent)
    : QObject(parent)
{}

void ChartCalculator::setPythonVenvPath(const QString &path) {
#ifdef FLATHUB_BUILD
    // In Flathub, check for the environment variable first
    QString envPath = qgetenv("ASTERIA_PYTHON_VENV");
    if (!envPath.isEmpty()) {
        m_pythonVenvPath = envPath;
        qDebug() << "Using Python venv path from environment:" << m_pythonVenvPath;
    } else {
        // Fall back to the default path
        m_pythonVenvPath = FLATHUB_DEFAULT_PYTHON_PATH;
        qDebug() << "Using default Python venv path:" << m_pythonVenvPath;
    }
#else
    m_pythonVenvPath = path;
#endif
}

bool ChartCalculator::isAvailable() const
{
#ifdef FLATHUB_BUILD
    // In Flathub, we only need to check if the script exists
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/chart_calculator.py");
    if (!QFile::exists(scriptPath)) {
        qDebug() << "Script not found at:" << scriptPath;
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
        qDebug() << "Python interpreter not found at:" << pythonPath;
        return false;
    }
    // Check if the script exists
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/chart_calculator-local.py");
    if (!QFile::exists(scriptPath)) {
        qDebug() << "Script not found at:" << scriptPath;
        return false;
    }
    return true;
#endif
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

    // Set script path based on build type
#ifdef FLATHUB_BUILD
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/chart_calculator.py");
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

    qDebug() << "CHARTCALCULATOR USED THESE ARGS " << args;

    // Run the command synchronously
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels); // Merge stdout and stderr
#ifdef FLATHUB_BUILD
    process.start("python3", args);
#else
    qDebug() << "Running command:" << pythonPath << args.join(" ");
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
    qDebug() << "OUTPUT FROM CHART_CALCULATOR" << output;
    return parseOutput(output);
}

ChartData ChartCalculator::parseOutput(const QString &output) {
    ChartData data;
    // Split the output into sections
    QStringList lines = output.split("\n");
    enum Section { None, Planets, Houses, Angles, Aspects };
    Section currentSection = None;
    qDebug() << "Starting to parse chart data";

    for (const QString &line : lines) {
        if (line.startsWith("PLANETS:")) {
            currentSection = Planets;
            qDebug() << "Entering PLANETS section";
            continue;
        } else if (line.startsWith("HOUSES:")) {
            currentSection = Houses;
            qDebug() << "Entering HOUSES section";
            continue;
        } else if (line.startsWith("ANGLES:")) {
            currentSection = Angles;
            qDebug() << "Entering ANGLES section";
            continue;
        } else if (line.startsWith("ASPECTS:")) {
            currentSection = Aspects;
            qDebug() << "Entering ASPECTS section";
            continue;
        }

        if (line.trimmed().isEmpty()) {
            continue;
        }

        switch (currentSection) {
        case Planets: {
            //qDebug() << "Parsing planet line:" << line;
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
                //qDebug() << "  Retrograde marker:" << retrogradeMarker << "isRetrograde:" << planet.isRetrograde;
                qDebug() << "  Parsed planet:" << planet.id << planet.sign << planet.longitude
                         << planet.house << (planet.isRetrograde ? "Retrograde" : "Direct");
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
                //qDebug() << "Parsed house:" << house.id << house.sign << house.longitude;
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
                //qDebug() << "Parsed angle:" << angle.id << angle.sign << angle.longitude;
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
                qDebug() << "Failed to parse aspect line:" << line;
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
    qDebug() << "North Node included:" << hasNorthNode;
    qDebug() << "South Node included:" << hasSouthNode;

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
                                           int numberOfDays) {
    if (!isAvailable()) {
        m_lastError = "Python environment is not properly set up";
        return QString();
    }

// Set script path based on build type
#ifdef FLATHUB_BUILD
    QString scriptPath = QDir(m_pythonVenvPath).filePath("scripts/transit_calculator.py");
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
    qDebug() << "Running transit command:" << pythonPath << args.join(" ");
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



