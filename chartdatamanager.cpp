#include "chartdatamanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ChartDataManager::ChartDataManager(QObject *parent)
    : QObject(parent)
    , m_calculator(new ChartCalculator(this))
{
}

ChartDataManager::~ChartDataManager()
{
    // QObject parent-child relationship will handle deletion
}

void ChartDataManager::setPythonVenvPath(const QString &path)
{
    m_calculator->setPythonVenvPath(path);
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

    // Calculate the chart
    ChartData data = m_calculator->calculateChart(birthDate, birthTime, utcOffset,
                                                  latitude, longitude, houseSystem, orbMax);

    // Check for errors
    if (!m_calculator->getLastError().isEmpty()) {
        m_lastError = m_calculator->getLastError();
        qDebug() << "Chart calculation error:" << m_lastError;
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
    ChartData data = calculateChart(birthDate, birthTime, utcOffset,
                                    latitude, longitude, houseSystem, orbMax);

    // If there was an error, return an empty object
    if (!m_lastError.isEmpty()) {
        return QJsonObject{{"error", m_lastError}};
    }

    // Convert to JSON
    return chartDataToJson(data);
}

QJsonObject ChartDataManager::chartDataToJson(const ChartData &data)
{
    QJsonObject json;

    // Add each component to the JSON object
    json["planets"] = planetsToJson(data.planets);
    json["houses"] = housesToJson(data.houses);
    json["angles"] = anglesToJson(data.angles);
    json["aspects"] = aspectsToJson(data.aspects);

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
        qDebug() << "Transit calculation error:" << m_lastError;
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

    return json;
}
