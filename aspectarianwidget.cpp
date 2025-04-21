#include "aspectarianwidget.h"
#include <QHeaderView>
#include <QDebug>

extern QString g_astroFontFamily;


AspectarianWidget::AspectarianWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void AspectarianWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Title label
    m_titleLabel = new QLabel("Aspectarian", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // Create table
    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setShowGrid(true);
    m_table->setGridStyle(Qt::SolidLine);
    m_table->horizontalHeader()->setVisible(true);
    m_table->verticalHeader()->setVisible(true);

    // Make the table expand to fill available space
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Add widgets to layout
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_table);
    setLayout(layout);

    // Set the size policy to expand in both directions
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void AspectarianWidget::updateData(const ChartData &chartData)
{
    // Clear the table
    m_table->clear();

    // Get list of planets to display
    QStringList planets;
    for (const auto &planet : chartData.planets) {
        planets << planet.id;
    }

    // Sort planets in traditional order
    QStringList orderedPlanets = {
        "Sun", "Moon", "Mercury", "Venus", "Mars", "Jupiter", "Saturn",
        "Uranus", "Neptune", "Pluto", "Chiron", "North Node", "South Node",
        "Pars Fortuna", "Syzygy"
    };

    // Filter and sort planets
    QStringList displayPlanets;
    for (const QString &planet : orderedPlanets) {
        if (planets.contains(planet)) {
            displayPlanets << planet;
        }
    }

    // Add any remaining planets not in the ordered list
    for (const QString &planet : planets) {
        if (!displayPlanets.contains(planet)) {
            displayPlanets << planet;
        }
    }

    // Set up table dimensions
    int numPlanets = displayPlanets.size();
    m_table->setRowCount(numPlanets);
    m_table->setColumnCount(numPlanets);

    // Create symbol headers
    QStringList symbolHeaders;
    for (const QString &planet : displayPlanets) {
        symbolHeaders << planetSymbol(planet);
    }

    // Set headers with symbols
    m_table->setHorizontalHeaderLabels(symbolHeaders);
    m_table->setVerticalHeaderLabels(symbolHeaders);

    // Set fixed font size for headers instead of incrementing
    QFont headerFont = m_table->font();
    headerFont.setPointSize(12); // Use a fixed size instead of incrementing

    if (!g_astroFontFamily.isEmpty()) {
        headerFont = QFont(g_astroFontFamily, 12);
    }


    m_table->horizontalHeader()->setFont(headerFont);
    m_table->verticalHeader()->setFont(headerFont);

    // Set tooltips for headers
    for (int i = 0; i < numPlanets; i++) {
        QTableWidgetItem* hItem = m_table->horizontalHeaderItem(i);
        QTableWidgetItem* vItem = m_table->verticalHeaderItem(i);
        if (hItem) {
            hItem->setToolTip(displayPlanets[i]);
        }
        if (vItem) {
            vItem->setToolTip(displayPlanets[i]);
        }
    }

    // Create a map for quick planet lookup
    QMap<QString, int> planetIndices;
    for (int i = 0; i < displayPlanets.size(); ++i) {
        planetIndices[displayPlanets[i]] = i;
    }

    // Fill the table with aspects
    for (const AspectData &aspect : chartData.aspects) {
        if (!planetIndices.contains(aspect.planet1) || !planetIndices.contains(aspect.planet2)) {
            continue;
        }
        int row = planetIndices[aspect.planet1];
        int col = planetIndices[aspect.planet2];
        // Only fill the upper triangle of the table (avoid duplicates)
        if (row > col) {
            std::swap(row, col);
        }

        // Create item with ONLY the aspect symbol
        QString aspectText = aspectSymbol(aspect.aspectType);
        QTableWidgetItem *item = new QTableWidgetItem(aspectText);
        item->setTextAlignment(Qt::AlignCenter);

        // Use fixed font size for aspect symbols
        QFont symbolFont = m_table->font();
        symbolFont.setPointSize(14); // Use a fixed size instead of incrementing
        item->setFont(symbolFont);

        // Set background color based on aspect type
        QColor color = aspectColor(aspect.aspectType);
        item->setBackground(color);

        // Set tooltip with more information
        item->setToolTip(QString("%1 %2 %3 (Orb: %4°)")
                             .arg(aspect.planet1)
                             .arg(aspect.aspectType)
                             .arg(aspect.planet2)
                             .arg(aspect.orb, 0, 'f', 2));

        m_table->setItem(row, col, item);
    }
}


QString AspectarianWidget::planetSymbol(const QString &planetName)
{
    if (planetName == "Sun") return "☉";
    if (planetName == "Moon") return "☽";
    if (planetName == "Mercury") return "☿";
    if (planetName == "Venus") return "♀";
    if (planetName == "Mars") return "♂";
    if (planetName == "Jupiter") return "♃";
    if (planetName == "Saturn") return "♄";
    if (planetName == "Uranus") return "♅";
    if (planetName == "Neptune") return "♆";
    if (planetName == "Pluto") return "♇";
    if (planetName == "Chiron") return "⚷";
    if (planetName == "North Node") return "☊";
    if (planetName == "South Node") return "☋";
    if (planetName == "Pars Fortuna") return "⊗";
    if (planetName == "Syzygy") return "☍";
    // Return first letter for any other planet
    return planetName.left(1);
}

QColor AspectarianWidget::aspectColor(const QString &aspectType)
{
    if (aspectType == "CON") return QColor(128, 128, 128, 50);    // Gray (Conjunction)
    if (aspectType == "OPP") return QColor(255, 0, 0, 50);         // Red (Opposition)
    if (aspectType == "SQR") return QColor(255, 0, 0, 50);         // Red (Square)
    if (aspectType == "TRI") return QColor(0, 0, 255, 50);         // Blue (Trine)
    if (aspectType == "SEX") return QColor(0, 255, 255, 50);       // Cyan (Sextile)
    if (aspectType == "QUI") return QColor(0, 255, 0, 50);         // Bright Green (Quincunx)
    if (aspectType == "SSQ") return QColor(255, 140, 0, 50);       // Dark Orange (Semi-square)
    if (aspectType == "SSX") return QColor(186, 85, 211, 50);      // Medium Orchid (Semi-sextile)

    return QColor(128, 128, 128, 50);  // Default gray
}


QString AspectarianWidget::aspectSymbol(const QString &aspectType)
{
    if (aspectType == "CON") return "☌";  // Conjunction
    if (aspectType == "OPP") return "☍";  // Opposition
    if (aspectType == "SQR") return "□";  // Square
    if (aspectType == "TRI") return "△";  // Trine
    if (aspectType == "SEX") return "✶";  // Sextile — more distinct than ⚹
    if (aspectType == "QUI") return "⦻";  // Quincunx — unique circle-cross
    if (aspectType == "SSQ") return "∟";  // Semi-square — right angle, sharper than ∠
    if (aspectType == "SSX") return "⧫";  // Semi-sextile — diamond
    return aspectType;  // Fallback to raw code
}

