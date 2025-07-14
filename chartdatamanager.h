#ifndef CHARTDATAMANAGER_H
#define CHARTDATAMANAGER_H

#include <QObject>
#include <QDate>
#include <QTime>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include "chartcalculator.h"

class ChartDataManager : public QObject
{
    Q_OBJECT

public:
    explicit ChartDataManager(QObject *parent = nullptr);
    ~ChartDataManager();



    // Check if the calculator is available
    bool isCalculatorAvailable() const;

    // Calculate chart and return structured data
    ChartData calculateChart(const QDate &birthDate,
                             const QTime &birthTime,
                             const QString &utcOffset,
                             const QString &latitude,
                             const QString &longitude,
                             const QString &houseSystem = "Placidus",
                             double orbMax = 8.0);

    // Calculate chart and return JSON data
    QJsonObject calculateChartAsJson(const QDate &birthDate,
                                     const QTime &birthTime,
                                     const QString &utcOffset,
                                     const QString &latitude,
                                     const QString &longitude,
                                     const QString &houseSystem = "Placidus",
                                     double orbMax = 8.0);

    // Convert ChartData to JSON
    QJsonObject chartDataToJson(const ChartData &data);

    // Get the last error message
    QString getLastError() const;

private:
    // Convert individual components to JSON
    QJsonArray planetsToJson(const QVector<PlanetData> &planets);
    QJsonArray housesToJson(const QVector<HouseData> &houses);
    QJsonArray anglesToJson(const QVector<AngleData> &angles);
    QJsonArray aspectsToJson(const QVector<AspectData> &aspects);

    ChartCalculator *m_calculator;
    QString m_lastError;

signals:
    void error(const QString &errorMessage);

public:
    QString calculateTransits(const QDate &birthDate,
                              const QTime &birthTime,
                              const QString &utcOffset,
                              const QString &latitude,
                              const QString &longitude,
                              const QDate &transitStartDate,
                              int numberOfDays);

    QJsonObject calculateTransitsAsJson(const QDate &birthDate,
                                        const QTime &birthTime,
                                        const QString &utcOffset,
                                        const QString &latitude,
                                        const QString &longitude,
                                        const QDate &transitStartDate,
                                        int numberOfDays);

    QJsonArray calculateEclipsesAsJson(
        const QDate &fromDate,
        const QDate &toDate,
        bool solarEclipses,
        bool lunarEclipses);

    QJsonObject calculateSolarReturnAsJson(const QDate &birthDate,
                                           const QTime &birthTime,
                                           const QString &utcOffset,
                                           const QString &latitude,
                                           const QString &longitude,
                                           const QString &houseSystem,

                                           const int year);

    QJsonObject calculateLunarReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        const QDate &targetDate
        );

    QJsonObject calculateSaturnReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber
        );
    QJsonObject calculateJupiterReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber
        );

    QJsonObject calculateVenusReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber);

    QJsonObject calculateMarsReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber);

    QJsonObject calculateMercuryReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber);

    QJsonObject calculateUranusReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber);

    QJsonObject calculateNeptuneReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber);

    QJsonObject calculatePlutoReturnAsJson(
        const QDate &birthDate,
        const QTime &birthTime,
        const QString &utcOffset,
        const QString &latitude,
        const QString &longitude,
        const QString &houseSystem,
        int returnNumber);

};

#endif // CHARTDATAMANAGER_H
