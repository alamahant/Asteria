#include "elementmodalitywidget.h"
#include <QFont>
#include <QFrame>

extern QString g_astroFontFamily;


ElementModalityWidget::ElementModalityWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ElementModalityWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Title label
    m_titleLabel = new QLabel("Elements & Modalities", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // Create grid layout for the element/modality table
    QWidget *gridWidget = new QWidget(this);
    m_gridLayout = new QGridLayout(gridWidget);
    m_gridLayout->setSpacing(2);

    // Create header labels
    QLabel *headerElement = new QLabel("", this);
    QLabel *headerFire = new QLabel("Fire", this);
    QLabel *headerEarth = new QLabel("Earth", this);
    QLabel *headerAir = new QLabel("Air", this);
    QLabel *headerWater = new QLabel("Water", this);
    QLabel *headerTotal = new QLabel("Total", this);

    QLabel *headerCardinal = new QLabel("Cardinal", this);
    QLabel *headerFixed = new QLabel("Fixed", this);
    QLabel *headerMutable = new QLabel("Mutable", this);

    // Set header styles
    QFont headerFont;
    headerFont.setBold(true);
    headerElement->setFont(headerFont);
    headerFire->setFont(headerFont);
    headerEarth->setFont(headerFont);
    headerAir->setFont(headerFont);
    headerWater->setFont(headerFont);
    headerTotal->setFont(headerFont);
    headerCardinal->setFont(headerFont);
    headerFixed->setFont(headerFont);
    headerMutable->setFont(headerFont);

    // Set header colors
    headerFire->setStyleSheet("background-color: rgba(255, 100, 100, 100);");
    headerEarth->setStyleSheet("background-color: rgba(255, 255, 100, 100);"); // Yellow for Earth
    headerAir->setStyleSheet("background-color: rgba(100, 200, 100, 100);");   // Green for Air
    headerWater->setStyleSheet("background-color: rgba(100, 100, 255, 100);");
    // Add headers to grid
    m_gridLayout->addWidget(headerElement, 0, 0);
    m_gridLayout->addWidget(headerFire, 0, 1);
    m_gridLayout->addWidget(headerEarth, 0, 2);
    m_gridLayout->addWidget(headerAir, 0, 3);
    m_gridLayout->addWidget(headerWater, 0, 4);
    m_gridLayout->addWidget(headerTotal, 0, 5);

    m_gridLayout->addWidget(headerCardinal, 1, 0);
    m_gridLayout->addWidget(headerFixed, 2, 0);
    m_gridLayout->addWidget(headerMutable, 3, 0);

    // Create cells for each sign
    QStringList signs = {
        "Aries", "Taurus", "Gemini", "Cancer",
        "Leo", "Virgo", "Libra", "Scorpio",
        "Sagittarius", "Capricorn", "Aquarius", "Pisces"
    };

    // Map signs to their positions in the grid
    QMap<QString, QPair<int, int>> signPositions = {
        {"Aries", {1, 1}},       // Cardinal Fire
        {"Taurus", {2, 2}},      // Fixed Earth
        {"Gemini", {3, 3}},      // Mutable Air
        {"Cancer", {1, 4}},      // Cardinal Water
        {"Leo", {2, 1}},         // Fixed Fire
        {"Virgo", {3, 2}},       // Mutable Earth
        {"Libra", {1, 3}},       // Cardinal Air
        {"Scorpio", {2, 4}},     // Fixed Water
        {"Sagittarius", {3, 1}}, // Mutable Fire
        {"Capricorn", {1, 2}},   // Cardinal Earth
        {"Aquarius", {2, 3}},    // Fixed Air
        {"Pisces", {3, 4}}       // Mutable Water
    };

    // Create labels for each sign
    for (const QString &sign : signs) {

        // Create label for the sign
        QString glyph = getSignGlyph(sign);
        QLabel *label = new QLabel(glyph, this);
        //label->setAlignment(Qt::AlignCenter);
        label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);


        label->setFrameShape(QFrame::Box);
        label->setMinimumSize(80, 60);

        // Use a font that supports Unicode symbols
        QFont glyphFont = label->font();
        glyphFont.setPointSize(16); // Larger font for the glyph
        label->setFont(glyphFont);

        // Set background color based on element
        QString element = getElement(sign);
        QColor color = getElementColor(element);
        label->setStyleSheet(QString("background-color: rgba(%1, %2, %3, 50);")
                                 .arg(color.red()).arg(color.green()).arg(color.blue()));

        // Add to grid at the correct position
        QPair<int, int> pos = signPositions[sign];
        m_gridLayout->addWidget(label, pos.first, pos.second);

        // Store for later updates
        m_signLabels[sign] = label;

    }

    // Create total labels
    m_fireTotal = new QLabel("0", this);
    m_earthTotal = new QLabel("0", this);
    m_airTotal = new QLabel("0", this);
    m_waterTotal = new QLabel("0", this);
    m_cardinalTotal = new QLabel("0", this);
    m_fixedTotal = new QLabel("0", this);
    m_mutableTotal = new QLabel("0", this);

    // Set total label styles
    QFont totalFont = m_fireTotal->font();
    totalFont.setBold(true);
    m_fireTotal->setFont(totalFont);
    m_earthTotal->setFont(totalFont);
    m_airTotal->setFont(totalFont);
    m_waterTotal->setFont(totalFont);
    m_cardinalTotal->setFont(totalFont);
    m_fixedTotal->setFont(totalFont);
    m_mutableTotal->setFont(totalFont);

    m_fireTotal->setAlignment(Qt::AlignCenter);
    m_earthTotal->setAlignment(Qt::AlignCenter);
    m_airTotal->setAlignment(Qt::AlignCenter);
    m_waterTotal->setAlignment(Qt::AlignCenter);
    m_cardinalTotal->setAlignment(Qt::AlignCenter);
    m_fixedTotal->setAlignment(Qt::AlignCenter);
    m_mutableTotal->setAlignment(Qt::AlignCenter);

    // Add total labels to grid
    m_gridLayout->addWidget(m_fireTotal, 4, 1);
    m_gridLayout->addWidget(m_earthTotal, 4, 2);
    m_gridLayout->addWidget(m_airTotal, 4, 3);
    m_gridLayout->addWidget(m_waterTotal, 4, 4);

    m_gridLayout->addWidget(m_cardinalTotal, 1, 5);
    m_gridLayout->addWidget(m_fixedTotal, 2, 5);
    m_gridLayout->addWidget(m_mutableTotal, 3, 5);

    // Add a grand total label
    QLabel *grandTotalLabel = new QLabel("Total", this);
    QLabel *grandTotal = new QLabel("0", this);
    grandTotalLabel->setFont(headerFont);
    grandTotal->setFont(totalFont);
    grandTotal->setAlignment(Qt::AlignCenter);

    m_gridLayout->addWidget(grandTotalLabel, 4, 0);
    m_gridLayout->addWidget(grandTotal, 4, 5);

    // Add the grid to the main layout
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(gridWidget);

    setLayout(mainLayout);
}


void ElementModalityWidget::updateData(const ChartData &chartData) {
    // Reset all counts and planet lists
    QMap<QString, QStringList> signPlanets;
    QMap<QString, int> elementCounts;
    QMap<QString, int> modalityCounts;

    // Initialize counts to zero and planet lists to empty
    QStringList signs = {
        "Aries", "Taurus", "Gemini", "Cancer",
        "Leo", "Virgo", "Libra", "Scorpio",
        "Sagittarius", "Capricorn", "Aquarius", "Pisces"
    };

    for (const QString &sign : signs) {
        signPlanets[sign] = QStringList();
    }

    elementCounts["Fire"] = 0;
    elementCounts["Earth"] = 0;
    elementCounts["Air"] = 0;
    elementCounts["Water"] = 0;
    modalityCounts["Cardinal"] = 0;
    modalityCounts["Fixed"] = 0;
    modalityCounts["Mutable"] = 0;

    // Collect planets in each sign
    for (const PlanetData &planet : chartData.planets) {
        if (signPlanets.contains(planet.sign)) {
            signPlanets[planet.sign].append(planet.id);

            // Update element and modality counts
            QString element = getElement(planet.sign);
            QString modality = getModality(planet.sign);
            elementCounts[element]++;
            modalityCounts[modality]++;
        }
    }

    // Update the sign labels with planet glyphs

    for (const QString &sign : signs) {
        if (m_signLabels.contains(sign)) {
            QString signGlyph = getSignGlyph(sign);
            QString planetGlyphs = "";

            // Convert planet names to glyphs and join them
            for (const QString &planetId : signPlanets[sign]) {
                planetGlyphs += getPlanetGlyph(planetId) + " ";
            }

            // Create a simple text label with sign glyph and planet glyphs
            QString labelText = signGlyph + "\n" + planetGlyphs.trimmed();

            // Set the label text without HTML formatting
            m_signLabels[sign]->setText(labelText);

            // Apply the Astromoony font to the label
            if (!g_astroFontFamily.isEmpty()) {
                QFont astroFont(g_astroFontFamily, 16);
                astroFont.setBold(true);
                m_signLabels[sign]->setFont(astroFont);
            }
        }
    }



    // Update the total labels
    m_fireTotal->setText(QString::number(elementCounts["Fire"]));
    m_earthTotal->setText(QString::number(elementCounts["Earth"]));
    m_airTotal->setText(QString::number(elementCounts["Air"]));
    m_waterTotal->setText(QString::number(elementCounts["Water"]));
    m_cardinalTotal->setText(QString::number(modalityCounts["Cardinal"]));
    m_fixedTotal->setText(QString::number(modalityCounts["Fixed"]));
    m_mutableTotal->setText(QString::number(modalityCounts["Mutable"]));

    // Update grand total
    int grandTotal = elementCounts["Fire"] + elementCounts["Earth"] +
                     elementCounts["Air"] + elementCounts["Water"];

    // Find the grand total label and update it
    QLayoutItem *item = m_gridLayout->itemAtPosition(4, 5);
    if (item && item->widget()) {
        QLabel *grandTotalLabel = qobject_cast<QLabel*>(item->widget());
        if (grandTotalLabel) {
            grandTotalLabel->setText(QString::number(grandTotal));
        }
    }
}

QString ElementModalityWidget::getElement(const QString &sign)
{
    if (sign == "Aries" || sign == "Leo" || sign == "Sagittarius") {
        return "Fire";
    } else if (sign == "Taurus" || sign == "Virgo" || sign == "Capricorn") {
        return "Earth";
    } else if (sign == "Gemini" || sign == "Libra" || sign == "Aquarius") {
        return "Air";
    } else if (sign == "Cancer" || sign == "Scorpio" || sign == "Pisces") {
        return "Water";
    }
    return "";
}

QString ElementModalityWidget::getModality(const QString &sign)
{
    if (sign == "Aries" || sign == "Cancer" || sign == "Libra" || sign == "Capricorn") {
        return "Cardinal";
    } else if (sign == "Taurus" || sign == "Leo" || sign == "Scorpio" || sign == "Aquarius") {
        return "Fixed";
    } else if (sign == "Gemini" || sign == "Virgo" || sign == "Sagittarius" || sign == "Pisces") {
        return "Mutable";
    }
    return "";
}
QColor ElementModalityWidget::getElementColor(const QString &element)
{
    if (element == "Fire") {
        return QColor(255, 100, 100);  // Red
    } else if (element == "Earth") {
        return QColor(255, 255, 100);  // Yellow for Earth
    } else if (element == "Air") {
        return QColor(100, 200, 100);  // Green for Air
    } else if (element == "Water") {
        return QColor(100, 100, 255);  // Blue
    }
    return QColor(200, 200, 200);  // Gray default
}

QString ElementModalityWidget::getSignGlyph(const QString &sign) {
    if (sign == "Aries") return "♈";
    if (sign == "Taurus") return "♉";
    if (sign == "Gemini") return "♊";
    if (sign == "Cancer") return "♋";
    if (sign == "Leo") return "♌";
    if (sign == "Virgo") return "♍";
    if (sign == "Libra") return "♎";
    if (sign == "Scorpio") return "♏";
    if (sign == "Sagittarius") return "♐";
    if (sign == "Capricorn") return "♑";
    if (sign == "Aquarius") return "♒";
    if (sign == "Pisces") return "♓";
    return sign;
}

QString ElementModalityWidget::getPlanetGlyph(const QString &planetId) {
    if (planetId == "Sun") return "☉";
    if (planetId == "Moon") return "☽";
    if (planetId == "Mercury") return "☿";
    if (planetId == "Venus") return "♀";
    if (planetId == "Mars") return "♂";
    if (planetId == "Jupiter") return "♃";
    if (planetId == "Saturn") return "♄";
    if (planetId == "Uranus") return "♅";
    if (planetId == "Neptune") return "♆";
    if (planetId == "Pluto") return "♇";
    if (planetId == "Chiron") return "⚷";
    if (planetId == "North Node") return "☊";
    if (planetId == "South Node") return "☋";
    if (planetId == "Pars Fortuna") return "⊗";
    if (planetId == "Syzygy") return "☍";
    // Additional bodies
    if (planetId == "Lilith") return "⚸";
    if (planetId == "Ceres") return "⚳";
    if (planetId == "Pallas") return "⚴";
    if (planetId == "Juno") return "⚵";
    if (planetId == "Vesta") return "⚶";
    if (planetId == "Vertex") return "⊗";
    if (planetId == "East Point") return "⊙";
    if (planetId == "Part of Spirit") return "⊖";
    // Return first letter for any other planet
    return planetId.left(1);
}
