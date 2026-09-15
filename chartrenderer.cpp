#include "chartrenderer.h"
#include <QPainter>
#include <QWheelEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneHoverEvent>
#include <QToolTip>
#include <QtMath>
#include <QDebug>
#include "Globals.h"

extern QString g_astroFontFamily;

PlanetItem::PlanetItem(const QString &id, const QString &sign, double longitude,
                       const QString &house, bool isRetrograde = false, QGraphicsItem *parent)
    : QGraphicsEllipseItem(0, 0, AsteriaFlags::planetSize, AsteriaFlags::planetSize, parent)
    , m_id(id)
    , m_sign(sign)
    , m_longitude(longitude)
    , m_house(house)
    ,m_isRetrograde(isRetrograde)
{

    setAcceptHoverEvents(true);
    setBrush(QBrush(Qt::white));
    setPen(QPen(Qt::black, 1));


    setZValue(10); // Ensure planets are always on top

    updateTooltip();
}



void PlanetItem::addAspect(const QString &otherPlanet, const QString &aspectType, double orb) {
    m_aspects.append(QString("%1 %2 (Orb: %3°)")
                         .arg(aspectType)
                         .arg(otherPlanet)
                         .arg(orb, 0, 'f', 1)); // Format orb to 1 decimal place
    updateTooltip();
}



void PlanetItem::updateTooltip() {
    bool isNode = (m_id == "North Node" || m_id == "South Node");

    /*
    QString tooltip = QString("%1 in %2 at %3°%4 in %5")
                          .arg(m_id)
                          .arg(m_sign)
                          .arg(m_longitude, 0, 'f', 2)
                          .arg((m_isRetrograde && !isNode) ? " ℞" : "")
                          .arg(m_house);
    */

    QString tooltip = QString("%1 in %2%3 in %4")
                          .arg(m_id)
                          .arg(m_sign)  // Already contains "Libra 23.4°"
                          .arg((m_isRetrograde && !isNode) ? " ℞" : "")
                          .arg(m_house);

    if (!m_aspects.isEmpty()) {
        tooltip += "\n\nAspects:";
        for (const QString &aspect : m_aspects) {
            tooltip += "\n• " + aspect;
        }
    }

    setToolTip(tooltip);
}

void PlanetItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_isRetrograde && m_id != "North Node" && m_id != "South Node") {
        setBrush(QBrush(QColor(255, 100, 100))); // Light red
    } else {
        setBrush(QBrush(Qt::white)); // Default color
    }
    QGraphicsEllipseItem::paint(painter, option, widget);

    QFont planetFont;

    if (!g_astroFontFamily.isEmpty()) {

        planetFont = QFont(g_astroFontFamily, AsteriaFlags::pointSize);
    } else {

        planetFont = QFont();
        planetFont.setPointSize(AsteriaFlags::pointSize);
        planetFont.setBold(true);
    }

    painter->setFont(planetFont);
    QString symbol = getPlanetSymbol(m_id);
    painter->drawText(boundingRect(), Qt::AlignCenter, symbol);
}


QString PlanetItem::getPlanetSymbol(const QString &planetId) const
{

    static QMap<QString, QString> symbols = {
        {"Sun", "☉"},
        {"Moon", "☽"},
        {"Mercury", "☿"},
        {"Venus", "♀"},
        {"Mars", "♂"},
        {"Jupiter", "♃"},
        {"Saturn", "♄"},
        {"Uranus", "♅"},
        {"Neptune", "♆"},
        {"Pluto", "♇"},
        {"Chiron", "⚷"},
        {"North Node", "☊"},
        {"South Node", "☋"},
        {"Pars Fortuna", "⊕"}, // Part of Fortune symbol (circle with plus)
        {"Syzygy", "☍"},        // Using opposition symbol for Syzygy
        {"Lilith", "⚸"},       // Black Moon Lilith symbol
        {"Ceres", "⚳"},        // Ceres symbol
        {"Pallas", "⚴"},       // Pallas symbol
        {"Juno", "⚵"},         // Juno symbol
        {"Vesta", "⚶"},        // Vesta symbol
        {"Vertex", "⊗"},       // Using a cross in circle for Vertex
        {"East Point", "⊙"},   // Using a dot in circle for East Point
        {"Part of Spirit", "⊖"} // Part of Spirit (circle with minus)
    };

    return symbols.value(planetId, planetId);
}

AspectItem::AspectItem(const QString &planet1, const QString &planet2,
                       const QString &aspectType, double orb,
                       QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
    , m_planet1(planet1)
    , m_planet2(planet2)
    , m_aspectType(aspectType)
    , m_orb(orb)
{
    setAcceptHoverEvents(false);
}

void AspectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsLineItem::paint(painter, option, widget);
}

ChartRenderer::ChartRenderer(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_outerWheel(nullptr)
    , m_innerWheel(nullptr)
    , m_showAspects(true)
    , m_showHouseCusps(true)
    , m_showPlanetSymbols(true)
    , m_showPlanetLabels(true)
    , m_chartSize(AsteriaFlags::chartSize)
    , m_wheelThickness(AsteriaFlags::wheelThickness)
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    m_scene->setSceneRect(-m_chartSize/2, -m_chartSize/2, m_chartSize, m_chartSize);
    centerOn(0, 0);

}

ChartRenderer::~ChartRenderer()
{
    clearChart();
}

void ChartRenderer::setChartData(const ChartData &data)
{
    m_chartData = data;
}

void ChartRenderer::clearChart()
{
    m_scene->clear();

    m_planetItems.clear();

    m_aspectItems.clear();
    m_houseCuspLines.clear();
    m_signTexts.clear();
    m_outerWheel = nullptr;
    m_innerWheel = nullptr;

}

void ChartRenderer::renderChart()
{
    clearChart();
    if (m_chartData.planets.isEmpty()) {
        return;
    }

    drawChartWheel();
    drawAngles();

    drawZodiacSigns();
    if (m_showHouseCusps) {
        drawHouseCusps();
        drawHouseRing(); // Added this line to draw the house ring
    }
    drawPlanets();

    if (m_showAspects) {
        drawAspects();
    }
    centerOn(0, 0);
}

void ChartRenderer::setShowAspects(bool show)
{
    m_showAspects = show;
}

void ChartRenderer::setShowHouseCusps(bool show)
{
    m_showHouseCusps = show;
}

void ChartRenderer::setShowPlanetSymbols(bool show)
{
    m_showPlanetSymbols = show;
}

void ChartRenderer::setChartSize(int size)
{
    m_chartSize = size;
    m_scene->setSceneRect(-m_chartSize/2, -m_chartSize/2, m_chartSize, m_chartSize);
}

void ChartRenderer::wheelEvent(QWheelEvent *event)
{
    double scaleFactor = 1.15;
    if (event->angleDelta().y() < 0) {
        scaleFactor = 1.0 / scaleFactor;
    }
    scale(scaleFactor, scaleFactor);
}

void ChartRenderer::resizeEvent(QResizeEvent *event)
{


}

void ChartRenderer::drawChartWheel(){
    double outerRadius = m_chartSize / 2.0;
    double innerRadius = outerRadius - m_wheelThickness;

    m_outerWheel = new QGraphicsEllipseItem(-outerRadius, -outerRadius,
                                            outerRadius * 2, outerRadius * 2);
    m_outerWheel->setPen(QPen(Qt::black, 2));
    m_outerWheel->setBrush(Qt::transparent);
    m_outerWheel->setZValue(1);
    m_scene->addItem(m_outerWheel);

    m_innerWheel = new QGraphicsEllipseItem(-innerRadius, -innerRadius,
                                            innerRadius * 2, innerRadius * 2);
    m_innerWheel->setPen(QPen(Qt::black, 1));
    m_innerWheel->setBrush(Qt::transparent);
    m_innerWheel->setZValue(1);
    m_scene->addItem(m_innerWheel);

    double padding = outerRadius * 0.15; // 15% padding

    QRectF sceneRect(-outerRadius - padding, -outerRadius - padding,
                     (outerRadius + padding) * 2, (outerRadius + padding) * 2);

    m_scene->setSceneRect(sceneRect);
}

void ChartRenderer::drawZodiacSigns()
{
    double outerRadius = m_chartSize / 2.0;
    double innerRadius = outerRadius - m_wheelThickness;
    double textRadius = (outerRadius + innerRadius) / 2.0;

    QMap<QString, QString> signNames = {
        {"♈", "Aries"},
        {"♉", "Taurus"},
        {"♊", "Gemini"},
        {"♋", "Cancer"},
        {"♌", "Leo"},
        {"♍", "Virgo"},
        {"♎", "Libra"},
        {"♏", "Scorpio"},
        {"♐", "Sagittarius"},
        {"♑", "Capricorn"},
        {"♒", "Aquarius"},
        {"♓", "Pisces"}
    };

    QMap<QString, QColor> signColors = {
        {"Aries", QColor(255, 200, 200)},      // Fire
        {"Leo", QColor(255, 200, 200)},        // Fire
        {"Sagittarius", QColor(255, 200, 200)},// Fire
        {"Taurus", QColor(255, 255, 200)},     // Earth
        {"Virgo", QColor(255, 255, 200)},      // Earth
        {"Capricorn", QColor(255, 255, 200)},  // Earth
        {"Gemini", QColor(200, 255, 200)},     // Air
        {"Libra", QColor(200, 255, 200)},      // Air
        {"Aquarius", QColor(200, 255, 200)},   // Air
        {"Cancer", QColor(200, 200, 255)},     // Water
        {"Scorpio", QColor(200, 200, 255)},    // Water
        {"Pisces", QColor(200, 200, 255)}      // Water
    };

    QStringList signs = {"♈", "♉", "♊", "♋", "♌", "♍", "♎", "♏", "♐", "♑", "♒", "♓"};




    double refAsc = (!m_chartData.houses.isEmpty() ? m_chartData.houses[0].longitude : getAscendantLongitude()); // prefer House 1 cusp
    double startAngle = 180.0 - refAsc; // Aries starts rotated by Asc/House1
    for (int i = 0; i < 12; i++) {

        double startSignAngle = startAngle + (i * 30.0); // Add instead of subtract
        double endSignAngle = startSignAngle + 30.0; // Add instead of subtract
        QPainterPath path;
        path.moveTo(0, 0);
        /*
        path.arcTo(-outerRadius, -outerRadius, outerRadius * 2, outerRadius * 2,
                   startSignAngle, -30.0);
        path.arcTo(-innerRadius, -innerRadius, innerRadius * 2, innerRadius * 2,
                   endSignAngle, 30.0);
        */
        path.arcTo(-outerRadius, -outerRadius, outerRadius * 2, outerRadius * 2,
                   startSignAngle, 30.0); // Positive angle (counterclockwise)
        path.arcTo(-innerRadius, -innerRadius, innerRadius * 2, innerRadius * 2,
                   endSignAngle, -30.0); // Negative angle (clockwise)
        path.closeSubpath();

        QGraphicsPathItem *segment = new QGraphicsPathItem(path);

        QString signName = signNames[signs[i]];
        segment->setBrush(QBrush(signColors[signName]));
        segment->setPen(QPen(Qt::black, 0.25));

        segment->setToolTip(signName);

        segment->setAcceptHoverEvents(true);

        m_scene->addItem(segment);

        double textAngle = startSignAngle + 15.0; // Add instead of subtract


        double textRadians = qDegreesToRadians(textAngle);

        double x = textRadius * qCos(textRadians);
        double y = -textRadius * qSin(textRadians); // Negative because Y increases downward in Qt

        QGraphicsTextItem *signText = new QGraphicsTextItem(signs[i]);


#ifdef Q_OS_WIN
    QFont font(g_astroFontFamily.isEmpty() ? "DejaVu Sans" : g_astroFontFamily,
               AsteriaFlags::uiFontSize + 2);
#else
    QFont font("DejaVu Sans", AsteriaFlags::uiFontSize + 2);      // use a known system font
    font.setStyleStrategy(QFont::NoFontMerging); // block emoji/color fallback on Linux
#endif

        signText->setFont(font);

        QRectF textRect = signText->boundingRect();
        signText->setPos(x - textRect.width()/2, y - textRect.height()/2);

        m_scene->addItem(signText);
        m_signTexts.append(signText);

        double lineRadians = qDegreesToRadians(startSignAngle);
        double x1 = innerRadius * qCos(lineRadians);
        double y1 = -innerRadius * qSin(lineRadians);
        double x2 = outerRadius * qCos(lineRadians);
        double y2 = -outerRadius * qSin(lineRadians);
        QGraphicsLineItem *line = new QGraphicsLineItem(x1, y1, x2, y2);
        line->setPen(QPen(Qt::black, 1));


        m_scene->addItem(line);
    }
}

void ChartRenderer::drawHouseCusps(){
    double outerRadius = m_chartSize / 2.0;
    double innerRadius = outerRadius - m_wheelThickness;
    for (const HouseData &house : m_chartData.houses) {
        double longitude = house.longitude;
        QPointF outerPoint = longitudeToPoint(longitude, outerRadius);
        QPointF centerPoint = QPointF(0, 0);
        QGraphicsLineItem *line = m_scene->addLine(QLineF(centerPoint, outerPoint));
        line->setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
        line->setZValue(-1);      // Below planets

        line->setToolTip(QString("House %1 cusp: %2° %3")
                             .arg(house.id.mid(5))
                             .arg(longitude)
                             .arg(house.sign));

        line->setAcceptHoverEvents(true);
        line->setCursor(Qt::PointingHandCursor); // Optional: changes cursor on hover

        QGraphicsLineItem *hitArea = m_scene->addLine(QLineF(centerPoint, outerPoint));
        hitArea->setPen(QPen(Qt::transparent, 20)); // Invisible but wide pen
        hitArea->setZValue(-2);      // Below planets

        hitArea->setToolTip(line->toolTip()); // Same tooltip
        hitArea->setAcceptHoverEvents(true);
        m_houseCuspLines.append(line);
    }
}


void ChartRenderer::drawAspects() {
    bool showAspectsLines = AspectSettings::instance().getShowAspectLines();
    if (!showAspectsLines) return;
    QMap<QString, QList<AspectData>> planetAspects;

    for (const AspectData &aspect : m_chartData.aspects) {

        AspectData reversedAspect;
        reversedAspect.planet1 = aspect.planet2;
        reversedAspect.planet2 = aspect.planet1;
        reversedAspect.aspectType = aspect.aspectType;
        reversedAspect.orb = aspect.orb;

        planetAspects[aspect.planet1].append(aspect);

        planetAspects[aspect.planet2].append(reversedAspect);
    }

    for (auto it = planetAspects.begin(); it != planetAspects.end(); ++it) {
        QString planetId = it.key();
        QList<AspectData> aspects = it.value();

        if (m_planetItems.contains(planetId)) {
            PlanetItem *planetItem = m_planetItems[planetId];

            QString baseTooltip = planetItem->toolTip(); // Get existing planet info tooltip
            QString aspectText = "\n\nAspects:";

            std::sort(aspects.begin(), aspects.end(),
                      [this](const AspectData &a, const AspectData &b) {
                          bool aMajor = isMajorAspect(a.aspectType);
                          bool bMajor = isMajorAspect(b.aspectType);

                          if (aMajor != bMajor) {
                              return aMajor > bMajor; // Major aspects first
                          }
                          return a.orb < b.orb; // Then by orb (smaller orb = stronger aspect)
                      });

            for (const AspectData &aspect : aspects) {
                aspectText += QString("\n• %1 %2 (Orb: %3°)")
                                  .arg(aspect.aspectType)
                                  .arg(aspect.planet2) // The other planet
                                  .arg(aspect.orb, 0, 'f', 1);
            }

            planetItem->setToolTip(baseTooltip + aspectText);
        }
    }

    for (const AspectData &aspect : m_chartData.aspects) {
        if (!m_planetItems.contains(aspect.planet1) || !m_planetItems.contains(aspect.planet2)) {

            continue; // Skip if either planet is not found
        }

        PlanetItem *planet1Item = m_planetItems[aspect.planet1];
        PlanetItem *planet2Item = m_planetItems[aspect.planet2];

        QPointF p1Center = planet1Item->pos() + QPointF(AsteriaFlags::planetSize/2, AsteriaFlags::planetSize/2);
        QPointF p2Center = planet2Item->pos() + QPointF(AsteriaFlags::planetSize/2, AsteriaFlags::planetSize/2);

        QLineF centerLine(p1Center, p2Center);
        double angle = centerLine.angle() * M_PI / 180.0; // Convert to radians

        double planetRadius = AsteriaFlags::planetSize / 2.0;

        QPointF p1Periphery(
            p1Center.x() + planetRadius * cos(angle),
            p1Center.y() - planetRadius * sin(angle)
            );

        QPointF p2Periphery(
            p2Center.x() - planetRadius * cos(angle),
            p2Center.y() + planetRadius * sin(angle)
            );

        AspectItem *aspectLine = new AspectItem(aspect.planet1, aspect.planet2,
                                                aspect.aspectType, aspect.orb);
        aspectLine->setLine(QLineF(p1Periphery, p2Periphery));
        QPen pen(aspectColor(aspect.aspectType), 1);

        if (isMajorAspect(aspect.aspectType)) {
            pen.setStyle(AspectSettings::instance().getMajorAspectStyle());
            pen.setWidthF(AspectSettings::instance().getMajorAspectWidth());
        } else {
            pen.setStyle(AspectSettings::instance().getMinorAspectStyle());
            pen.setWidthF(AspectSettings::instance().getMinorAspectWidth());
        }

        aspectLine->setPen(pen);
        aspectLine->setZValue(-5); // Draw behind planets

        aspectLine->setAcceptHoverEvents(false);

        m_scene->addItem(aspectLine);
        m_aspectItems.append(aspectLine);
    }
}

void ChartRenderer::drawAngles() {

    double testRadius = m_chartSize / 2.0;


    double outerRadius = m_chartSize / 2.0;
    double houseRingOuterRadius = outerRadius; // House ring is at the outer edge
    double zodiacOuterRadius = houseRingOuterRadius - AsteriaFlags::wheelThickness;
    double zodiacInnerRadius = zodiacOuterRadius - m_wheelThickness;

    double labelRadius = houseRingOuterRadius - (AsteriaFlags::wheelThickness * 0.5) + 70;


    QMap<QString, QPointF> anglePoints;

    QMap<QString, QString> displayNames;
    displayNames["Asc"] = "AC";
    displayNames["Desc"] = "DC";
    displayNames["MC"] = "MC";
    displayNames["IC"] = "IC";

    for (const AngleData &angle : m_chartData.angles) {

        double longitude = angle.longitude;
        if (m_chartData.houses.size() == 12) {
            if (angle.id == "Asc")      longitude = m_chartData.houses[0].longitude;  // House 1 cusp
            else if (angle.id == "Desc") longitude = m_chartData.houses[6].longitude;  // House 7 cusp
            else if (angle.id == "MC")   longitude = m_chartData.houses[9].longitude;  // House 10 cusp
            else if (angle.id == "IC")   longitude = m_chartData.houses[3].longitude;  // House 4 cusp
        }
        QPointF outerPoint = longitudeToPoint(longitude, outerRadius);
        QPointF centerPoint = QPointF(0, 0);

        anglePoints[angle.id] = outerPoint;

        QGraphicsLineItem *line = m_scene->addLine(QLineF(centerPoint, outerPoint));

        QPen pen(Qt::red, 1);
        if (angle.id == "Asc") {
            pen.setColor(Qt::red);
        } else if (angle.id == "MC") {
            pen.setColor(Qt::blue);
        } else if (angle.id == "Desc") {
            pen.setColor(Qt::darkRed);
        } else if (angle.id == "IC") {
            pen.setColor(Qt::darkBlue);
        }
        line->setPen(pen);

        /*
        QString tooltipText = QString("%1 (%2): %3° %4")

                                  .arg(angle.id)
                                  .arg(displayNames[angle.id])
                                  .arg(longitude, 0, 'f', 2)
                                  .arg(angle.sign);

        */

        QString tooltipText = QString("%1 (%2): %3")
                                  .arg(angle.id)
                                  .arg(displayNames[angle.id])
                                  .arg(angle.sign);  // Just show "Asc (AC): Libra 28°36'"



        line->setToolTip(tooltipText);
        line->setAcceptHoverEvents(true);
        line->setCursor(Qt::PointingHandCursor); // Changes cursor on hover

        QGraphicsLineItem *hitArea = m_scene->addLine(QLineF(centerPoint, outerPoint));
        hitArea->setPen(QPen(Qt::transparent, 20)); // Invisible but wide pen
        hitArea->setToolTip(tooltipText); // Same tooltip
        hitArea->setAcceptHoverEvents(true);
        hitArea->setZValue(-2); // Below the visible line but still detectable

        line->setZValue(-1); // Above the hit area but below planets

        QPointF textPos = longitudeToPoint(longitude, labelRadius);
        QGraphicsTextItem *textItem = m_scene->addText(displayNames[angle.id]);
        textItem->setDefaultTextColor(pen.color());

        QFont font = textItem->font();
        font.setBold(true);
        textItem->setFont(font);

        QRectF textRect = textItem->boundingRect();
        textItem->setPos(textPos.x() - textRect.width()/2,
                         textPos.y() - textRect.height()/2);

        textItem->setToolTip(tooltipText);
    }

    if (anglePoints.contains("Asc") && anglePoints.contains("Desc")) {
        QGraphicsLineItem *ascDescAxis = m_scene->addLine(
            QLineF(anglePoints["Asc"], anglePoints["Desc"]));
        QPen axisPen(Qt::transparent, 0); // Transparent pen with zero width

        ascDescAxis->setPen(axisPen);
        ascDescAxis->setZValue(-1); // Draw behind other elements
    }

    if (anglePoints.contains("MC") && anglePoints.contains("IC")) {
        QGraphicsLineItem *mcIcAxis = m_scene->addLine(
            QLineF(anglePoints["MC"], anglePoints["IC"]));
        QPen axisPen(Qt::transparent, 0); // Transparent pen with zero width

        mcIcAxis->setPen(axisPen);
        mcIcAxis->setZValue(-1); // Draw behind other elements
    }
}


QPointF ChartRenderer::longitudeToPoint(double longitude, double radius){
    /* Traditional astrological chart orientation:
 * - Degrees increase counterclockwise (following the natural motion of planets)
 * - 0° Aries is typically at the 9 o'clock position (left)
 * - Formula: double angleRadians = qDegreesToRadians(270 - longitude);
 *
 * Current implementation:
 * - Degrees increase clockwise
 * - 0° Aries is at the 3 o'clock position (right)
 * - Formula: double angleRadians = qDegreesToRadians(90 - longitude);
 *
 * Alternative counterclockwise with 0° at right:
 * - Degrees increase counterclockwise
 * - 0° Aries is at the 3 o'clock position (right)
 * - Formula: double angleRadians = qDegreesToRadians(450 - longitude);
 *   or equivalently: double angleRadians = qDegreesToRadians(90 - longitude);
 *   with: double y = radius * qSin(angleRadians); // No negative sign
 *
 * The choice of orientation doesn't affect the underlying astronomical data,
 * only how it's visually presented on the chart.
 */


    double refAsc = (!m_chartData.houses.isEmpty() ? m_chartData.houses[0].longitude : getAscendantLongitude());
    double angleDeg = 180.0 + (longitude - refAsc);
    double angleRadians = qDegreesToRadians(angleDeg);

    double x = radius * qCos(angleRadians);
    double y = -radius * qSin(angleRadians);
    return QPointF(x, y);
}


QColor ChartRenderer::aspectColor(const QString &aspectType) {
    if (aspectType == "CON") return QColor(128, 128, 128);       // Conjunction - Neutral Gray
    if (aspectType == "OPP") return QColor(220, 20, 60);         // Opposition - Crimson
    if (aspectType == "SQR") return QColor(255, 69, 0);          // Square - Fiery Red-Orange
    if (aspectType == "TRI") return QColor(30, 144, 255);        // Trine - Dodger Blue
    if (aspectType == "SEX") return QColor(0, 206, 209);         // Sextile - Turquoise
    if (aspectType == "QUI") return QColor(138, 43, 226);        // Quincunx - Blue Violet
    if (aspectType == "SSQ") return QColor(255, 165, 0);         // Semi-square - Orange
    if (aspectType == "SSX") return QColor(0, 128, 0);           // Semi-sextile - Classic Green
    if (aspectType == "SQQ") return QColor(255, 105, 180); // Sesquiquadrate - Pink



    return QColor(105, 105, 105); // Default - Dim Gray
}



bool ChartRenderer::isMajorAspect(const QString &aspectType) {
    return (aspectType == "CON" ||
            aspectType == "OPP" ||
            aspectType == "SQR" ||
            aspectType == "TRI" ||
            aspectType == "SEX");
}

QString ChartRenderer::signSymbol(const QString &signName){
    if (signName == "Aries") return "♈";
    if (signName == "Taurus") return "♉";
    if (signName == "Gemini") return "♊";
    if (signName == "Cancer") return "♋";
    if (signName == "Leo") return "♌";
    if (signName == "Virgo") return "♍";
    if (signName == "Libra") return "♎";
    if (signName == "Scorpio") return "♏";
    if (signName == "Sagittarius") return "♐";
    if (signName == "Capricorn") return "♑";
    if (signName == "Aquarius") return "♒";
    if (signName == "Pisces") return "♓";
    return signName.left(3);
}

void ChartRenderer::drawPlanets() {
    if (m_chartData.planets.isEmpty()) {
        return;
    }

    double chartRadius = m_chartSize / 2.0;
    double baseRadius = chartRadius - m_wheelThickness - 35; // Default radius for planets

    double planetSize = AsteriaFlags::planetSize;
    double minDistance = planetSize * 1.2; // 20% buffer for spacing

    QList<PlanetData> sortedPlanets = m_chartData.planets;
    std::sort(sortedPlanets.begin(), sortedPlanets.end(),
              [](const PlanetData &a, const PlanetData &b) {
                  return a.longitude < b.longitude;
              });

    struct PlanetPosition {
        PlanetData planet;
        double radius;
        QPointF position;
    };

    QList<PlanetPosition> planetPositions;

    for (const PlanetData &planet : sortedPlanets) {
        PlanetPosition pos;
        pos.planet = planet;
        pos.radius = baseRadius;

        pos.position = longitudeToPoint(planet.longitude, pos.radius);

        planetPositions.append(pos);
    }

    bool hasCollisions = true;
    int iterations = 0;
    int maxIterations = 50; // Prevent infinite loops

    while (hasCollisions && iterations < maxIterations) {
        hasCollisions = false;
        iterations++;

        for (int i = 0; i < planetPositions.size(); i++) {
            for (int j = i + 1; j < planetPositions.size(); j++) {
                QPointF diff = planetPositions[i].position - planetPositions[j].position;
                double distance = qSqrt(diff.x() * diff.x() + diff.y() * diff.y());

                if (distance < minDistance) {
                    hasCollisions = true;

                    planetPositions[j].radius -= minDistance / 2;

                    planetPositions[j].position = longitudeToPoint(planetPositions[j].planet.longitude, planetPositions[j].radius);
                }
            }
        }
    }

    for (const PlanetPosition &pos : planetPositions) {
        drawPlanet(pos.planet, pos.radius);

        if (pos.radius < baseRadius) {
            QPointF actualPoint = longitudeToPoint(pos.planet.longitude, baseRadius);

            QGraphicsLineItem *line = m_scene->addLine(
                QLineF(actualPoint, pos.position)
                );
            line->setPen(QPen(Qt::gray, 0.5, Qt::DotLine));
        }
    }
}



void ChartRenderer::drawPlanet(const PlanetData &planet, double radius) {



    QPointF p = longitudeToPoint(planet.longitude, radius);
    double x = p.x();
    double y = p.y();



    PlanetItem *planetItem = new PlanetItem(planet.id, planet.sign,
                                            planet.longitude, planet.house, planet.isRetrograde);


    planetItem->setPos(x - AsteriaFlags::planetSize/2, y - AsteriaFlags::planetSize/2);

    m_scene->addItem(planetItem);
    m_planetItems[planet.id] = planetItem;
}


QString ChartRenderer::getPlanetSymbol(const QString &planetId) {

    static QMap<QString, QString> symbols = {
        {"Sun", "☉"},
        {"Moon", "☽"},
        {"Mercury", "☿"},
        {"Venus", "♀"},
        {"Mars", "♂"},
        {"Jupiter", "♃"},
        {"Saturn", "♄"},
        {"Uranus", "♅"},
        {"Neptune", "♆"},
        {"Pluto", "♇"},
        {"Chiron", "⚷"},
        {"North Node", "☊"},
        {"South Node", "☋"},
        {"Pars Fortuna", "⊕"}, // Part of Fortune symbol (circle with plus)
        {"Syzygy", "☍"},        // Using opposition symbol for Syzygy
        {"pa", "⊕"},            // Abbreviated Pars Fortuna (changed to match Part of Fortune)
        {"sy", "☍"},            // Abbreviated Syzygy

        {"Lilith", "⚸"},        // Black Moon Lilith symbol
        {"Ceres", "⚳"},         // Ceres symbol
        {"Pallas", "⚴"},        // Pallas symbol
        {"Juno", "⚵"},          // Juno symbol
        {"Vesta", "⚶"},         // Vesta symbol
        {"Vertex", "⊗"},        // Using a cross in circle for Vertex
        {"East Point", "⊙"},    // Using a dot in circle for East Point
        {"Part of Spirit", "⊖"}  // Part of Spirit (circle with minus)
    };



    return symbols.value(planetId, planetId);
}

void ChartRenderer::updateSettings(bool showAspects, bool showHouseCusps,
                                   bool showPlanetSymbols, int chartSize) {
    m_showAspects = showAspects;
    m_showHouseCusps = showHouseCusps;
    m_showPlanetSymbols = showPlanetSymbols;

    if (chartSize != m_chartSize) {
        m_chartSize = chartSize;
        m_scene->setSceneRect(-m_chartSize/2, -m_chartSize/2, m_chartSize, m_chartSize);
    }

    renderChart();
}



void ChartRenderer::drawHouseRing() {
    double outerRadius = m_chartSize / 2.0;
    double zodiacOuterRadius = outerRadius;
    double houseRingInnerRadius = zodiacOuterRadius + 10; // Small gap between zodiac and house ring
    double houseRingOuterRadius = houseRingInnerRadius + 30; // Width of house ring

    QGraphicsEllipseItem *houseRingOuter = new QGraphicsEllipseItem(
        -houseRingOuterRadius, -houseRingOuterRadius,
        houseRingOuterRadius * 2, houseRingOuterRadius * 2);
    houseRingOuter->setPen(QPen(Qt::black, 1));
    houseRingOuter->setBrush(Qt::transparent);
    m_scene->addItem(houseRingOuter);

    QGraphicsEllipseItem *houseRingInner = new QGraphicsEllipseItem(
        -houseRingInnerRadius, -houseRingInnerRadius,
        houseRingInnerRadius * 2, houseRingInnerRadius * 2);
    houseRingInner->setPen(QPen(Qt::black, 1));
    houseRingInner->setBrush(Qt::transparent);
    m_scene->addItem(houseRingInner);

    QColor fireColor(255, 200, 200);  // Light red with transparency
    QColor earthColor(255, 255, 200);  // Light yellow with transparency
    QColor airColor(200, 255, 200);  // Light green with transparency
    QColor waterColor(200, 200, 255);  // Light blue with transparency

    QStringList houseTooltips = {
        "House 1 (Fire/Aries): Self, identity, appearance",
        "House 2 (Earth/Taurus): Possessions, values, resources",
        "House 3 (Air/Gemini): Communication, siblings, local travel",
        "House 4 (Water/Cancer): Home, family, roots",
        "House 5 (Fire/Leo): Creativity, pleasure, children",
        "House 6 (Earth/Virgo): Work, health, service",
        "House 7 (Air/Libra): Partnerships, marriage, open enemies",
        "House 8 (Water/Scorpio): Shared resources, transformation, death",
        "House 9 (Fire/Sagittarius): Higher education, philosophy, travel",
        "House 10 (Earth/Capricorn): Career, public image, authority",
        "House 11 (Air/Aquarius): Friends, groups, hopes and wishes",
        "House 12 (Water/Pisces): Unconscious, spirituality, hidden matters"
    };

    if (m_chartData.houses.size() == 12) {
        double refAsc = (!m_chartData.houses.isEmpty() ? m_chartData.houses[0].longitude : getAscendantLongitude());
        for (int i = 0; i < 12; i++) {
            const HouseData &currentHouse = m_chartData.houses[i];
            const HouseData &nextHouse = m_chartData.houses[(i + 1) % 12];

            double currentLongitude = currentHouse.longitude;
            double nextLongitude = nextHouse.longitude;

            if (nextLongitude < currentLongitude) {
                nextLongitude += 360.0;
            }

            double midLongitude = (currentLongitude + nextLongitude) / 2.0;
            if (midLongitude >= 360.0) {
                midLongitude -= 360.0;
            }

            QPainterPath housePath;

            QPointF innerStartPoint = longitudeToPoint(currentLongitude, houseRingInnerRadius);
            housePath.moveTo(innerStartPoint);

            QPointF outerStartPoint = longitudeToPoint(currentLongitude, houseRingOuterRadius);
            housePath.lineTo(outerStartPoint);


            double startAngle = 180.0 + (currentLongitude - refAsc);  // rotated by Asc/House1
            double sweepAngle = nextLongitude - currentLongitude; // counterclockwise extent
            if (sweepAngle < 0) sweepAngle += 360.0;  // ensure positive extent

            /*
            housePath.arcTo(-houseRingOuterRadius, -houseRingOuterRadius,
                            houseRingOuterRadius * 2, houseRingOuterRadius * 2,
                            startAngle, sweepAngle);

            QPointF innerEndPoint = longitudeToPoint(nextLongitude, houseRingInnerRadius);
            housePath.lineTo(innerEndPoint);

            housePath.arcTo(-houseRingInnerRadius, -houseRingInnerRadius,
                            houseRingInnerRadius * 2, houseRingInnerRadius * 2,
                            startAngle + sweepAngle, -sweepAngle);

            housePath.closeSubpath();
            */

            housePath.arcTo(-houseRingOuterRadius, -houseRingOuterRadius,
                            houseRingOuterRadius * 2, houseRingOuterRadius * 2,
                            startAngle, sweepAngle); // Positive for counterclockwise

            QPointF innerEndPoint = longitudeToPoint(nextLongitude, houseRingInnerRadius);
            housePath.lineTo(innerEndPoint);

            housePath.arcTo(-houseRingInnerRadius, -houseRingInnerRadius,
                            houseRingInnerRadius * 2, houseRingInnerRadius * 2,
                            startAngle + sweepAngle, -sweepAngle); // Negative to go clockwise for return

            housePath.closeSubpath();



            QColor houseColor;
            int houseNum = i + 1;  // Convert to 1-indexed house number

            if (houseNum == 1 || houseNum == 5 || houseNum == 9) {
                houseColor = fireColor;
            }
            else if (houseNum == 2 || houseNum == 6 || houseNum == 10) {
                houseColor = earthColor;
            }
            else if (houseNum == 3 || houseNum == 7 || houseNum == 11) {
                houseColor = airColor;
            }
            else {
                houseColor = waterColor;
            }

            QGraphicsPathItem *houseItem = new QGraphicsPathItem(housePath);

            houseItem->setPen(QPen(Qt::black, 1));
            houseItem->setBrush(QBrush(houseColor));
            QString cuspInfo = QString("@ %1").arg(currentHouse.sign);
            QString tooltip = houseTooltips[i] + QString("\nCusp: %1").arg(cuspInfo);
            houseItem->setToolTip(tooltip);
            m_scene->addItem(houseItem);

            double textRadius = (houseRingInnerRadius + houseRingOuterRadius) / 2.0;
            QPointF textPoint = longitudeToPoint(midLongitude, textRadius);

            QGraphicsTextItem *houseNumber = new QGraphicsTextItem(QString::number(i + 1));

            QFont font;
            font.setPointSize(AsteriaFlags::uiFontSize - 2);
            font.setBold(true);
            houseNumber->setFont(font);

            QRectF textRect = houseNumber->boundingRect();
            houseNumber->setPos(textPoint.x() - textRect.width()/2,
                                textPoint.y() - textRect.height()/2);

            m_scene->addItem(houseNumber);

            QPointF innerPoint = longitudeToPoint(currentLongitude, houseRingInnerRadius);
            QPointF outerPoint = longitudeToPoint(currentLongitude, houseRingOuterRadius);
            QGraphicsLineItem *extensionLine = new QGraphicsLineItem(
                QLineF(innerPoint, outerPoint));
            extensionLine->setPen(QPen(Qt::black, 1, Qt::SolidLine));
            m_scene->addItem(extensionLine);
        }
    }
}


double ChartRenderer::getAscendantLongitude() const {
    for (const AngleData &angle : m_chartData.angles) {
        if (angle.id == "Asc") {
            return angle.longitude;
        }
    }
    return 0.0; // Fallback if not found
}

