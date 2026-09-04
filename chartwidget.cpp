#include "chartwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QtMath>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget{parent}
    , m_hasData(false)
{

    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

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

    if (!m_hasData) {
        painter.drawText(rect(), Qt::AlignCenter, "No chart data available");
        return;
    }

    drawWheel(painter);
    drawAngles(painter);  // Added this line to draw angles
    drawPlanets(painter);
    drawAspects(painter);
}

void ChartWidget::drawWheel(QPainter &painter)
{
    QPointF center(width() / 2.0, height() / 2.0);

    qreal size = qMin(width(), height()) * 0.9;
    qreal outerRadius = size / 2.0;
    qreal innerRadius = outerRadius * 0.6; // Inner circle at 60% of outer

    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(Qt::transparent);
    painter.drawEllipse(center, outerRadius, outerRadius);

    painter.drawEllipse(center, innerRadius, innerRadius);

    for (int i = 0; i < 12; i++) {
        qreal angle = i * 30.0;
        QPointF outer = pointOnWheel(angle, outerRadius);
        QPointF inner = pointOnWheel(angle, innerRadius);

        painter.drawLine(inner, outer);

        qreal textAngle = angle + 15.0;
        QPointF textPos = pointOnWheel(textAngle, (outerRadius + innerRadius) / 2.0);

        QString symbols[] = {"♈", "♉", "♊", "♋", "♌", "♍", "♎", "♏", "♐", "♑", "♒", "♓"};

        painter.save();

        painter.translate(textPos);
        painter.rotate(textAngle - 90); // Adjust text orientation

        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);
        painter.drawText(QPointF(0, 0), symbols[i]);

        painter.restore();
    }

    if (m_chartData.contains("houses") && m_chartData["houses"].isArray()) {
        QJsonArray houses = m_chartData["houses"].toArray();

        for (int i = 0; i < houses.size(); i++) {
            QJsonObject house = houses[i].toObject();
            if (house.contains("longitude") && house.contains("id")) {
                qreal longitude = house["longitude"].toDouble();
                QString houseId = house["id"].toString();

                QPointF outer = pointOnWheel(longitude, outerRadius);
                QPointF inner = pointOnWheel(longitude, innerRadius * 0.8);

                painter.setPen(QPen(Qt::blue, 1.5, Qt::DashLine));
                painter.drawLine(inner, outer);

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

    QPointF center(width() / 2.0, height() / 2.0);

    qreal size = qMin(width(), height()) * 0.9;
    qreal planetRadius = size / 2.0 * 0.5; // Place planets at 50% of wheel radius

    QJsonArray planets = m_chartData["planets"].toArray();

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

            QPointF planetPos = pointOnWheel(longitude, planetRadius);

            painter.setPen(Qt::black);
            painter.setBrush(Qt::white);
            painter.drawEllipse(planetPos, 12, 12);

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

    QPointF center(width() / 2.0, height() / 2.0);

    qreal size = qMin(width(), height()) * 0.9;
    qreal planetRadius = size / 2.0 * 0.5; // Place planets at 50% of wheel radius

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
    QPointF center(width() / 2.0, height() / 2.0);

    qreal radians = (degree - 90) * M_PI / 180.0;

    qreal x = center.x() + radius * qCos(radians);
    qreal y = center.y() + radius * qSin(radians);

    return QPointF(x, y);
}

void ChartWidget::drawAngles(QPainter &painter)
{
    if (!m_chartData.contains("angles") || !m_chartData["angles"].isArray()) {
        return;
    }

    QPointF center(width() / 2.0, height() / 2.0);

    qreal size = qMin(width(), height()) * 0.9;
    qreal outerRadius = size / 2.0;
    qreal innerRadius = outerRadius * 0.6; // Inner circle at 60% of outer

    QJsonArray angles = m_chartData["angles"].toArray();

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

            if (!angleSymbols.contains(angleId)) {
                continue;
            }

            QString symbol = angleSymbols[angleId].first;
            QColor color = angleSymbols[angleId].second;

            QPointF outer = pointOnWheel(longitude, outerRadius);
            QPointF inner = pointOnWheel(longitude, innerRadius * 0.5);

            painter.setPen(QPen(color, 2.5, Qt::SolidLine));
            painter.drawLine(center, outer);

            painter.setPen(color);
            QFont font = painter.font();
            font.setBold(true);
            font.setPointSize(10);
            painter.setFont(font);

            QPointF labelPos = pointOnWheel(longitude, outerRadius * 1.05);

            painter.save();
            painter.translate(labelPos);

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
