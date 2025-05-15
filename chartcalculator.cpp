#include "chartcalculator.h"
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QTemporaryFile>
#include <QResource>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimeZone>
#include <cmath>
#include"Globals.h"
// Include Swiss Ephemeris headers
extern "C" {
#include "swephexp.h"
#include "swehouse.h"
}

// Swiss Ephemeris planet constants
#define SE_SUN          0
#define SE_MOON         1
#define SE_MERCURY      2
#define SE_VENUS        3
#define SE_MARS         4
#define SE_JUPITER      5
#define SE_SATURN       6
#define SE_URANUS       7
#define SE_NEPTUNE      8
#define SE_PLUTO        9
#define SE_MEAN_NODE    10  // Mean Lunar Node
#define SE_TRUE_NODE    11  // True Lunar Node
#define SE_CHIRON       15

ChartCalculator::ChartCalculator(QObject *parent)
    : QObject(parent), m_isInitialized(false)
{
    initialize();
}

ChartCalculator::~ChartCalculator()
{
    // Close Swiss Ephemeris
    if (m_isInitialized) {
        swe_close();
    }
}

/*
bool ChartCalculator::initialize()
{
    // Possible locations for ephemeris files
    QStringList searchPaths = {
        // Build directory
        QCoreApplication::applicationDirPath() + "/ephemeris",
        // AppImage structure
        QCoreApplication::applicationDirPath() + "/../share/Asteria/ephemeris",
        // Flatpak structure
        "/app/share/Asteria/ephemeris",
        // Standard locations
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ephemeris",
    };

    // Find first valid path
    for (const QString& path : searchPaths) {
        if (path.isEmpty()) continue;

        QDir dir(path);
        if (dir.exists() && (dir.exists("sepl_18.se1") || dir.exists("sepl_20.se1"))) {
            m_ephemerisPath = path;

            // Set ephemeris path for Swiss Ephemeris
            QByteArray pathBytes = m_ephemerisPath.toLocal8Bit();
            swe_set_ephe_path(pathBytes.constData());

            m_isInitialized = true;
            return true;
        }
    }

    qWarning() << "Ephemeris files not found in any standard location!";
    m_lastError = "Ephemeris files not found. Please check installation.";
    return false;
}

*/


bool ChartCalculator::initialize()
{
    // Possible locations for ephemeris files
    QStringList searchPaths;

#ifdef FLATHUB_BUILD
        // For Flatpak builds, the ephemeris files are in /app/share/swisseph
    searchPaths << "/app/share/swisseph";
#else
        // Standard locations for non-Flatpak builds
    searchPaths << QCoreApplication::applicationDirPath() + "/ephemeris"
                << QCoreApplication::applicationDirPath() + "/../share/Asteria/ephemeris"
                << "/app/share/Asteria/ephemeris"
                << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ephemeris";
#endif

    // Find first valid path
    for (const QString& path : searchPaths) {
        if (path.isEmpty()) continue;

        QDir dir(path);
        if (dir.exists()) {
            // Check for existence of key ephemeris files
            if (dir.exists("sepl_18.se1") || dir.exists("sepl_20.se1") ||
                dir.exists("seas_18.se1") || dir.exists("semo_18.se1")) {

                m_ephemerisPath = path;

                // Set ephemeris path for Swiss Ephemeris
                QByteArray pathBytes = m_ephemerisPath.toLocal8Bit();
                swe_set_ephe_path(pathBytes.constData());

                // Debug output to verify files are accessible
                QStringList files = dir.entryList(QDir::Files, QDir::Name);
                for (int i = 0; i < qMin(5, files.size()); i++) {
                }
                if (files.size() > 5) {
                }

                m_isInitialized = true;
                return true;
            }
        }
    }

    // If we get here, no valid path was found
    qWarning() << "Ephemeris files not found in any standard location!";
    qWarning() << "Searched paths:";
    for (const QString& path : searchPaths) {
        qWarning() << "  " << path;
    }

    m_lastError = "Ephemeris files not found. Please check installation.";
    return false;
}




QString ChartCalculator::getLastError() const {
    return m_lastError;
}

bool ChartCalculator::isAvailable() const {

    return true;
}




double ChartCalculator::dateTimeToJulianDay(const QDateTime &dateTime, const QString &utcOffset) const {
    // Parse UTC offset
    bool negative = utcOffset.startsWith('-');
    QString offsetStr = utcOffset;
    if (negative || offsetStr.startsWith('+')) {
        offsetStr = offsetStr.mid(1);
    }
    QStringList parts = offsetStr.split(':');
    int offsetHours = parts[0].toInt();
    int offsetMinutes = parts.size() > 1 ? parts[1].toInt() : 0;
    double offset = offsetHours + (offsetMinutes / 60.0);
    if (negative) offset = -offset;

    // Get date and time components
    int year = dateTime.date().year();
    int month = dateTime.date().month();
    int day = dateTime.date().day();
    int hour = dateTime.time().hour();
    int minute = dateTime.time().minute();
    int second = dateTime.time().second();

    // Convert time to decimal hours
    double hours = hour + (minute / 60.0) + (second / 3600.0);

    // Adjust for UTC offset (subtract offset to get UTC time)
    hours -= offset;

    // Handle day boundary changes if needed
    while (hours < 0) {
        hours += 24;
        day--;
        if (day < 1) {
            month--;
            if (month < 1) {
                month = 12;
                year--;
            }
            // Get days in the new month
            QDate tempDate(year, month, 1);
            day = tempDate.daysInMonth();
        }
    }

    while (hours >= 24) {
        hours -= 24;
        day++;
        QDate tempDate(year, month, 1);
        if (day > tempDate.daysInMonth()) {
            day = 1;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }

    // Calculate Julian day
    double jd = swe_julday(year, month, day, hours, SE_GREG_CAL);
    return jd;
}




QString ChartCalculator::getZodiacSign(double longitude) const {
    // Normalize longitude to 0-360
    longitude = fmod(longitude, 360.0);
    if (longitude < 0) longitude += 360.0;

    // Calculate sign index (0-11)
    int signIndex = static_cast<int>(longitude / 30.0);

    // Return sign name
    static const QString signs[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
        "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"
    };

    return signs[signIndex];
}

QString ChartCalculator::findHouse(double longitude, const QVector<HouseData> &houses) const {
    // Normalize longitude to 0-360
    longitude = fmod(longitude, 360.0);
    if (longitude < 0) longitude += 360.0;

    // Find house for this longitude
    for (int i = 0; i < houses.size(); i++) {
        int nextHouse = (i + 1) % houses.size();
        double start = houses[i].longitude;
        double end = houses[nextHouse].longitude;

        // Handle case where house spans 0°
        if (end < start) {
            if (longitude >= start || longitude < end) {
                return houses[i].id;
            }
        } else {
            if (longitude >= start && longitude < end) {
                return houses[i].id;
            }
        }
    }

    // Default if not found (shouldn't happen)
    return "House1";
}

QVector<AspectData> ChartCalculator::calculateAspects(const QVector<PlanetData> &planets, double orbMax) const {
    QVector<AspectData> aspects;

    // Define aspect types and their angles
    struct AspectType {
        QString name;
        double angle;
        double orb;
    };
    orbMax= getOrbMax();
    const AspectType aspectTypes[] = {
        {"CON", 0.0, orbMax},      // Conjunction
        {"OPP", 180.0, orbMax},    // Opposition
        {"TRI", 120.0, orbMax},    // Trine
        {"SQR", 90.0, orbMax},     // Square
        {"SEX", 60.0, orbMax},     // Sextile
        {"QUI", 150.0, orbMax * 0.75},  // Quintile
        {"SSQ", 45.0, orbMax * 0.75},  // Semi-square
        {"SSX", 30.0, orbMax * 0.75}   // Semi-sextile
        //{"SSP", 0.0, orbMax * 0.5},     // Semiparallel (custom) - typically for declination
        //{"PAR", 0.0, orbMax * 0.5}      // Parallel (custom) - typically for declination
    };

    // Calculate aspects between all planets
    for (int i = 0; i < planets.size(); i++) {
        for (int j = i + 1; j < planets.size(); j++) {
            double angle1 = planets[i].longitude;
            double angle2 = planets[j].longitude;

            // Calculate the smallest angle between the two planets
            double diff = fabs(angle1 - angle2);
            if (diff > 180.0) diff = 360.0 - diff;

            // Check each aspect type
            for (const AspectType &aspectType : aspectTypes) {
                double orb = fabs(diff - aspectType.angle);
                if (orb <= aspectType.orb) {
                    AspectData aspect;
                    aspect.planet1 = planets[i].id;
                    aspect.planet2 = planets[j].id;
                    aspect.aspectType = aspectType.name;
                    aspect.orb = orb;
                    aspects.append(aspect);
                    break;  // Only add the closest matching aspect
                }
            }
        }
    }

    return aspects;
}



ChartData ChartCalculator::calculateChart(const QDate &birthDate,
                                          const QTime &birthTime,
                                          const QString &utcOffset,
                                          const QString &latitude,
                                          const QString &longitude,
                                          const QString &houseSystem,
                                          double orbMax) {
    ChartData data;
    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    // Convert input to required format
    QDateTime birthDateTime(birthDate, birthTime);
    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    // Convert to Julian day
    double jd = dateTimeToJulianDay(birthDateTime, utcOffset);

    // Calculate house cusps
    QVector<HouseData> houses = calculateHouseCusps(jd, lat, lon, houseSystem);
    data.houses = houses;

    // Calculate angles (Asc, MC, etc.)
    data.angles = calculateAngles(jd, lat, lon, houseSystem);
    // Calculate planet positions
    data.planets = calculatePlanetPositions(jd, houses);

    ////////// Add Syzygy and Pars Fortuna and other methods
    addSyzygyAndParsFortuna(data.planets, jd, houses, data.angles);
    calculateAdditionalBodies(data.planets, jd, houses);
    ///////////

    // Calculate aspects
    orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);
    return data;
}




QVector<HouseData> ChartCalculator::calculateHouseCusps(double jd, double lat, double lon, const QString &houseSystem) const {
    QVector<HouseData> houses;

    // Convert house system string to char
    char hsys = 'P'; // Placidus by default
    if (houseSystem == "Placidus") hsys = 'P';
    else if (houseSystem == "Koch") hsys = 'K';
    else if (houseSystem == "Porphyrius") hsys = 'O';
    else if (houseSystem == "Regiomontanus") hsys = 'R';
    else if (houseSystem == "Campanus") hsys = 'C';
    else if (houseSystem == "Equal") hsys = 'E';
    else if (houseSystem == "Whole Sign") hsys = 'W';



    // Arrays to hold house cusps and ascmc values
    double cusps[13] = {0};
    double ascmc[10] = {0};

    // Calculate house cusps
    int result = swe_houses(jd, lat, lon, hsys, cusps, ascmc);

    if (result < 0) {
        qWarning() << "Error calculating house cusps";
        return houses;
    }

    // Process house cusps
    for (int i = 1; i <= 12; i++) {
        HouseData house;
        house.id = QString("House%1").arg(i);
        house.longitude = cusps[i];
        house.sign = getZodiacSign(house.longitude);
        houses.append(house);
    }

    return houses;
}

QVector<AngleData> ChartCalculator::calculateAngles(double jd, double lat, double lon, const QString &houseSystem) const {
    QVector<AngleData> angles;

    // Convert house system string to char (same as in calculateHouseCusps)
    char hsys = 'P'; // Placidus by default
    if (houseSystem == "Placidus") hsys = 'P';
    else if (houseSystem == "Koch") hsys = 'K';
    else if (houseSystem == "Porphyrius") hsys = 'O';
    else if (houseSystem == "Regiomontanus") hsys = 'R';
    else if (houseSystem == "Campanus") hsys = 'C';
    else if (houseSystem == "Equal") hsys = 'E';
    else if (houseSystem == "Whole Sign") hsys = 'W';


    // Arrays to hold house cusps and ascmc values
    double cusps[13] = {0};
    double ascmc[10] = {0};

    // Calculate house cusps with the specified house system
    int result = swe_houses(jd, lat, lon, hsys, cusps, ascmc);
    if (result < 0) {
        qWarning() << "Error calculating angles";
        return angles;
    }

    // Add Ascendant
    AngleData asc;
    asc.id = "Asc";
    asc.longitude = ascmc[0];
    asc.sign = getZodiacSign(asc.longitude);
    angles.append(asc);

    // Add Midheaven
    AngleData mc;
    mc.id = "MC";
    mc.longitude = ascmc[1];
    mc.sign = getZodiacSign(mc.longitude);
    angles.append(mc);

    // Add Descendant
    AngleData desc;
    desc.id = "Desc";
    desc.longitude = fmod(ascmc[0] + 180.0, 360.0);
    desc.sign = getZodiacSign(desc.longitude);
    angles.append(desc);

    // Add Imum Coeli
    AngleData ic;
    ic.id = "IC";
    ic.longitude = fmod(ascmc[1] + 180.0, 360.0);
    ic.sign = getZodiacSign(ic.longitude);
    angles.append(ic);

    return angles;
}




QVector<PlanetData> ChartCalculator::calculatePlanetPositions(double jd, const QVector<HouseData> &houses) const {
    QVector<PlanetData> planets;
    // Define the planets to calculate
    int planetIds[] = {
        SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS,
        SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO,
        SE_TRUE_NODE, SE_CHIRON
    };

    QString planetNames[] = {
        "Sun", "Moon", "Mercury", "Venus", "Mars",
        "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto",
        "North Node", "Chiron"
    };

    int numPlanets = sizeof(planetIds) / sizeof(planetIds[0]);

    // Calculate each planet
    for (int i = 0; i < numPlanets; i++) {
        double xx[6]; // Position and speed
        char serr[256];

        // Calculate planet position
        int flag = SEFLG_SPEED | SEFLG_SWIEPH;
        int ret = swe_calc_ut(jd, planetIds[i], flag, xx, serr);

        if (ret < 0) {
            qWarning() << "Error calculating position for planet" << planetNames[i] << ":" << serr;
            continue;
        }

        PlanetData planet;
        planet.id = planetNames[i];
        planet.longitude = xx[0]; // Longitude in degrees
        planet.latitude = xx[1];  // Latitude in degrees
        planet.sign = getZodiacSign(planet.longitude);
        planet.isRetrograde = (xx[3] < 0); // Retrograde if speed is negative
        planet.house = findHouse(planet.longitude, houses);

        planets.append(planet);
    }

    // Add South Node (opposite to North Node)
    for (const PlanetData &planet : planets) {
        if (planet.id == "North Node") {
            PlanetData southNode;
            southNode.id = "South Node";
            southNode.longitude = fmod(planet.longitude + 180.0, 360.0);
            southNode.latitude = -planet.latitude;
            southNode.sign = getZodiacSign(southNode.longitude);
            southNode.isRetrograde = planet.isRetrograde;
            southNode.house = findHouse(southNode.longitude, houses);
            planets.append(southNode);
            break;
        }
    }

    return planets;
}



// New methods for special calculations

ChartData ChartCalculator::calculateSolarReturn(const QDate &birthDate,
                                                const QTime &birthTime,
                                                const QString &utcOffset,
                                                const QString &latitude,
                                                const QString &longitude,
                                                int year) {
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    // Convert birth data to Julian day
    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    // Get Sun's position at birth
    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_SUN, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Sun position: %1").arg(serr);
        return data;
    }

    double sunLongitude = xx[0];

    // Estimate solar return time (around birthday in the target year)
    QDate approxDate(year, birthDate.month(), birthDate.day());
    if (!approxDate.isValid()) {
        // Handle Feb 29 for non-leap years
        approxDate = QDate(year, birthDate.month(), birthDate.daysInMonth());
    }

    QDateTime approxDateTime(approxDate, birthTime);
    double approxJd = dateTimeToJulianDay(approxDateTime, utcOffset);

    // Find exact solar return (when Sun returns to the same longitude)
    double srJd = findPlanetaryEvent(SE_SUN, approxJd, sunLongitude);

    if (srJd <= 0) {
        m_lastError = "Could not find solar return";
        return data;
    }

    // Calculate chart for the solar return time
    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    // Calculate house cusps
    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;

    // Calculate angles
    data.angles = calculateAngles(srJd, lat, lon, houseSystem);

    // Calculate planet positions
    data.planets = calculatePlanetPositions(srJd, houses);

    // Calculate aspects
    data.aspects = calculateAspects(data.planets, 8.0);

    return data;
}

ChartData ChartCalculator::calculateSaturnReturn(const QDate &birthDate,
                                                 const QTime &birthTime,
                                                 const QString &utcOffset,
                                                 const QString &latitude,
                                                 const QString &longitude,
                                                 int returnNumber) {
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    // Convert birth data to Julian day
    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);
    // Get Saturn's position at birth
    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_SATURN, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Saturn position: %1").arg(serr);
        return data;
    }

    double saturnLongitude = xx[0];

    // Estimate Saturn return time
    // Saturn takes about 29.5 years for one orbit
    double approxJd = birthJd + (returnNumber * 29.5 * 365.25);

    // Find exact Saturn return (when Saturn returns to the same longitude)
    double srJd = findPlanetaryEvent(SE_SATURN, approxJd, saturnLongitude);

    if (srJd <= 0) {
        m_lastError = "Could not find Saturn return";
        return data;
    }

    // Calculate chart for the Saturn return time
    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    // Calculate house cusps
    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;

    // Calculate angles
    data.angles = calculateAngles(srJd, lat, lon, houseSystem);

    // Calculate planet positions
    data.planets = calculatePlanetPositions(srJd, houses);

    // Calculate aspects
    data.aspects = calculateAspects(data.planets, 8.0);

    return data;
}

double ChartCalculator::findPlanetaryEvent(int planet, double startJd, double targetLongitude) const {
    // Search window (days)
    const double searchWindow = 60.0;

    // Search step (days)
    const double step = 1.0;

    // Tolerance (degrees)
    const double tolerance = 0.0001;

    double jd = startJd - searchWindow / 2;
    double endJd = startJd + searchWindow / 2;

    double bestJd = 0;
    double bestDiff = 360.0;

    // First pass: find approximate time with daily steps
    while (jd <= endJd) {
        double xx[6];
        char serr[256];
        int flag = SEFLG_SWIEPH;
        int ret = swe_calc_ut(jd, planet, flag, xx, serr);

        if (ret < 0) {
            qWarning() << "Error in findPlanetaryEvent:" << serr;
            return 0;
        }

        double longitude = xx[0];
        double diff = fabs(longitude - targetLongitude);
        if (diff > 180.0) diff = 360.0 - diff;

        if (diff < bestDiff) {
            bestDiff = diff;
            bestJd = jd;
        }

        jd += step;
    }

    if (bestDiff > 10.0) {
        // If we're not close, try a wider search
        return findPlanetaryEvent(planet, startJd - searchWindow, targetLongitude);
    }

    // Second pass: refine with binary search
    double lowerJd = bestJd - step;
    double upperJd = bestJd + step;

    for (int i = 0; i < 20; i++) {  // 20 iterations should be enough for precision
        double midJd = (lowerJd + upperJd) / 2;

        double xx[6];
        char serr[256];
        int flag = SEFLG_SWIEPH;
        int ret = swe_calc_ut(midJd, planet, flag, xx, serr);

        if (ret < 0) {
            qWarning() << "Error in findPlanetaryEvent refinement:" << serr;
            return 0;
        }

        double longitude = xx[0];
        double diff = longitude - targetLongitude;

        // Normalize difference to -180 to +180
        if (diff > 180.0) diff -= 360.0;
        if (diff < -180.0) diff += 360.0;

        if (fabs(diff) < tolerance) {
            return midJd;  // Found with required precision
        }

        if (diff < 0) {
            lowerJd = midJd;
        } else {
            upperJd = midJd;
        }
    }

    // Return best approximation
    return (lowerJd + upperJd) / 2;
}



QString ChartCalculator::calculateTransits(const QDate &birthDate,
                                           const QTime &birthTime,
                                           const QString &utcOffset,
                                           const QString &latitude,
                                           const QString &longitude,
                                           const QDate &transitStartDate,
                                           int numberOfDays) {
    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return QString();
    }

    // Convert birth data to Julian day
    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    // Calculate natal positions
    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    // Calculate natal house cusps
    QVector<HouseData> natalHouses = calculateHouseCusps(birthJd, lat, lon, houseSystem);

    // Calculate natal angles
    QVector<AngleData> natalAngles = calculateAngles(birthJd, lat, lon, houseSystem);

    // Calculate natal planet positions
    QVector<PlanetData> natalPlanets = calculatePlanetPositions(birthJd, natalHouses);

    // Add Syzygy and Pars Fortuna to natal planets
    addSyzygyAndParsFortuna(natalPlanets, birthJd, natalHouses, natalAngles);

    // Convert transit start date to Julian day
    QDateTime transitStartDateTime(transitStartDate, QTime(0, 0));
    double transitStartJd = dateTimeToJulianDay(transitStartDateTime, "+0:00");

    // Define planets to check for transits (exclude certain objects)
    QStringList excludedTransitingObjects = {
        "Pars Fortuna", "North Node", "South Node", "Chiron", "Syzygy"
    };

    // Define target objects for natal chart
    QStringList includedTargetObjects = {
        "Sun", "Moon", "Mercury", "Venus", "Mars",
        "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"
    };

    // Define aspects to check with their codes
    struct AspectType {
        QString code;
        double angle;
        double orb;
    };

    double orbMax = getOrbMax();
    const AspectType aspectTypes[] = {
        {"CON", 0.0, orbMax},
        {"OPP", 180.0, orbMax},
        {"TRI", 120.0, orbMax},
        {"SQR", 90.0, orbMax},
        {"SEX", 60.0, orbMax},
        {"QUI", 150.0, orbMax * 0.75},
        {"SSQ", 45.0, orbMax * 0.75},
        {"SSX", 30.0, orbMax * 0.75}
    };

    int numAspectTypes = sizeof(aspectTypes) / sizeof(aspectTypes[0]);

    // Build transit report
    QString report;
    report += "---TRANSITS---\n";

    // Check each day
    for (int day = 0; day < numberOfDays; day++) {
        double transitJd = transitStartJd + day;
        QDateTime transitDateTime = julianDayToDateTime(transitJd);

        // Format date as YYYY/MM/DD to match target format
        QString dateStr = transitDateTime.toString("yyyy/MM/dd");

        // Calculate transit house cusps
        QVector<HouseData> transitHouses = calculateHouseCusps(transitJd, lat, lon, houseSystem);

        // Calculate transit angles
        QVector<AngleData> transitAngles = calculateAngles(transitJd, lat, lon, houseSystem);

        // Calculate transit planet positions
        QVector<PlanetData> transitPlanets = calculatePlanetPositions(transitJd, transitHouses);

        // Add Syzygy and Pars Fortuna to transit planets
        addSyzygyAndParsFortuna(transitPlanets, transitJd, transitHouses, transitAngles);

        // Store aspects for this day
        QStringList dayAspects;

        // Check aspects between transit and natal planets
        for (const PlanetData &transitPlanet : transitPlanets) {
            // Skip excluded transiting objects
            if (excludedTransitingObjects.contains(transitPlanet.id)) {
                continue;
            }

            for (const PlanetData &natalPlanet : natalPlanets) {
                // Only include target objects from natal chart
                if (!includedTargetObjects.contains(natalPlanet.id)) {
                    continue;
                }

                // Calculate the smallest angle between the two planets
                double diff = fabs(transitPlanet.longitude - natalPlanet.longitude);
                if (diff > 180.0) diff = 360.0 - diff;

                // Check each aspect type
                for (int j = 0; j < numAspectTypes; j++) {
                    double orb = fabs(diff - aspectTypes[j].angle);
                    if (orb <= aspectTypes[j].orb) {
                        // Format aspect string to match the expected format for parsing
                        QString transitPlanetName = transitPlanet.id;
                        if (transitPlanet.isRetrograde) {
                            transitPlanetName += " (R)";
                        }

                        QString aspectStr = QString("%1 %2 %3( %4°)")
                                                .arg(transitPlanetName)
                                                .arg(aspectTypes[j].code)
                                                .arg(natalPlanet.id)
                                                .arg(orb, 0, 'f', 2);

                        dayAspects.append(aspectStr);
                        break;  // Only report the closest aspect
                    }
                }
            }
        }

        // Format output for parsing
        report += dateStr + ": " + dayAspects.join(", ") + "\n";
    }
    return report;
}


QDateTime ChartCalculator::julianDayToDateTime(double jd, const QString &utcOffset) const {
    int year, month, day, hour, minute, second;
    double hour_fraction;

    // Convert Julian day to calendar date and time (UTC)
    swe_revjul(jd, SE_GREG_CAL, &year, &month, &day, &hour_fraction);

    // Convert hour fraction to hour, minute, second
    hour = static_cast<int>(hour_fraction);
    minute = static_cast<int>((hour_fraction - hour) * 60);
    second = static_cast<int>(((hour_fraction - hour) * 60 - minute) * 60);

    // Create QDateTime in UTC
    QDate date(year, month, day);
    QTime time(hour, minute, second);
    QDateTime utcDateTime(date, time, QTimeZone::UTC);

    // Parse UTC offset to convert back to local time
    bool negative = utcOffset.startsWith('-');
    QString offsetStr = utcOffset;
    if (negative || offsetStr.startsWith('+')) {
        offsetStr = offsetStr.mid(1);
    }
    QStringList parts = offsetStr.split(':');
    int offsetHours = parts[0].toInt();
    int offsetMinutes = parts.size() > 1 ? parts[1].toInt() : 0;
    int offsetSeconds = (offsetHours * 60 + offsetMinutes) * 60;
    if (negative) offsetSeconds = -offsetSeconds;

    // Convert UTC to local time
    QDateTime localDateTime = utcDateTime.addSecs(offsetSeconds);

    return localDateTime;
}


QVector<EclipseData> ChartCalculator::findEclipses(const QDate &startDate,
                                                   const QDate &endDate,
                                                   bool solarEclipses,
                                                   bool lunarEclipses) {
    QVector<EclipseData> eclipses;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return eclipses;
    }

    // Convert dates to Julian days
    QDateTime startDateTime(startDate, QTime(0, 0));
    QDateTime endDateTime(endDate, QTime(23, 59, 59));
    double startJd = dateTimeToJulianDay(startDateTime, "+0:00");
    double endJd = dateTimeToJulianDay(endDateTime, "+0:00");

    char serr[256] = {0};

    // Process solar eclipses
    if (solarEclipses) {
        double tjd = startJd;
        while (tjd < endJd) {
            double tret[10] = {0}; // Array to receive eclipse times

            // Find next global solar eclipse
            int32 iflgret = swe_sol_eclipse_when_glob(tjd, SEFLG_SWIEPH, 0, tret, 0, serr);

            if (iflgret < 0) {
                m_lastError = QString("Error finding solar eclipse: %1").arg(serr);
                break;
            }

            // tret[0] contains the time of maximum eclipse
            tjd = tret[0];

            // If we found an eclipse within our range
            if (tjd <= endJd) {
                EclipseData eclipse;
                eclipse.date = julianDayToDateTime(tjd).date();
                eclipse.time = julianDayToDateTime(tjd).time();
                eclipse.julianDay = tjd;

                // Determine eclipse type
                if (iflgret & SE_ECL_TOTAL) {
                    eclipse.type = "Total Solar Eclipse";
                } else if (iflgret & SE_ECL_ANNULAR) {
                    eclipse.type = "Annular Solar Eclipse";
                } else if (iflgret & SE_ECL_PARTIAL) {
                    eclipse.type = "Partial Solar Eclipse";
                } else if (iflgret & SE_ECL_ANNULAR_TOTAL) {
                    eclipse.type = "Hybrid Solar Eclipse";
                } else {
                    eclipse.type = "Solar Eclipse";
                }

                // Get eclipse details
                double attr[20] = {0};
                // Corrected function call with 5 arguments
                swe_sol_eclipse_where(tjd, SEFLG_SWIEPH, attr, 0, serr);

                // Eclipse magnitude
                eclipse.magnitude = attr[0];

                // Geographic coordinates of maximum eclipse
                eclipse.latitude = attr[2];
                eclipse.longitude = attr[3];

                eclipses.append(eclipse);

                // Move to the next day to find the next eclipse
                tjd += 10; // Add 10 days to avoid finding the same eclipse
            } else {
                break; // Eclipse is beyond our end date
            }
        }
    }

    // Process lunar eclipses
    if (lunarEclipses) {
        double tjd = startJd;
        while (tjd < endJd) {
            // For lunar eclipses, we'll use a different approach
            // We'll find the next full moon and check if it's an eclipse

            // First, find the next full moon
            double tret[10] = {0};
            double fullMoonJd = tjd;

            // Calculate sun and moon positions to find opposition (full moon)
            bool foundFullMoon = false;
            for (int i = 0; i < 30 && fullMoonJd < endJd; i++) {
                fullMoonJd = tjd + i;

                double sunPos[6], moonPos[6];
                if (swe_calc_ut(fullMoonJd, SE_SUN, SEFLG_SWIEPH, sunPos, serr) < 0 ||
                    swe_calc_ut(fullMoonJd, SE_MOON, SEFLG_SWIEPH, moonPos, serr) < 0) {
                    m_lastError = QString("Error calculating positions: %1").arg(serr);
                    break;
                }

                // Calculate angle between sun and moon
                double angle = fabs(moonPos[0] - sunPos[0]);
                if (angle > 180) angle = 360 - angle;

                // If close to 180 degrees (opposition), we found a full moon
                if (fabs(angle - 180) < 1.0) {
                    foundFullMoon = true;
                    break;
                }
            }

            if (!foundFullMoon) {
                // If we couldn't find a full moon, move forward and try again
                tjd += 10;
                continue;
            }

            // Now check if this full moon is a lunar eclipse
            // We'll use the node position to determine this
            double nodePos[6];
            if (swe_calc_ut(fullMoonJd, SE_TRUE_NODE, SEFLG_SWIEPH, nodePos, serr) < 0) {
                m_lastError = QString("Error calculating node position: %1").arg(serr);
                break;
            }

            double moonPos[6];
            if (swe_calc_ut(fullMoonJd, SE_MOON, SEFLG_SWIEPH, moonPos, serr) < 0) {
                m_lastError = QString("Error calculating moon position: %1").arg(serr);
                break;
            }

            double moonNodeAngle = fabs(moonPos[0] - nodePos[0]);
            if (moonNodeAngle > 180) moonNodeAngle = 360 - moonNodeAngle;

            // If moon is close to a node, it's likely an eclipse
            if (moonNodeAngle < 12 || fabs(moonNodeAngle - 180) < 12) {
                EclipseData eclipse;
                eclipse.date = julianDayToDateTime(fullMoonJd).date();
                eclipse.time = julianDayToDateTime(fullMoonJd).time();
                eclipse.julianDay = fullMoonJd;

                // Determine eclipse type based on proximity to node
                double proximity = std::min(moonNodeAngle, fabs(moonNodeAngle - 180));
                if (proximity < 4) {
                    eclipse.type = "Total Lunar Eclipse";
                    eclipse.magnitude = 1.0;
                } else if (proximity < 8) {
                    eclipse.type = "Partial Lunar Eclipse";
                    eclipse.magnitude = 0.5;
                } else {
                    eclipse.type = "Penumbral Lunar Eclipse";
                    eclipse.magnitude = 0.2;
                }

                // Lunar eclipses are visible from approximately half the Earth
                eclipse.latitude = 0;
                eclipse.longitude = 0;

                eclipses.append(eclipse);
            }

            // Move to a few days after the full moon
            tjd = fullMoonJd + 10;
        }
    }

    // Sort eclipses by date
    std::sort(eclipses.begin(), eclipses.end(), [](const EclipseData &a, const EclipseData &b) {
        return a.julianDay < b.julianDay;
    });

    return eclipses;
}

void ChartCalculator::addSyzygyAndParsFortuna(QVector<PlanetData> &planets, double jd,
                                              const QVector<HouseData> &houses,
                                              const QVector<AngleData> &angles) const {
    // Calculate Syzygy (Pre-Natal Lunation - New or Full Moon)
    PlanetData syzygy;
    syzygy.id = "Syzygy";

    // Find the exact time of the last New Moon or Full Moon before birth
    double tjd_start = jd - 30; // Start searching 30 days before birth
    char serr[256] = {0};
    int flags = SEFLG_SWIEPH;

    // Function to find exact lunation time using binary search
    auto findExactLunation = [&](double start_jd, double end_jd, bool isNewMoon) -> double {
        double precision = 0.0001; // Precision in days (about 8.6 seconds)
        double mid_jd;

        while (end_jd - start_jd > precision) {
            mid_jd = (start_jd + end_jd) / 2;

            double sun_pos[6], moon_pos[6];
            if (swe_calc_ut(mid_jd, SE_SUN, flags, sun_pos, serr) < 0 ||
                swe_calc_ut(mid_jd, SE_MOON, flags, moon_pos, serr) < 0) {
                return 0; // Error
            }

            // Calculate angular distance between Sun and Moon
            double angle = moon_pos[0] - sun_pos[0];
            while (angle < 0) angle += 360;
            while (angle >= 360) angle -= 360;

            if (isNewMoon) {
                // For New Moon, we want conjunction (0°)
                if (angle > 180) angle = 360 - angle;

                if (angle < 180) {
                    end_jd = mid_jd;
                } else {
                    start_jd = mid_jd;
                }
            } else {
                // For Full Moon, we want opposition (180°)
                double diff = fabs(angle - 180);

                if (diff < 90) {
                    end_jd = mid_jd;
                } else {
                    start_jd = mid_jd;
                }
            }
        }

        return (start_jd + end_jd) / 2;
    };

    // Find approximate times first using a coarser search
    double last_new_moon = 0;
    double last_full_moon = 0;

    // Search for new and full moons
    double curr_jd = jd;
    double prev_angle = -1;

    while (curr_jd > tjd_start) {
        double sun_pos[6], moon_pos[6];
        if (swe_calc_ut(curr_jd, SE_SUN, flags, sun_pos, serr) < 0 ||
            swe_calc_ut(curr_jd, SE_MOON, flags, moon_pos, serr) < 0) {
            break;
        }

        // Calculate angular distance between Sun and Moon
        double angle = moon_pos[0] - sun_pos[0];
        while (angle < 0) angle += 360;
        while (angle >= 360) angle -= 360;

        // Check for New Moon (conjunction)
        if (prev_angle >= 0) {
            // Detect crossing 0° (conjunction)
            if ((prev_angle > 330 && angle < 30) || (prev_angle < 30 && angle > 330)) {
                // Found approximate new moon, now find exact time
                double exact_jd = findExactLunation(curr_jd - 1, curr_jd, true);
                if (exact_jd > 0 && exact_jd < jd && (last_new_moon == 0 || exact_jd > last_new_moon)) {
                    last_new_moon = exact_jd;
                }
            }

            // Detect crossing 180° (opposition)
            if ((prev_angle < 170 && angle > 190) || (prev_angle > 190 && angle < 170)) {
                // Found approximate full moon, now find exact time
                double exact_jd = findExactLunation(curr_jd - 1, curr_jd, false);
                if (exact_jd > 0 && exact_jd < jd && (last_full_moon == 0 || exact_jd > last_full_moon)) {
                    last_full_moon = exact_jd;
                }
            }
        }

        prev_angle = angle;
        curr_jd -= 1.0; // Step back one day
    }

    // Determine which was more recent
    double syzygy_jd = 0;
    bool is_new_moon = false;

    if (last_new_moon > last_full_moon) {
        syzygy_jd = last_new_moon;
        is_new_moon = true;
    } else if (last_full_moon > 0) {
        syzygy_jd = last_full_moon;
        is_new_moon = false;
    }

    // If we found a valid syzygy
    if (syzygy_jd > 0) {
        // Calculate Sun and Moon positions at the syzygy
        double xx_sun[6], xx_moon[6];
        if (swe_calc_ut(syzygy_jd, SE_SUN, flags, xx_sun, serr) >= 0 &&
            swe_calc_ut(syzygy_jd, SE_MOON, flags, xx_moon, serr) >= 0) {

            if (is_new_moon) {
                // For New Moon, use the Sun's position (same as Moon)
                syzygy.longitude = xx_sun[0];
            } else {
                // For Full Moon, use the Sun's position (NOT Sun + 180)
                // This follows traditional Syzygy calculation for Full Moon
                syzygy.longitude = xx_sun[0];
            }

            syzygy.sign = getZodiacSign(syzygy.longitude);
            syzygy.house = findHouse(syzygy.longitude, houses);
            syzygy.isRetrograde = false;
            planets.append(syzygy);

            // Debug output
        }
    } else {
        // Fallback if we couldn't find a syzygy
        qWarning() << "Could not find a valid Syzygy, using fallback";
        double xx[6];
        if (swe_calc_ut(jd, SE_SUN, flags, xx, serr) >= 0) {
            syzygy.longitude = xx[0];
            syzygy.sign = getZodiacSign(syzygy.longitude);
            syzygy.house = findHouse(syzygy.longitude, houses);
            syzygy.isRetrograde = false;
            planets.append(syzygy);
        }
    }

    // Calculate Pars Fortuna (unchanged)
    double asc = 0.0;
    for (const AngleData &angle : angles) {
        if (angle.id == "Asc") {
            asc = angle.longitude;
            break;
        }
    }
    double sun_lon = 0.0;
    double moon_lon = 0.0;
    for (const PlanetData &planet : planets) {
        if (planet.id == "Sun") {
            sun_lon = planet.longitude;
        } else if (planet.id == "Moon") {
            moon_lon = planet.longitude;
        }
    }
    PlanetData parsFortuna;
    parsFortuna.id = "Pars Fortuna";
    parsFortuna.longitude = fmod(asc + moon_lon - sun_lon, 360.0);
    if (parsFortuna.longitude < 0) parsFortuna.longitude += 360.0;
    parsFortuna.sign = getZodiacSign(parsFortuna.longitude);
    parsFortuna.house = findHouse(parsFortuna.longitude, houses);
    parsFortuna.isRetrograde = false;
    planets.append(parsFortuna);
}


void ChartCalculator::calculateAdditionalBodies(QVector<PlanetData> &planets, double jd,
                                                const QVector<HouseData> &houses) const {
    int flags = SEFLG_SWIEPH | SEFLG_SPEED;
    char serr[256] = {0};

    // 1. Add Ceres, Pallas, Juno, Vesta (major asteroids)
    int asteroidIds[] = {SE_CERES, SE_PALLAS, SE_JUNO, SE_VESTA};
    QString asteroidNames[] = {"Ceres", "Pallas", "Juno", "Vesta"};

    for (int i = 0; i < 4; i++) {
        double xx[6];
        if (swe_calc_ut(jd, asteroidIds[i], flags, xx, serr) >= 0) {
            PlanetData asteroid;
            asteroid.id = asteroidNames[i];
            asteroid.longitude = xx[0];
            asteroid.latitude = xx[1];
            asteroid.sign = getZodiacSign(asteroid.longitude);
            asteroid.isRetrograde = (xx[3] < 0);
            asteroid.house = findHouse(asteroid.longitude, houses);
            planets.append(asteroid);
        }
    }

    // 2. Add Vertex (sensitive point)
    for (const HouseData &house : houses) {
        if (house.id == "Vertex") {
            PlanetData vertex;
            vertex.id = "Vertex";
            vertex.longitude = house.longitude;
            vertex.sign = getZodiacSign(vertex.longitude);
            vertex.house = findHouse(vertex.longitude, houses);
            vertex.isRetrograde = false;
            planets.append(vertex);
            break;
        }
    }

    // 3. Add Lilith (Mean Black Moon)
    double xx[6];
    if (swe_calc_ut(jd, SE_MEAN_APOG, flags, xx, serr) >= 0) {
        PlanetData lilith;
        lilith.id = "Lilith";
        lilith.longitude = xx[0];
        lilith.latitude = xx[1];
        lilith.sign = getZodiacSign(lilith.longitude);
        lilith.isRetrograde = (xx[3] < 0);
        lilith.house = findHouse(lilith.longitude, houses);
        planets.append(lilith);
    }



    // 5. Add Part of Spirit (reverse of Pars Fortuna)
    double asc = 0.0, sun_lon = 0.0, moon_lon = 0.0;

    for (const PlanetData &planet : planets) {
        if (planet.id == "Sun") {
            sun_lon = planet.longitude;
        } else if (planet.id == "Moon") {
            moon_lon = planet.longitude;
        }
    }

    // Find Ascendant from houses
    for (const HouseData &house : houses) {
        if (house.id == "1") {
            asc = house.longitude;
            break;
        }
    }

    PlanetData partOfSpirit;
    partOfSpirit.id = "Part of Spirit";
    partOfSpirit.longitude = fmod(asc + sun_lon - moon_lon, 360.0);
    if (partOfSpirit.longitude < 0) partOfSpirit.longitude += 360.0;
    partOfSpirit.sign = getZodiacSign(partOfSpirit.longitude);
    partOfSpirit.house = findHouse(partOfSpirit.longitude, houses);
    partOfSpirit.isRetrograde = false;
    planets.append(partOfSpirit);



    // 8. Add East Point
    // This requires calculating the East Point using the formula:
    // ARMC with latitude 0
    double cusps[13], ascmc[10];
    if (swe_houses_ex(jd, 0, 0.0, 0.0, int(houseSystem[0].toLatin1()), cusps, ascmc) == 0) {
        PlanetData eastPoint;
        eastPoint.id = "East Point";
        eastPoint.longitude = ascmc[2]; // ARMC with latitude 0
        eastPoint.sign = getZodiacSign(eastPoint.longitude);
        eastPoint.house = findHouse(eastPoint.longitude, houses);
        eastPoint.isRetrograde = false;
        planets.append(eastPoint);
    }
}



