#ifndef CHARTCALCULATOR_H
#define CHARTCALCULATOR_H

#include <QObject>
#include <QDate>
#include <QTime>
#include <QString>
#include <QVector>

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

class ChartCalculator : public QObject
{
    Q_OBJECT

public:
    explicit ChartCalculator(QObject *parent = nullptr);

    // Set the path to the Python virtual environment
    void setPythonVenvPath(const QString &path);

    // Calculate chart with the given birth data
    ChartData calculateChart(const QDate &birthDate,
                             const QTime &birthTime,
                             const QString &utcOffset,
                             const QString &latitude,
                             const QString &longitude,
                             const QString &houseSystem = "Placidus",
                             double orbMax = 8.0);

    // Check if the Python environment is properly set up
    bool isAvailable() const;

    // Get the last error message
    QString getLastError() const;

private:
    // Parse the output from the Python script
    ChartData parseOutput(const QString &output);

    QString m_pythonVenvPath;
    QString m_lastError;
public:
    QString calculateTransits(const QDate &birthDate,
                              const QTime &birthTime,
                              const QString &utcOffset,
                              const QString &latitude,
                              const QString &longitude,
                              const QDate &transitStartDate,
                              int numberOfDays);

};

#endif // CHARTCALCULATOR_H
