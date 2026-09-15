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
extern "C" {
#include "swephexp.h"
#include "swehouse.h"
}

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
    if (m_isInitialized) {
        swe_close();
    }
}



bool ChartCalculator::initialize()
{
    QStringList searchPaths;

#ifdef FLATHUB_BUILD
    searchPaths << "/app/share/swisseph";
#else
    searchPaths << QCoreApplication::applicationDirPath() + "/ephemeris"
                << QCoreApplication::applicationDirPath() + "/../share/Asteria/ephemeris"
                << "/app/share/Asteria/ephemeris"
                << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ephemeris";
#endif

    for (const QString& path : searchPaths) {
        if (path.isEmpty()) continue;

        QDir dir(path);
        if (dir.exists()) {
            if (dir.exists("sepl_18.se1") || dir.exists("sepl_20.se1") ||
                dir.exists("seas_18.se1") || dir.exists("semo_18.se1")) {

                m_ephemerisPath = path;

                QByteArray pathBytes = m_ephemerisPath.toLocal8Bit();
                swe_set_ephe_path(pathBytes.constData());

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

    int year = dateTime.date().year();
    int month = dateTime.date().month();
    int day = dateTime.date().day();
    int hour = dateTime.time().hour();
    int minute = dateTime.time().minute();
    int second = dateTime.time().second();

    double hours = hour + (minute / 60.0) + (second / 3600.0);

    hours -= offset;

    while (hours < 0) {
        hours += 24;
        day--;
        if (day < 1) {
            month--;
            if (month < 1) {
                month = 12;
                year--;
            }
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

    double jd = swe_julday(year, month, day, hours, SE_GREG_CAL);
    return jd;
}




QString ChartCalculator::getZodiacSign(double longitude) const {
    longitude = fmod(longitude, 360.0);
    if (longitude < 0) longitude += 360.0;

    int signIndex = static_cast<int>(longitude / 30.0);
    double degreeInSign = longitude - (signIndex * 30.0);

    int degree = static_cast<int>(degreeInSign);
    int minute = static_cast<int>((degreeInSign - degree) * 60);

    static const QString signs[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
        "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"
    };

    return QString("%1 %2° %3'").arg(signs[signIndex]).arg(degree).arg(minute);
}



QString ChartCalculator::findHouse(double longitude, const QVector<HouseData> &houses) const {
    longitude = fmod(longitude, 360.0);
    if (longitude < 0) longitude += 360.0;

    for (int i = 0; i < houses.size(); i++) {
        int nextHouse = (i + 1) % houses.size();
        double start = houses[i].longitude;
        double end = houses[nextHouse].longitude;

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

    return "House1";
}

QVector<AspectData> ChartCalculator::calculateAspects(const QVector<PlanetData> &planets, double orbMax) const {
    QVector<AspectData> aspects;

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
        {"SQQ", 135.0, orbMax * 0.75}, // Sesquiquadrate
        {"SSX", 30.0, orbMax * 0.75}  // Semi-sextile
    };

    for (int i = 0; i < planets.size(); i++) {
        for (int j = i + 1; j < planets.size(); j++) {
            double angle1 = planets[i].longitude;
            double angle2 = planets[j].longitude;

            double diff = fabs(angle1 - angle2);
            if (diff > 180.0) diff = 360.0 - diff;

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

    QDateTime birthDateTime(birthDate, birthTime);
    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    double jd = dateTimeToJulianDay(birthDateTime, utcOffset);

    QVector<HouseData> houses = calculateHouseCusps(jd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(jd, lat, lon, houseSystem);
    data.planets = calculatePlanetPositions(jd, houses);

    addSyzygyAndParsFortuna(data.planets, jd, houses, data.angles);
    calculateAdditionalBodies(data.planets, jd, houses);

    orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);
    return data;
}




QVector<HouseData> ChartCalculator::calculateHouseCusps(double jd, double lat, double lon, const QString &houseSystem) const {
    QVector<HouseData> houses;

    char hsys = 'P'; // Placidus by default
    if (houseSystem == "Placidus") hsys = 'P';
    else if (houseSystem == "Koch") hsys = 'K';
    else if (houseSystem == "Porphyrius") hsys = 'O';
    else if (houseSystem == "Regiomontanus") hsys = 'R';
    else if (houseSystem == "Campanus") hsys = 'C';
    else if (houseSystem == "Equal") hsys = 'E';
    else if (houseSystem == "Whole Sign") hsys = 'W';



    double cusps[13] = {0};
    double ascmc[10] = {0};

    int result = swe_houses(jd, lat, lon, hsys, cusps, ascmc);

    if (result < 0) {
        qWarning() << "Error calculating house cusps";
        return houses;
    }

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

    char hsys = 'P'; // Placidus by default
    if (houseSystem == "Placidus") hsys = 'P';
    else if (houseSystem == "Koch") hsys = 'K';
    else if (houseSystem == "Porphyrius") hsys = 'O';
    else if (houseSystem == "Regiomontanus") hsys = 'R';
    else if (houseSystem == "Campanus") hsys = 'C';
    else if (houseSystem == "Equal") hsys = 'E';
    else if (houseSystem == "Whole Sign") hsys = 'W';


    double cusps[13] = {0};
    double ascmc[10] = {0};

    int result = swe_houses(jd, lat, lon, hsys, cusps, ascmc);
    if (result < 0) {
        qWarning() << "Error calculating angles";
        return angles;
    }

    AngleData asc;
    asc.id = "Asc";
    asc.longitude = ascmc[0];
    asc.sign = getZodiacSign(asc.longitude);
    angles.append(asc);

    AngleData mc;
    mc.id = "MC";
    mc.longitude = ascmc[1];
    mc.sign = getZodiacSign(mc.longitude);
    angles.append(mc);

    AngleData desc;
    desc.id = "Desc";
    desc.longitude = fmod(ascmc[0] + 180.0, 360.0);
    desc.sign = getZodiacSign(desc.longitude);
    angles.append(desc);

    AngleData ic;
    ic.id = "IC";
    ic.longitude = fmod(ascmc[1] + 180.0, 360.0);
    ic.sign = getZodiacSign(ic.longitude);
    angles.append(ic);

    return angles;
}

QVector<PlanetData> ChartCalculator::calculatePlanetPositions(double jd, const QVector<HouseData> &houses) const {
    QVector<PlanetData> planets;
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

    for (int i = 0; i < numPlanets; i++) {
        double xx[6]; // Position and speed
        char serr[256];

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




ChartData ChartCalculator::calculateSolarReturn(const QDate &birthDate,
                                                const QTime &birthTime,
                                                const QString &utcOffset,
                                                const QString &latitude,
                                                const QString &longitude,
                                                const QString &houseSystem,

                                                int year)
{
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_SUN, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Sun position: %1").arg(serr);
        return data;
    }

    double sunLongitude = xx[0];

    QDate approxDate(year, birthDate.month(), birthDate.day());
    if (!approxDate.isValid()) {
        approxDate = QDate(year, birthDate.month(), birthDate.daysInMonth());
    }

    QDateTime approxDateTime(approxDate, birthTime);
    double approxJd = dateTimeToJulianDay(approxDateTime, utcOffset);

    double srJd = findPlanetaryEvent(SE_SUN, approxJd, sunLongitude);

    if (srJd <= 0) {
        m_lastError = "Could not find solar return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(srJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(srJd, houses);
    addSyzygyAndParsFortuna(data.planets, srJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, srJd, houses);

    double orbMax = getOrbMax();

    data.aspects = calculateAspects(data.planets, orbMax);


    QDateTime returnDateTime = julianDayToDateTime(srJd, utcOffset);

    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = srJd;

    return data;
}

ChartData ChartCalculator::calculateSaturnReturn(const QDate &birthDate,
                                                 const QTime &birthTime,
                                                 const QString &utcOffset,
                                                 const QString &latitude,
                                                 const QString &longitude,
                                                 const QString &houseSystem,
                                                 int returnNumber) {
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);
    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_SATURN, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Saturn position: %1").arg(serr);
        return data;
    }

    double saturnLongitude = xx[0];

    double approxJd = birthJd + (returnNumber * 29.5 * 365.25);

    double srJd = findPlanetaryEvent(SE_SATURN, approxJd, saturnLongitude);

    if (srJd <= 0) {
        m_lastError = "Could not find Saturn return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(srJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(srJd, houses);
    addSyzygyAndParsFortuna(data.planets, srJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, srJd, houses);
    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(srJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = srJd;

    return data;
}

double ChartCalculator::findPlanetaryEvent(int planet, double startJd, double targetLongitude) const {
    const double searchWindow = 60.0;

    const double step = 1.0;

    const double tolerance = 0.0001;

    double jd = startJd - searchWindow / 2;
    double endJd = startJd + searchWindow / 2;

    double bestJd = 0;
    double bestDiff = 360.0;

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
        return findPlanetaryEvent(planet, startJd - searchWindow, targetLongitude);
    }

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


    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);
    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> natalHouses = calculateHouseCusps(birthJd, lat, lon, houseSystem);
    QVector<AngleData> natalAngles = calculateAngles(birthJd, lat, lon, houseSystem);
    QVector<PlanetData> natalPlanets = calculatePlanetPositions(birthJd, natalHouses);
    if (AsteriaFlags::additionalBodiesEnabled) {
        addSyzygyAndParsFortuna(natalPlanets, birthJd, natalHouses, natalAngles);
        calculateAdditionalBodies(natalPlanets, birthJd, natalHouses);
    }

    QDateTime transitStartDateTime(transitStartDate, QTime(0, 0));
    double transitStartJd = dateTimeToJulianDay(transitStartDateTime, "+0:00");

    QStringList includedTargetObjects;
    QStringList excludedTransitingObjects;

    if (AsteriaFlags::additionalBodiesEnabled) {
        includedTargetObjects = {"Sun", "Moon", "Mercury", "Venus", "Mars",
                                 "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto",
                                 "Lilith", "Ceres", "Pallas", "Juno", "Vesta", "Vertex", "East Point", "Chiron",
                                 "Pars Fortuna", "North Node", "South Node"};
        excludedTransitingObjects = {""};
    } else {
        includedTargetObjects = {"Sun", "Moon", "Mercury", "Venus", "Mars",
                                 "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"};
        excludedTransitingObjects = {"Chiron", "North Node", "South Node"};
    }

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
        {"SQQ", 135.0, orbMax * 0.75},


        {"SSX", 30.0, orbMax * 0.75}
    };
    int numAspectTypes = sizeof(aspectTypes) / sizeof(aspectTypes[0]);

    QString report;
    report += "---TRANSITS---\n";

    for (int day = 0; day < numberOfDays; day++) {
        double transitJd = transitStartJd + day;
        QDateTime transitDateTime = julianDayToDateTime(transitJd);
        QString dateStr = transitDateTime.toString("yyyy/MM/dd");

        QVector<HouseData> transitHouses = calculateHouseCusps(transitJd, lat, lon, houseSystem);
        QVector<AngleData> transitAngles = calculateAngles(transitJd, lat, lon, houseSystem);
        QVector<PlanetData> transitPlanets = calculatePlanetPositions(transitJd, transitHouses);

        if (AsteriaFlags::additionalBodiesEnabled) {
            addSyzygyAndParsFortuna(transitPlanets, transitJd, transitHouses, transitAngles);
            calculateAdditionalBodies(transitPlanets, transitJd, transitHouses);
        }


        QStringList dayAspects;
        for (const PlanetData &transitPlanet : transitPlanets) {
            if (excludedTransitingObjects.contains(transitPlanet.id)) {
                continue;
            }
            for (const PlanetData &natalPlanet : natalPlanets) {
                if (!includedTargetObjects.contains(natalPlanet.id)) {
                    continue;
                }
                double diff = fabs(transitPlanet.longitude - natalPlanet.longitude);
                if (diff > 180.0) diff = 360.0 - diff;

                for (int j = 0; j < numAspectTypes; j++) {
                    double orb = fabs(diff - aspectTypes[j].angle);
                    if (orb <= aspectTypes[j].orb) {
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


                        break;
                    }
                }
            }
        }
        report += dateStr + ": " + dayAspects.join(", ") + "\n";
    }
    return report;
}

QDateTime ChartCalculator::julianDayToDateTime(double jd, const QString &utcOffset) const {
    int year, month, day, hour, minute, second;
    double hour_fraction;

    swe_revjul(jd, SE_GREG_CAL, &year, &month, &day, &hour_fraction);

    hour = static_cast<int>(hour_fraction);
    minute = static_cast<int>((hour_fraction - hour) * 60);
    second = static_cast<int>(((hour_fraction - hour) * 60 - minute) * 60);

    QDate date(year, month, day);
    QTime time(hour, minute, second);
    QDateTime utcDateTime(date, time, QTimeZone::UTC);

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

    double startJd = dateTimeToJulianDay(QDateTime(startDate, QTime(0, 0)), "+0:00");
    double endJd = dateTimeToJulianDay(QDateTime(endDate, QTime(23, 59, 59)), "+0:00");
    char serr[256] = {0};

    if (solarEclipses) {
        double tjd = startJd;
        while (tjd < endJd) {
            double tret[10] = {0};
            int32 iflgret = swe_sol_eclipse_when_glob(tjd, SEFLG_SWIEPH, 0, tret, 0, serr);

            if (iflgret < 0) {
                m_lastError = QString("Error finding solar eclipse: %1").arg(serr);
                break;
            }

            tjd = tret[0];
            if (tjd <= endJd) {
                EclipseData eclipse;
                eclipse.date = julianDayToDateTime(tjd).date();
                eclipse.time = julianDayToDateTime(tjd).time();
                eclipse.julianDay = tjd;

                if (iflgret & SE_ECL_TOTAL) eclipse.type = "Total Solar Eclipse";
                else if (iflgret & SE_ECL_ANNULAR) eclipse.type = "Annular Solar Eclipse";
                else if (iflgret & SE_ECL_PARTIAL) eclipse.type = "Partial Solar Eclipse";
                else if (iflgret & SE_ECL_ANNULAR_TOTAL) eclipse.type = "Hybrid Solar Eclipse";
                else eclipse.type = "Solar Eclipse";

                double geopos[3] = {0, 0, 0}; // Will be filled by the function
                double attr[20] = {0};
                if (swe_sol_eclipse_where(tjd, SEFLG_SWIEPH, geopos, attr, serr) >= 0) {
                    eclipse.longitude = geopos[0];
                    eclipse.latitude = geopos[1];
                    eclipse.magnitude = attr[0];
                }

                eclipses.append(eclipse);
                tjd += 10; // Next search
            } else {
                break;
            }
        }
    }

    if (lunarEclipses) {
        double tjd = startJd;
        while (tjd < endJd) {
            double tret[10] = {0};
            int32 iflgret = swe_lun_eclipse_when(tjd, SEFLG_SWIEPH, 0, tret, 0, serr);

            if (iflgret < 0) {
                m_lastError = QString("Error finding lunar eclipse: %1").arg(serr);
                break;
            }

            tjd = tret[0];
            if (tjd <= endJd) {
                EclipseData eclipse;
                eclipse.date = julianDayToDateTime(tjd).date();
                eclipse.time = julianDayToDateTime(tjd).time();
                eclipse.julianDay = tjd;

                if (iflgret & SE_ECL_TOTAL) {
                    eclipse.type = "Total Lunar Eclipse";
                    eclipse.magnitude = 1.0;
                } else if (iflgret & SE_ECL_PARTIAL) {
                    eclipse.type = "Partial Lunar Eclipse";
                    eclipse.magnitude = 0.7;
                } else if (iflgret & SE_ECL_PENUMBRAL) {
                    eclipse.type = "Penumbral Lunar Eclipse";
                    eclipse.magnitude = 0.3;
                } else {
                    eclipse.type = "Lunar Eclipse";
                    eclipse.magnitude = 0.5;
                }

                eclipse.latitude = 0;
                eclipse.longitude = 0;

                eclipses.append(eclipse);
                tjd += 10;
            } else {
                break;
            }
        }
    }

    std::sort(eclipses.begin(), eclipses.end(), [](const EclipseData &a, const EclipseData &b) {
        return a.julianDay < b.julianDay;
    });

    return eclipses;
}

void ChartCalculator::addSyzygyAndParsFortuna(QVector<PlanetData> &planets, double jd,
                                              const QVector<HouseData> &houses,
                                              const QVector<AngleData> &angles) const {
    PlanetData syzygy;
    syzygy.id = "Syzygy";

    double tjd_start = jd - 30; // Start searching 30 days before birth
    char serr[256] = {0};
    int flags = SEFLG_SWIEPH;

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

            double angle = moon_pos[0] - sun_pos[0];
            while (angle < 0) angle += 360;
            while (angle >= 360) angle -= 360;

            if (isNewMoon) {
                if (angle > 180) angle = 360 - angle;

                if (angle < 180) {
                    end_jd = mid_jd;
                } else {
                    start_jd = mid_jd;
                }
            } else {
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

    double last_new_moon = 0;
    double last_full_moon = 0;

    double curr_jd = jd;
    double prev_angle = -1;

    while (curr_jd > tjd_start) {
        double sun_pos[6], moon_pos[6];
        if (swe_calc_ut(curr_jd, SE_SUN, flags, sun_pos, serr) < 0 ||
            swe_calc_ut(curr_jd, SE_MOON, flags, moon_pos, serr) < 0) {
            break;
        }

        double angle = moon_pos[0] - sun_pos[0];
        while (angle < 0) angle += 360;
        while (angle >= 360) angle -= 360;

        if (prev_angle >= 0) {
            if ((prev_angle > 330 && angle < 30) || (prev_angle < 30 && angle > 330)) {
                double exact_jd = findExactLunation(curr_jd - 1, curr_jd, true);
                if (exact_jd > 0 && exact_jd < jd && (last_new_moon == 0 || exact_jd > last_new_moon)) {
                    last_new_moon = exact_jd;
                }
            }

            if ((prev_angle < 170 && angle > 190) || (prev_angle > 190 && angle < 170)) {
                double exact_jd = findExactLunation(curr_jd - 1, curr_jd, false);
                if (exact_jd > 0 && exact_jd < jd && (last_full_moon == 0 || exact_jd > last_full_moon)) {
                    last_full_moon = exact_jd;
                }
            }
        }

        prev_angle = angle;
        curr_jd -= 1.0; // Step back one day
    }

    double syzygy_jd = 0;
    bool is_new_moon = false;

    if (last_new_moon > last_full_moon) {
        syzygy_jd = last_new_moon;
        is_new_moon = true;
    } else if (last_full_moon > 0) {
        syzygy_jd = last_full_moon;
        is_new_moon = false;
    }

    if (syzygy_jd > 0) {
        double xx_sun[6], xx_moon[6];
        if (swe_calc_ut(syzygy_jd, SE_SUN, flags, xx_sun, serr) >= 0 &&
            swe_calc_ut(syzygy_jd, SE_MOON, flags, xx_moon, serr) >= 0) {

            if (is_new_moon) {
                syzygy.longitude = xx_sun[0];
            } else {
                syzygy.longitude = xx_sun[0];
            }

            syzygy.sign = getZodiacSign(syzygy.longitude);
            syzygy.house = findHouse(syzygy.longitude, houses);
            syzygy.isRetrograde = false;
            planets.append(syzygy);

        }
    } else {
        double xx[6];
        if (swe_calc_ut(jd, SE_SUN, flags, xx, serr) >= 0) {
            syzygy.longitude = xx[0];
            syzygy.sign = getZodiacSign(syzygy.longitude);
            syzygy.house = findHouse(syzygy.longitude, houses);
            syzygy.isRetrograde = false;
            planets.append(syzygy);
        }
    }

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



    double asc = 0.0, sun_lon = 0.0, moon_lon = 0.0;

    for (const PlanetData &planet : planets) {
        if (planet.id == "Sun") {
            sun_lon = planet.longitude;
        } else if (planet.id == "Moon") {
            moon_lon = planet.longitude;
        }
    }

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


ChartData ChartCalculator::calculateLunarReturn(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    const QDate &targetDate // The date for which to find the lunar return
    )
{
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_MOON, flag, xx, serr);
    if (ret < 0) {
        m_lastError = QString("Error calculating Moon position: %1").arg(serr);
        return data;
    }
    double moonLongitude = xx[0];

    QDate approxDate = targetDate;
    QTime approxTime = birthTime; // Use birth time as a starting guess
    QDateTime approxDateTime(approxDate, approxTime);
    double approxJd = dateTimeToJulianDay(approxDateTime, utcOffset);

    double lrJd = findPlanetaryEvent(SE_MOON, approxJd, moonLongitude);
    if (lrJd <= 0) {
        m_lastError = "Could not find lunar return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(lrJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(lrJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(lrJd, houses);

    addSyzygyAndParsFortuna(data.planets, lrJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, lrJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(lrJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = lrJd;

    return data;
}


bool ChartCalculator::calculateSunriseSunset(
    const QDate &date,
    double latitude,
    double longitude,
    QDateTime &sunrise,
    QDateTime &sunset,
    QString &errorMsg
    ) {
    QDateTime dt(date, QTime(0, 0), QTimeZone::utc());
    double jd_ut = swe_julday(dt.date().year(), dt.date().month(), dt.date().day(), 0.0, SE_GREG_CAL);

    double geopos[3] = { longitude, latitude, 0.0 }; // longitude, latitude, altitude (meters)
    double tret_rise = 0.0, tret_set = 0.0;
    char serr[256] = {0};

    int ret_rise = swe_rise_trans(
        jd_ut,
        SE_SUN,
        NULL,
        SEFLG_SWIEPH,
        SE_CALC_RISE,
        geopos,
        0,      // atpress (0 = default)
        0,      // attemp (0 = default)
        &tret_rise,
        serr
        );
    if (ret_rise < 0) {
        errorMsg = QString("Sunrise calculation error: %1").arg(serr);
        return false;
    }

    int ret_set = swe_rise_trans(
        jd_ut,
        SE_SUN,
        NULL,
        SEFLG_SWIEPH,
        SE_CALC_SET,
        geopos,
        0,      // atpress (0 = default)
        0,      // attemp (0 = default)
        &tret_set,
        serr
        );
    if (ret_set < 0) {
        errorMsg = QString("Sunset calculation error: %1").arg(serr);
        return false;
    }

    sunrise = julianDayToDateTime(tret_rise);
    sunset = julianDayToDateTime(tret_set);
    errorMsg.clear();
    return true;
}


ChartData ChartCalculator::calculateJupiterReturn(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_JUPITER, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Jupiter position: %1").arg(serr);
        return data;
    }

    double jupiterLongitude = xx[0];

    double approxJd = birthJd + (returnNumber * 11.86 * 365.25);

    double jrJd = findPlanetaryEvent(SE_JUPITER, approxJd, jupiterLongitude);

    if (jrJd <= 0) {
        m_lastError = "Could not find Jupiter return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(jrJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(jrJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(jrJd, houses);

    addSyzygyAndParsFortuna(data.planets, jrJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, jrJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(jrJd, utcOffset);

    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = jrJd;

    return data;
}


ChartData ChartCalculator::calculateVenusReturn(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_VENUS, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Venus position: %1").arg(serr);
        return data;
    }

    double venusLongitude = xx[0];

    double approxJd = birthJd + (returnNumber * 0.61519726 * 365.25);

    double vrJd = findPlanetaryEvent(SE_VENUS, approxJd, venusLongitude);

    if (vrJd <= 0) {
        m_lastError = "Could not find Venus return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(vrJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(vrJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(vrJd, houses);

    addSyzygyAndParsFortuna(data.planets, vrJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, vrJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(vrJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = vrJd;

    return data;
}

ChartData ChartCalculator::calculateMarsReturn(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_MARS, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Mars position: %1").arg(serr);
        return data;
    }

    double marsLongitude = xx[0];

    double approxJd = birthJd + (returnNumber * 1.8808476 * 365.25);

    double mrJd = findPlanetaryEvent(SE_MARS, approxJd, marsLongitude);

    if (mrJd <= 0) {
        m_lastError = "Could not find Mars return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(mrJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(mrJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(mrJd, houses);

    addSyzygyAndParsFortuna(data.planets, mrJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, mrJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(mrJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = mrJd;

    return data;
}

ChartData ChartCalculator::calculateMercuryReturn(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    ChartData data;

    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_MERCURY, flag, xx, serr);

    if (ret < 0) {
        m_lastError = QString("Error calculating Mercury position: %1").arg(serr);
        return data;
    }

    double mercuryLongitude = xx[0];

    double approxJd = birthJd + (returnNumber * 0.2408467 * 365.25);

    double mrJd = findPlanetaryEvent(SE_MERCURY, approxJd, mercuryLongitude);

    if (mrJd <= 0) {
        m_lastError = "Could not find Mercury return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(mrJd, lat, lon, houseSystem);
    data.houses = houses;

    data.angles = calculateAngles(mrJd, lat, lon, houseSystem);

    data.planets = calculatePlanetPositions(mrJd, houses);

    addSyzygyAndParsFortuna(data.planets, mrJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, mrJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(mrJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = mrJd;

    return data;
}


ChartData ChartCalculator::calculateUranusReturn(const QDate &birthDate, const QTime &birthTime, const QString &utcOffset, const QString &latitude, const QString &longitude, const QString &houseSystem, int returnNumber)
{
    ChartData data;
    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_URANUS, flag, xx, serr);
    if (ret < 0) {
        m_lastError = QString("Error calculating Uranus position: %1").arg(serr);
        return data;
    }
    double uranusLongitude = xx[0];

    double uranusPeriod = 84.016846;
    double approxJd = birthJd + (returnNumber * uranusPeriod * 365.25);

    double srJd = findPlanetaryEvent(SE_URANUS, approxJd, uranusLongitude);
    if (srJd <= 0) {
        m_lastError = "Could not find Uranus return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;
    data.angles = calculateAngles(srJd, lat, lon, houseSystem);
    data.planets = calculatePlanetPositions(srJd, houses);

    addSyzygyAndParsFortuna(data.planets, srJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, srJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(srJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = srJd;

    return data;
}

ChartData ChartCalculator::calculateNeptuneReturn(const QDate &birthDate, const QTime &birthTime, const QString &utcOffset, const QString &latitude, const QString &longitude, const QString &houseSystem, int returnNumber)
{
    ChartData data;
    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_NEPTUNE, flag, xx, serr);
    if (ret < 0) {
        m_lastError = QString("Error calculating Neptune position: %1").arg(serr);
        return data;
    }
    double neptuneLongitude = xx[0];

    double neptunePeriod = 164.79132;
    double approxJd = birthJd + (returnNumber * neptunePeriod * 365.25);

    double srJd = findPlanetaryEvent(SE_NEPTUNE, approxJd, neptuneLongitude);
    if (srJd <= 0) {
        m_lastError = "Could not find Neptune return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;
    data.angles = calculateAngles(srJd, lat, lon, houseSystem);
    data.planets = calculatePlanetPositions(srJd, houses);

    addSyzygyAndParsFortuna(data.planets, srJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, srJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(srJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = srJd;

    return data;
}

ChartData ChartCalculator::calculatePlutoReturn(const QDate &birthDate, const QTime &birthTime, const QString &utcOffset, const QString &latitude, const QString &longitude, const QString &houseSystem, int returnNumber)
{
    ChartData data;
    if (!m_isInitialized) {
        m_lastError = "Swiss Ephemeris not initialized";
        return data;
    }

    QDateTime birthDateTime(birthDate, birthTime);
    double birthJd = dateTimeToJulianDay(birthDateTime, utcOffset);

    double xx[6];
    char serr[256];
    int flag = SEFLG_SWIEPH;
    int ret = swe_calc_ut(birthJd, SE_PLUTO, flag, xx, serr);
    if (ret < 0) {
        m_lastError = QString("Error calculating Pluto position: %1").arg(serr);
        return data;
    }
    double plutoLongitude = xx[0];

    double plutoPeriod = 248.00;
    double approxJd = birthJd + (returnNumber * plutoPeriod * 365.25);

    double srJd = findPlanetaryEvent(SE_PLUTO, approxJd, plutoLongitude);
    if (srJd <= 0) {
        m_lastError = "Could not find Pluto return";
        return data;
    }

    double lat = latitude.toDouble();
    double lon = longitude.toDouble();

    QVector<HouseData> houses = calculateHouseCusps(srJd, lat, lon, houseSystem);
    data.houses = houses;
    data.angles = calculateAngles(srJd, lat, lon, houseSystem);
    data.planets = calculatePlanetPositions(srJd, houses);

    addSyzygyAndParsFortuna(data.planets, srJd, houses, data.angles);
    calculateAdditionalBodies(data.planets, srJd, houses);

    double orbMax = getOrbMax();
    data.aspects = calculateAspects(data.planets, orbMax);

    QDateTime returnDateTime = julianDayToDateTime(srJd, utcOffset);
    data.returnDate = returnDateTime.date();
    data.returnTime = returnDateTime.time();
    data.returnJulianDay = srJd;

    return data;
}
