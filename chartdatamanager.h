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
};

#endif // CHARTDATAMANAGER_H
