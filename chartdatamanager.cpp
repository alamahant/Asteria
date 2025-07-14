#include "chartdatamanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include"Globals.h"

ChartDataManager::ChartDataManager(QObject *parent)
    : QObject(parent)
    , m_calculator(new ChartCalculator(this))
{
}

ChartDataManager::~ChartDataManager()
{
    // QObject parent-child relationship will handle deletion
}



bool ChartDataManager::isCalculatorAvailable() const
{
    return m_calculator->isAvailable();
}

ChartData ChartDataManager::calculateChart(const QDate &birthDate,
                                           const QTime &birthTime,
                                           const QString &utcOffset,
                                           const QString &latitude,
                                           const QString &longitude,
                                           const QString &houseSystem,
                                           double orbMax)
{
    // Clear any previous error
    m_lastError.clear();

    orbMax = getOrbMax();
    // Calculate the chart
    ChartData data = m_calculator->calculateChart(birthDate, birthTime, utcOffset,
                                                  latitude, longitude, houseSystem, orbMax);

    // Check for errors
    if (!m_calculator->getLastError().isEmpty()) {
        m_lastError = m_calculator->getLastError();
    }

    return data;
}

QJsonObject ChartDataManager::calculateChartAsJson(const QDate &birthDate,
                                                   const QTime &birthTime,
                                                   const QString &utcOffset,
                                                   const QString &latitude,
                                                   const QString &longitude,
                                                   const QString &houseSystem,
                                                   double orbMax)
{
    // Calculate the chart
    orbMax = getOrbMax();

    ChartData data = calculateChart(birthDate, birthTime, utcOffset,
                                    latitude, longitude, houseSystem, orbMax);

    // If there was an error, return an empty object
    if (!m_lastError.isEmpty()) {
        return QJsonObject{{"error", m_lastError}};
    }

    // Convert to JSON
    return chartDataToJson(data);
}
/*
QJsonObject ChartDataManager::chartDataToJson(const ChartData &data)
{
    QJsonObject json;

    // Add each component to the JSON object
    json["planets"] = planetsToJson(data.planets);
    json["houses"] = housesToJson(data.houses);
    json["angles"] = anglesToJson(data.angles);
    json["aspects"] = aspectsToJson(data.aspects);

    if (data.returnDate.isValid())
        json["solarReturnDate"] = data.returnDate.toString("dd/MM/yyyy");
    if (data.returnTime.isValid())
        json["solarReturnTime"] = data.returnTime.toString("HH:mm");
    if (data.returnJulianDay > 0)
        json["solarReturnJulianDay"] = QString::number(data.returnJulianDay, 'f', 6);

    return json;
}
*/

QJsonObject ChartDataManager::chartDataToJson(const ChartData &data)
{
    QJsonObject json;

    // Add each component to the JSON object
    json["planets"] = planetsToJson(data.planets);
    json["houses"] = housesToJson(data.houses);
    json["angles"] = anglesToJson(data.angles);
    json["aspects"] = aspectsToJson(data.aspects);

    // Use generic keys for all types of returns
    if (data.returnDate.isValid())
        json["returnDate"] = data.returnDate.toString("dd/MM/yyyy");

    if (data.returnTime.isValid())
        json["returnTime"] = data.returnTime.toString("HH:mm");

    if (data.returnJulianDay > 0)
        json["returnJulianDay"] = QString::number(data.returnJulianDay, 'f', 6);

    return json;
}




QJsonArray ChartDataManager::planetsToJson(const QVector<PlanetData> &planets)
{
    QJsonArray jsonArray;
    for (const PlanetData &planet : planets) {
        QJsonObject jsonPlanet;
        jsonPlanet["id"] = planet.id;
        jsonPlanet["sign"] = planet.sign;
        jsonPlanet["longitude"] = planet.longitude;
        jsonPlanet["house"] = planet.house;
        jsonPlanet["isRetrograde"] = planet.isRetrograde;  // Add the retrograde status
        jsonArray.append(jsonPlanet);
    }
    return jsonArray;
}

QJsonArray ChartDataManager::housesToJson(const QVector<HouseData> &houses)
{
    QJsonArray jsonArray;

    for (const HouseData &house : houses) {
        QJsonObject jsonHouse;
        jsonHouse["id"] = house.id;
        jsonHouse["sign"] = house.sign;
        jsonHouse["longitude"] = house.longitude;
        jsonArray.append(jsonHouse);
    }

    return jsonArray;
}

QJsonArray ChartDataManager::anglesToJson(const QVector<AngleData> &angles)
{
    QJsonArray jsonArray;

    for (const AngleData &angle : angles) {
        QJsonObject jsonAngle;
        jsonAngle["id"] = angle.id;
        jsonAngle["sign"] = angle.sign;
        jsonAngle["longitude"] = angle.longitude;
        jsonArray.append(jsonAngle);
    }

    return jsonArray;
}

QJsonArray ChartDataManager::aspectsToJson(const QVector<AspectData> &aspects)
{
    QJsonArray jsonArray;

    for (const AspectData &aspect : aspects) {
        QJsonObject jsonAspect;
        jsonAspect["planet1"] = aspect.planet1;
        jsonAspect["planet2"] = aspect.planet2;
        jsonAspect["aspectType"] = aspect.aspectType;
        jsonAspect["orb"] = aspect.orb;
        jsonArray.append(jsonAspect);
    }

    return jsonArray;
}

QString ChartDataManager::getLastError() const
{
    return m_lastError;
}

///////////////////////Predictions

QString ChartDataManager::calculateTransits(const QDate &birthDate,
                                            const QTime &birthTime,
                                            const QString &utcOffset,
                                            const QString &latitude,
                                            const QString &longitude,
                                            const QDate &transitStartDate,
                                            int numberOfDays) {
    // Clear any previous error
    m_lastError.clear();

    // Calculate the transits
    QString output = m_calculator->calculateTransits(birthDate, birthTime, utcOffset,
                                                     latitude, longitude,
                                                     transitStartDate, numberOfDays);

    // Check for errors
    if (!m_calculator->getLastError().isEmpty()) {
        m_lastError = m_calculator->getLastError();
    }

    return output;
}

QJsonObject ChartDataManager::calculateTransitsAsJson(const QDate &birthDate,
                                                      const QTime &birthTime,
                                                      const QString &utcOffset,
                                                      const QString &latitude,
                                                      const QString &longitude,
                                                      const QDate &transitStartDate,
                                                      int numberOfDays) {
    // Calculate the transits
    QString output = calculateTransits(birthDate, birthTime, utcOffset,
                                       latitude, longitude,
                                       transitStartDate, numberOfDays);

    // Debug the raw output

    // If there was an error, return an empty object
    if (!m_lastError.isEmpty()) {
        return QJsonObject{{"error", m_lastError}};
    }

    // Create a simple JSON object with the raw output
    QJsonObject json;
    json["birthDate"] = birthDate.toString("yyyy-MM-dd");
    json["birthTime"] = birthTime.toString("HH:mm");
    json["latitude"] = latitude;
    json["longitude"] = longitude;
    json["transitStartDate"] = transitStartDate.toString("yyyy-MM-dd");
    json["numberOfDays"] = QString::number(numberOfDays);
    json["rawTransitData"] = output;

    // Debug the JSON object

    return json;
}

QJsonArray ChartDataManager::calculateEclipsesAsJson(
    const QDate &fromDate,
    const QDate &toDate,
    bool solarEclipses,
    bool lunarEclipses)
{
    m_lastError.clear();

    QVector<EclipseData> eclipses = m_calculator->findEclipses(fromDate, toDate, solarEclipses, lunarEclipses);

    if (!m_calculator->getLastError().isEmpty()) {
        m_lastError = m_calculator->getLastError();
        return QJsonArray();
    }

    QJsonArray eclipseArray;
    for (const EclipseData &eclipse : eclipses) {
        QJsonObject obj;
        obj["date"] = eclipse.date.toString("yyyy-MM-dd");
        obj["time"] = eclipse.time.toString("HH:mm:ss");
        obj["julianDay"] = eclipse.julianDay;
        obj["type"] = eclipse.type;
        obj["latitude"] = eclipse.latitude;
        obj["longitude"] = eclipse.longitude;
        obj["magnitude"] = eclipse.magnitude;
        eclipseArray.append(obj);
    }
    return eclipseArray;
}

QJsonObject ChartDataManager::calculateSolarReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,

    const int year)
{
    // Clear any previous error
    m_lastError.clear();

    // Call the solar return calculation
    ChartData data = m_calculator->calculateSolarReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, year
        );

    // If there was an error, return an error object
    if (!m_calculator->getLastError().isEmpty()) {
        m_lastError = m_calculator->getLastError();
        return QJsonObject{{"error", m_lastError}};
    }

    // Convert the result to JSON
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateLunarReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    const QDate &targetDate
    )
{
    // Clear any previous error
    m_lastError.clear();

    // Call the lunar return calculation
    ChartData data = m_calculator->calculateLunarReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, targetDate
        );

    // If there was an error, return an error object
    if (!m_calculator->getLastError().isEmpty()) {
        m_lastError = m_calculator->getLastError();
        return QJsonObject{{"error", m_lastError}};
    }

    // Convert the result to JSON
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateSaturnReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber
    ) {
    m_lastError.clear();
    ChartData data = m_calculator->calculateSaturnReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );
    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateJupiterReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber
    ) {
    m_lastError.clear();
    ChartData data = m_calculator->calculateJupiterReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );
    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateVenusReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    m_lastError.clear();

    ChartData data = m_calculator->calculateVenusReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );

    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }

    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateMarsReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    m_lastError.clear();

    ChartData data = m_calculator->calculateMarsReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );

    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }

    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateMercuryReturnAsJson(
    const QDate &birthDate,
    const QTime &birthTime,
    const QString &utcOffset,
    const QString &latitude,
    const QString &longitude,
    const QString &houseSystem,
    int returnNumber)
{
    m_lastError.clear();

    ChartData data = m_calculator->calculateMercuryReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );

    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }

    return chartDataToJson(data);
}

// Uranus Neptune Pluto

QJsonObject ChartDataManager::calculateUranusReturnAsJson(const QDate &birthDate, const QTime &birthTime, const QString &utcOffset, const QString &latitude, const QString &longitude, const QString &houseSystem, int returnNumber)
{
    m_lastError.clear();
    ChartData data = m_calculator->calculateUranusReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );
    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculateNeptuneReturnAsJson(const QDate &birthDate, const QTime &birthTime, const QString &utcOffset, const QString &latitude, const QString &longitude, const QString &houseSystem, int returnNumber)
{
    m_lastError.clear();
    ChartData data = m_calculator->calculateNeptuneReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );
    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::calculatePlutoReturnAsJson(const QDate &birthDate, const QTime &birthTime, const QString &utcOffset, const QString &latitude, const QString &longitude, const QString &houseSystem, int returnNumber)
{
    m_lastError.clear();
    ChartData data = m_calculator->calculatePlutoReturn(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
        );
    if (!m_lastError.isEmpty()) {
        return QJsonObject();
    }
    return chartDataToJson(data);
}
