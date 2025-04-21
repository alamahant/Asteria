#ifndef CHARTRENDERER_H
#define CHARTRENDERER_H
#define DEFAULT_CHART_SIZE 700
#define DEFAULT_WHEEL_THICKNESS 30
#define PLANET_SIZE 35
#define POINT_SIZE 16

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMap>
#include <QColor>
#include "chartcalculator.h"


// Forward declarations
class QGraphicsEllipseItem;
class QGraphicsLineItem;
class QGraphicsTextItem;



// Custom graphics item for planets
class PlanetItem : public QGraphicsEllipseItem
{
public:
    PlanetItem(const QString &id, const QString &sign, double longitude,
               const QString &house, bool isRetrograde, QGraphicsItem *parent = nullptr);
    QString id() const { return m_id; }
    QString sign() const { return m_sign; }
    double longitude() const { return m_longitude; }
    QString house() const { return m_house; }
    // Override for hover events and tooltips
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void addAspect(const QString &otherPlanet, const QString &aspectType, double orb);

private:
    QString m_id;
    QString m_sign;
    double m_longitude;
    QString m_house;
    bool m_isRetrograde;
private:
    QString getPlanetSymbol(const QString &planetId) const;
    QStringList m_aspects; // Store aspect information
    void updateTooltip();

};

// Custom graphics item for aspects
class AspectItem : public QGraphicsLineItem
{
public:
    AspectItem(const QString &planet1, const QString &planet2,
               const QString &aspectType, double orb,
               QGraphicsItem *parent = nullptr);
    QString planet1() const { return m_planet1; }
    QString planet2() const { return m_planet2; }
    QString aspectType() const { return m_aspectType; }
    double orb() const { return m_orb; }
    // Override for hover events and tooltips
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    QString m_planet1;
    QString m_planet2;
    QString m_aspectType;
    double m_orb;
};

class ChartRenderer : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ChartRenderer(QWidget *parent = nullptr);
    ~ChartRenderer();
    // Set the chart data to render
    void setChartData(const ChartData &data);
    // Clear the chart
    void clearChart();
    // Render the chart
    void renderChart();
    // Customization options
    void setShowAspects(bool show);
    void setShowHouseCusps(bool show);
    void setShowPlanetSymbols(bool show);
    void setChartSize(int size);
    void drawHouseRing();

protected:
    // Override for zoom functionality
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // Helper methods for rendering
    void drawChartWheel();
    void drawZodiacSigns();
    void drawHouseCusps();
    void drawPlanets();
    void drawAspects();
    void drawAngles();

    // Helper for planet rendering
    void drawPlanet(const PlanetData &planet, double radius);

    // Helper for symbols
    QString getPlanetSymbol(const QString &planetId);
    QString getSignSymbol(const QString &signName);

    // Helper for coordinate conversion
    QPointF longitudeToPoint(double longitude, double radius);

    // Get color for aspect type
    QColor aspectColor(const QString &aspectType);

    // Get symbol for planet or sign
    QString planetSymbol(const QString &planetId);
    QString signSymbol(const QString &signName);

    QGraphicsScene *m_scene;
    ChartData m_chartData;

    // Chart elements
    QGraphicsEllipseItem *m_outerWheel;
    QGraphicsEllipseItem *m_innerWheel;
    QMap<QString, PlanetItem*> m_planetItems;
    QList<AspectItem*> m_aspectItems;
    QList<QGraphicsLineItem*> m_houseCuspLines;
    QList<QGraphicsTextItem*> m_signTexts;

    // Configuration
    bool m_showAspects;
    bool m_showHouseCusps;
    bool m_showPlanetSymbols;
    bool m_showPlanetLabels;
    int m_chartSize;
    double m_wheelThickness;

    // Constants
    //static const int DEFAULT_CHART_SIZE = 600;
    //static const int DEFAULT_WHEEL_THICKNESS = 10;
    bool isMajorAspect(const QString &aspectType);
    void updateSettings(bool showAspects, bool showHouseCusps, bool showPlanetSymbols, int chartSize);

};

#endif // CHARTRENDERER_H

