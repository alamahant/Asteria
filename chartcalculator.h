#ifndef CHARTCALCULATOR_H
#define CHARTCALCULATOR_H

#include <QObject>
#include <QDate>
#include <QTime>
#include <QString>
#include <QVector>

// Forward declare Swiss Ephemeris types to avoid including C headers in header
typedef void* SWEPH_HANDLE;

// Structure to hold planet information
struct PlanetData {
    QString id;
    QString sign;
    double longitude;
    double latitude;
    QString house;
    bool isRetrograde=false;
};

// Structure to hold house information
struct HouseData {
    QString id;
    QString sign;
    double longitude;
};

// Structure to hold angle information
struct AngleData {
    QString id;
    QString sign;
    double longitude;
};

// Structure to hold aspect information
struct AspectData {
    QString planet1;
    QString planet2;
    QString aspectType;
    double orb;
};

// Structure to hold the complete chart data
struct ChartData {
    QVector<PlanetData> planets;
    QVector<HouseData> houses;
    QVector<AngleData> angles;
    QVector<AspectData> aspects;
};

// Structure for eclipse data
struct EclipseData {
    QDate date;
    QTime time;
    double julianDay;
    QString type;
    double magnitude;
    double latitude;   // Geographic latitude where the eclipse is maximum
    double longitude;  // Geographic longitude where the eclipse is maximum
};

class ChartCalculator : public QObject
{
    Q_OBJECT
public:
    explicit ChartCalculator(QObject *parent = nullptr);
    ~ChartCalculator();

    // Initialize Swiss Ephemeris
    bool initialize();


    // Calculate chart with the given birth data
    ChartData calculateChart(const QDate &birthDate,
                             const QTime &birthTime,
                             const QString &utcOffset,
                             const QString &latitude,
                             const QString &longitude,
                             const QString &houseSystem,
                             double orbMax = 8.0);

    // Calculate transits
    QString calculateTransits(const QDate &birthDate,
                              const QTime &birthTime,
                              const QString &utcOffset,
                              const QString &latitude,
                              const QString &longitude,
                              const QDate &transitStartDate,
                              int numberOfDays);

    // New methods using Swiss Ephemeris

    // Calculate solar return for a specific year
    ChartData calculateSolarReturn(const QDate &birthDate,
                                   const QTime &birthTime,
                                   const QString &utcOffset,
                                   const QString &latitude,
                                   const QString &longitude,
                                   int year);

    // Calculate Saturn return
    ChartData calculateSaturnReturn(const QDate &birthDate,
                                    const QTime &birthTime,
                                    const QString &utcOffset,
                                    const QString &latitude,
                                    const QString &longitude,
                                    int returnNumber = 1);

    // Find eclipses in a date range

    QVector<EclipseData> findEclipses(const QDate &startDate,
                                      const QDate &endDate,
                                      bool solarEclipses = true,
                                      bool lunarEclipses = true);

    // Check if the calculator is available
    bool isAvailable() const;

    // Get the last error message
    QString getLastError() const;

private:
    //ChartData parseOutput(const QString &output);

    // Helper methods for Swiss Ephemeris calculations
    double dateTimeToJulianDay(const QDateTime &dateTime, const QString &utcOffset) const;
    QDateTime julianDayToDateTime(double jd, const QString &utcOffset = "+0:00") const;


    QString getZodiacSign(double longitude) const;
    QString findHouse(double longitude, const QVector<HouseData> &houses) const;
    QVector<AspectData> calculateAspects(const QVector<PlanetData> &planets, double orbMax) const;

    // Swiss Ephemeris calculation methods
    QVector<HouseData> calculateHouseCusps(double jd, double lat, double lon, const QString &houseSystem) const;
    QVector<PlanetData> calculatePlanetPositions(double jd, const QVector<HouseData> &houses) const;

    void addSyzygyAndParsFortuna(QVector<PlanetData> &planets, double jd, const QVector<HouseData> &houses, const QVector<AngleData> &angles) const;

    QVector<AngleData> calculateAngles(double jd, double lat, double lon, const QString &houseSystem) const;


    // Find specific planetary event (for returns)
    double findPlanetaryEvent(int planet, double startJd, double targetLongitude) const;

    QString m_lastError;
    QString m_ephemerisPath;private:

    bool m_isInitialized;
    QString houseSystem = "Placidus";
    //QString houseSystem;
    void calculateAdditionalBodies(QVector<PlanetData> &planets, double jd,const QVector<HouseData> &houses) const;

};

#endif // CHARTCALCULATOR_H

