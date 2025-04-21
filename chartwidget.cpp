#include "chartwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QtMath>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget{parent}
    , m_hasData(false)
{

    // Set background color
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // Set minimum size
    setMinimumSize(400, 400);
}

void ChartWidget::setChartData(const QJsonObject &chartData)
{
    m_chartData = chartData;
    m_hasData = !chartData.isEmpty();
    update(); // Trigger repaint
}

void ChartWidget::clear()
{
    m_chartData = QJsonObject();
    m_hasData = false;
    update(); // Trigger repaint
}

void ChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // If no chart data, draw placeholder
    if (!m_hasData) {
        painter.drawText(rect(), Qt::AlignCenter, "No chart data available");
        return;
    }

    // Draw chart components
    drawWheel(painter);
    drawAngles(painter);  // Added this line to draw angles
    drawPlanets(painter);
    drawAspects(painter);
}

void ChartWidget::drawWheel(QPainter &painter)
{
    // Center of the widget
    QPointF center(width() / 2.0, height() / 2.0);

    // Calculate wheel size (90% of the smallest dimension)
    qreal size = qMin(width(), height()) * 0.9;
    qreal outerRadius = size / 2.0;
    qreal innerRadius = outerRadius * 0.6; // Inner circle at 60% of outer

    // Draw outer circle (zodiac wheel)
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(Qt::transparent);
    painter.drawEllipse(center, outerRadius, outerRadius);

    // Draw inner circle
    painter.drawEllipse(center, innerRadius, innerRadius);

    // Draw zodiac divisions (12 segments of 30 degrees each)
    for (int i = 0; i < 12; i++) {
        qreal angle = i * 30.0;
        QPointF outer = pointOnWheel(angle, outerRadius);
        QPointF inner = pointOnWheel(angle, innerRadius);

        painter.drawLine(inner, outer);

        // Draw zodiac sign symbol at middle of segment
        qreal textAngle = angle + 15.0;
        QPointF textPos = pointOnWheel(textAngle, (outerRadius + innerRadius) / 2.0);

        // Zodiac symbols: ♈♉♊♋♌♍♎♏♐♑♒♓
        QString symbols[] = {"♈", "♉", "♊", "♋", "♌", "♍", "♎", "♏", "♐", "♑", "♒", "♓"};

        // Save current transform
        painter.save();

        // Move to text position and rotate text
        painter.translate(textPos);
        painter.rotate(textAngle - 90); // Adjust text orientation

        // Draw text
        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);
        painter.drawText(QPointF(0, 0), symbols[i]);

        // Restore transform
        painter.restore();
    }

    // Draw house cusps if available in chart data
    if (m_chartData.contains("houses") && m_chartData["houses"].isArray()) {
        QJsonArray houses = m_chartData["houses"].toArray();

        for (int i = 0; i < houses.size(); i++) {
            QJsonObject house = houses[i].toObject();
            if (house.contains("longitude") && house.contains("id")) {
                qreal longitude = house["longitude"].toDouble();
                QString houseId = house["id"].toString();

                QPointF outer = pointOnWheel(longitude, outerRadius);
                QPointF inner = pointOnWheel(longitude, innerRadius * 0.8);

                // Draw house cusp line
                painter.setPen(QPen(Qt::blue, 1.5, Qt::DashLine));
                painter.drawLine(inner, outer);

                // Draw house number
                painter.setPen(Qt::blue);
                QPointF textPos = pointOnWheel(longitude, innerRadius * 0.9);

                painter.save();
                painter.translate(textPos);
                painter.drawText(QRect(-10, -10, 20, 20), Qt::AlignCenter, houseId);
                painter.restore();
            }
        }
    }
}

void ChartWidget::drawPlanets(QPainter &painter)
{
    if (!m_chartData.contains("planets") || !m_chartData["planets"].isArray()) {
        return;
    }

    // Center of the widget
    QPointF center(width() / 2.0, height() / 2.0);

    // Calculate wheel size
    qreal size = qMin(width(), height()) * 0.9;
    qreal planetRadius = size / 2.0 * 0.5; // Place planets at 50% of wheel radius

    QJsonArray planets = m_chartData["planets"].toArray();

    // Planet symbols: ☉☽☿♀♂♃♄♅♆♇
    QMap<QString, QString> planetSymbols;
    planetSymbols["Sun"] = "☉";
    planetSymbols["Moon"] = "☽";
    planetSymbols["Mercury"] = "☿";
    planetSymbols["Venus"] = "♀";
    planetSymbols["Mars"] = "♂";
    planetSymbols["Jupiter"] = "♃";
    planetSymbols["Saturn"] = "♄";
    planetSymbols["Uranus"] = "♅";
    planetSymbols["Neptune"] = "♆";
    planetSymbols["Pluto"] = "♇";

    for (int i = 0; i < planets.size(); i++) {
        QJsonObject planet = planets[i].toObject();
        if (planet.contains("longitude") && planet.contains("id")) {
            qreal longitude = planet["longitude"].toDouble();
            QString planetId = planet["id"].toString();

            // Calculate planet position
            QPointF planetPos = pointOnWheel(longitude, planetRadius);

            // Draw planet symbol
            painter.setPen(Qt::black);
            painter.setBrush(Qt::white);
            painter.drawEllipse(planetPos, 12, 12);

            // Draw planet symbol
            painter.save();
            QFont font = painter.font();
            font.setPointSize(12);
            painter.setFont(font);

            QString symbol = planetSymbols.value(planetId, planetId.left(1));
            painter.drawText(QRectF(planetPos.x() - 10, planetPos.y() - 10, 20, 20),
                             Qt::AlignCenter, symbol);
            painter.restore();
        }
    }
}

void ChartWidget::drawAspects(QPainter &painter)
{
    if (!m_chartData.contains("aspects") || !m_chartData["aspects"].isArray() ||
        !m_chartData.contains("planets") || !m_chartData["planets"].isArray()) {
        return;
    }

    // Center of the widget
    QPointF center(width() / 2.0, height() / 2.0);

    // Calculate wheel size
    qreal size = qMin(width(), height()) * 0.9;
    qreal planetRadius = size / 2.0 * 0.5; // Place planets at 50% of wheel radius

    // Create map of planet positions
    QMap<QString, QPointF> planetPositions;
    QJsonArray planets = m_chartData["planets"].toArray();

    for (int i = 0; i < planets.size(); i++) {
        QJsonObject planet = planets[i].toObject();
        if (planet.contains("longitude") && planet.contains("id")) {
            qreal longitude = planet["longitude"].toDouble();
            QString planetId = planet["id"].toString();
            planetPositions[planetId] = pointOnWheel(longitude, planetRadius);
        }
    }

    // Draw aspect lines
    QJsonArray aspects = m_chartData["aspects"].toArray();

    for (int i = 0; i < aspects.size(); i++) {
        QJsonObject aspect = aspects[i].toObject();
        if (aspect.contains("planet1") && aspect.contains("planet2") && aspect.contains("aspectType")) {
            QString planet1 = aspect["planet1"].toString();
            QString planet2 = aspect["planet2"].toString();
            QString aspectType = aspect["aspectType"].toString();

            if (planetPositions.contains(planet1) && planetPositions.contains(planet2)) {
                QPointF pos1 = planetPositions[planet1];
                QPointF pos2 = planetPositions[planet2];

                // Set pen based on aspect type
                QPen aspectPen;
                if (aspectType == "Conjunction") {
                    aspectPen = QPen(Qt::red, 1);
                } else if (aspectType == "Opposition") {
                    aspectPen = QPen(Qt::red, 1, Qt::DashLine);
                } else if (aspectType == "Trine") {
                    aspectPen = QPen(Qt::green, 1);
                } else if (aspectType == "Square") {
                    aspectPen = QPen(Qt::blue, 1);
                } else if (aspectType == "Sextile") {
                    aspectPen = QPen(Qt::cyan, 1);
                } else {
                    aspectPen = QPen(Qt::gray, 1, Qt::DotLine);
                }

                painter.setPen(aspectPen);
                painter.drawLine(pos1, pos2);
            }
        }
    }
}

QPointF ChartWidget::pointOnWheel(qreal degree, qreal radius)
{
    // Center of the widget
    QPointF center(width() / 2.0, height() / 2.0);

    // Convert degrees to radians
    // Note: In astrology charts, 0° is at the 9 o'clock position (East)
    // and degrees increase counterclockwise
    qreal radians = (degree - 90) * M_PI / 180.0;

    // Calculate point on circle
    qreal x = center.x() + radius * qCos(radians);
    qreal y = center.y() + radius * qSin(radians);

    return QPointF(x, y);
}

// Add this new method to draw angles
void ChartWidget::drawAngles(QPainter &painter)
{
    if (!m_chartData.contains("angles") || !m_chartData["angles"].isArray()) {
        return;
    }

    // Center of the widget
    QPointF center(width() / 2.0, height() / 2.0);

    // Calculate wheel size
    qreal size = qMin(width(), height()) * 0.9;
    qreal outerRadius = size / 2.0;
    qreal innerRadius = outerRadius * 0.6; // Inner circle at 60% of outer

    QJsonArray angles = m_chartData["angles"].toArray();

    // Angle symbols and colors
    QMap<QString, QPair<QString, QColor>> angleSymbols;
    angleSymbols["Asc"] = qMakePair("Asc", QColor(255, 0, 0));      // Ascendant - Red
    angleSymbols["MC"] = qMakePair("MC", QColor(0, 0, 255));        // Midheaven - Blue
    angleSymbols["Dsc"] = qMakePair("Dsc", QColor(255, 0, 0, 128)); // Descendant - Transparent Red
    angleSymbols["IC"] = qMakePair("IC", QColor(0, 0, 255, 128));   // Imum Coeli - Transparent Blue

    for (int i = 0; i < angles.size(); i++) {
        QJsonObject angle = angles[i].toObject();
        if (angle.contains("longitude") && angle.contains("id")) {
            qreal longitude = angle["longitude"].toDouble();
            QString angleId = angle["id"].toString();

            // Only draw the main angles
            if (!angleSymbols.contains(angleId)) {
                continue;
            }

            // Get symbol and color for this angle
            QString symbol = angleSymbols[angleId].first;
            QColor color = angleSymbols[angleId].second;

            // Calculate angle position
            QPointF outer = pointOnWheel(longitude, outerRadius);
            QPointF inner = pointOnWheel(longitude, innerRadius * 0.5);

            // Draw angle line
            painter.setPen(QPen(color, 2.5, Qt::SolidLine));
            painter.drawLine(center, outer);

            // Draw angle label
            painter.setPen(color);
            QFont font = painter.font();
            font.setBold(true);
            font.setPointSize(10);
            painter.setFont(font);

            // Position for label - slightly outside the wheel
            QPointF labelPos = pointOnWheel(longitude, outerRadius * 1.05);

            painter.save();
            painter.translate(labelPos);

            // Adjust text orientation based on position in the wheel
            qreal textAngle = longitude;
            if (textAngle > 90 && textAngle < 270) {
                textAngle += 180; // Flip text on bottom half of wheel
            }
            painter.rotate(textAngle - 90);

            painter.drawText(QRect(-30, -15, 60, 30), Qt::AlignCenter, symbol);
            painter.restore();
        }
    }
}
