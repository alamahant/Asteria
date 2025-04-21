#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QJsonObject>
#include<QJsonDocument>
#include<QJsonArray>

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    // Set chart data from JSON and update display
    void setChartData(const QJsonObject &chartData);

    // Clear the chart display
    void clear();

    // Get current chart data
    QJsonObject chartData() const { return m_chartData; }

protected:
    // Override paint event to draw the chart
    void paintEvent(QPaintEvent *event) override;

private:
    // Draw zodiac wheel with houses
    void drawWheel(QPainter &painter);

    // Draw planets at their positions
    void drawPlanets(QPainter &painter);

    // Draw angles (Ascendant, MC, etc.)
    void drawAngles(QPainter &painter);

    // Draw aspect lines between planets
    void drawAspects(QPainter &painter);

    // Calculate point on wheel at given degree
    QPointF pointOnWheel(qreal degree, qreal radius);

    QJsonObject m_chartData;
    bool m_hasData;
signals:
};

#endif // CHARTWIDGET_H
