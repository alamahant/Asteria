#include "planetlistwidget.h"
#include <QHeaderView>
#include <QFont>

extern QString g_astroFontFamily;


PlanetListWidget::PlanetListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PlanetListWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Title label
    m_titleLabel = new QLabel("Planets", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // Create table
    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(true);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);

    // Set up columns
    m_table->setColumnCount(5);
    QStringList headers;
    headers << "Planet" << "Sign" << "Degree" << "Minute" << "House";
    m_table->setHorizontalHeaderLabels(headers);

    // Add widgets to layout
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_table);

    setLayout(layout);
}

void PlanetListWidget::updateData(const ChartData &chartData)
{
    // Clear the table
    m_table->setRowCount(0);

    // Sort planets in traditional order
    QStringList orderedPlanets = {
        "Sun", "Moon", "Mercury", "Venus", "Mars", "Jupiter", "Saturn",
        "Uranus", "Neptune", "Pluto", "Chiron", "North Node", "South Node",
        "Pars Fortuna", "Syzygy"
    };

    // Create a map for quick lookup
    QMap<QString, PlanetData> planetMap;
    for (const PlanetData &planet : chartData.planets) {
        planetMap[planet.id] = planet;
    }

    // Create font for symbols if Astromoony is available
    QFont symbolFont = m_table->font();
    bool useCustomFont = !g_astroFontFamily.isEmpty();
    if (useCustomFont) {
        symbolFont = QFont(g_astroFontFamily, symbolFont.pointSize());
    }

    // Add planets in order
    for (const QString &planetId : orderedPlanets) {
        if (planetMap.contains(planetId)) {
            const PlanetData &planet = planetMap[planetId];
            int row = m_table->rowCount();
            m_table->insertRow(row);

            // Planet symbol and name
            QString planetSymbol = getSymbolForPlanet(planet.id);
            //QTableWidgetItem *planetItem = new QTableWidgetItem(planetSymbol + " " + planet.id);
            QTableWidgetItem *planetItem = new QTableWidgetItem(planetSymbol + " " + planet.id + (planet.isRetrograde && planet.id != "North Node" && planet.id != "South Node" ? " ℞ " : ""));
            if (useCustomFont) {
                planetItem->setFont(symbolFont);
            }

            // Sign symbol and name
            QString signSymbol = getSymbolForSign(planet.sign);
            QTableWidgetItem *signItem = new QTableWidgetItem(signSymbol + " " + planet.sign);
            if (useCustomFont) {
                signItem->setFont(symbolFont);
            }
            signItem->setBackground(getColorForSign(planet.sign));

            // Degree
            int degree = static_cast<int>(planet.longitude) % 30;
            QTableWidgetItem *degreeItem = new QTableWidgetItem(QString::number(degree) + "°");

            // Minute
            int minute = static_cast<int>((planet.longitude - static_cast<int>(planet.longitude)) * 60);
            QTableWidgetItem *minuteItem = new QTableWidgetItem(QString::number(minute) + "'");

            // House - use the house string directly
            QTableWidgetItem *houseItem = new QTableWidgetItem(planet.house);

            // Set items in the table
            m_table->setItem(row, 0, planetItem);
            m_table->setItem(row, 1, signItem);
            m_table->setItem(row, 2, degreeItem);
            m_table->setItem(row, 3, minuteItem);
            m_table->setItem(row, 4, houseItem);

            // Center align all items
            for (int col = 0; col < m_table->columnCount(); ++col) {
                m_table->item(row, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }

    // Add any remaining planets not in the ordered list
    for (const PlanetData &planet : chartData.planets) {
        if (!orderedPlanets.contains(planet.id)) {
            int row = m_table->rowCount();
            m_table->insertRow(row);

            // Planet name
            QTableWidgetItem *planetItem = new QTableWidgetItem(planet.id);

            // Sign
            QTableWidgetItem *signItem = new QTableWidgetItem(planet.sign);
            signItem->setBackground(getColorForSign(planet.sign));

            // Degree
            int degree = static_cast<int>(planet.longitude) % 30;
            QTableWidgetItem *degreeItem = new QTableWidgetItem(QString::number(degree) + "°");

            // Minute
            int minute = static_cast<int>((planet.longitude - static_cast<int>(planet.longitude)) * 60);
            QTableWidgetItem *minuteItem = new QTableWidgetItem(QString::number(minute) + "'");

            // House - use the house string directly
            QTableWidgetItem *houseItem = new QTableWidgetItem(planet.house);

            // Set items in the table
            m_table->setItem(row, 0, planetItem);
            m_table->setItem(row, 1, signItem);
            m_table->setItem(row, 2, degreeItem);
            m_table->setItem(row, 3, minuteItem);
            m_table->setItem(row, 4, houseItem);

            // Center align all items
            for (int col = 0; col < m_table->columnCount(); ++col) {
                m_table->item(row, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    // Resize columns to content
    m_table->resizeColumnsToContents();
}

QString PlanetListWidget::getSymbolForPlanet(const QString &planetId)
{
    // Unicode symbols for planets
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

    return "";
}

QString PlanetListWidget::getSymbolForSign(const QString &sign)
{
    // Unicode symbols for zodiac signs
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

    return "";
}

QColor PlanetListWidget::getColorForSign(const QString &sign)
{
    // Colors based on elements
    if (sign == "Aries" || sign == "Leo" || sign == "Sagittarius") {
        return QColor(255, 200, 200);  // Light red for Fire
    } else if (sign == "Taurus" || sign == "Virgo" || sign == "Capricorn") {
        return QColor(255, 255, 200);  // Light green for Earth 200, 255, 200
    } else if (sign == "Gemini" || sign == "Libra" || sign == "Aquarius") {
        return QColor(200, 255, 200);  // Light yellow for Air 255, 255, 200
    } else if (sign == "Cancer" || sign == "Scorpio" || sign == "Pisces") {
        return QColor(200, 200, 255);  // Light blue for Water
    }

    return QColor(240, 240, 240);  // Light gray default
}
