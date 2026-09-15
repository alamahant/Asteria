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

    m_titleLabel = new QLabel("Planets", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setShowGrid(true);

#ifndef FLATHUB_BUILD
    m_table->setAlternatingRowColors(false);
#else
    m_table->setAlternatingRowColors(true);
#endif

    m_table->verticalHeader()->setVisible(false);
    m_table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    m_table->setWordWrap(false);
    m_table->horizontalHeader()->setMinimumSectionSize(1);
    m_table->verticalHeader()->setMinimumSectionSize(1);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_table->setColumnCount(5);
    QStringList headers;
    headers << "Planet" << "Sign" << "Degree" << "Minute" << "House";
    m_table->setHorizontalHeaderLabels(headers);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_table);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
}

void PlanetListWidget::updateData(const ChartData &chartData)
{
    m_table->setRowCount(0);

    QStringList orderedPlanets = {
        "Sun", "Moon", "Mercury", "Venus", "Mars", "Jupiter", "Saturn",
        "Uranus", "Neptune", "Pluto", "Chiron", "North Node", "South Node",
        "Pars Fortuna", "Syzygy",
        "Lilith", "Ceres", "Pallas", "Juno", "Vesta",
        "Vertex", "East Point", "Part of Spirit"
    };

    QMap<QString, PlanetData> planetMap;
    for (const PlanetData &planet : chartData.planets) {
        planetMap[planet.id] = planet;
    }

    QFont symbolFont = m_table->font();
    bool useCustomFont = !g_astroFontFamily.isEmpty();
    if (useCustomFont) {
        symbolFont = QFont(g_astroFontFamily, symbolFont.pointSize());

    }

    for (const QString &planetId : orderedPlanets) {
        if (planetMap.contains(planetId)) {
            const PlanetData &planet = planetMap[planetId];
            int row = m_table->rowCount();
            m_table->insertRow(row);

            QString planetSymbol = getSymbolForPlanet(planet.id);
            QTableWidgetItem *planetItem = new QTableWidgetItem(planetSymbol + " " + planet.id + (planet.isRetrograde && planet.id != "North Node" && planet.id != "South Node" ? " ℞ " : ""));
            if (useCustomFont) {
                planetItem->setFont(symbolFont);
            }


            QString signName = planet.sign.split(' ').first();
            QString signSymbol = getSymbolForSign(signName);
            QTableWidgetItem *signItem = new QTableWidgetItem(signSymbol + " " + signName);

            if (useCustomFont) {
            #ifdef Q_OS_WIN
                QFont zodiacFont(g_astroFontFamily.isEmpty() ? "DejaVu Sans" : g_astroFontFamily,
                                 symbolFont.pointSize());
            #else
                QFont zodiacFont("DejaVu Sans", symbolFont.pointSize());      // use a known system font
                zodiacFont.setStyleStrategy(QFont::NoFontMerging);
            #endif
                signItem->setFont(zodiacFont);
            }

            signItem->setBackground(getColorForSign(signName));

            int degree = static_cast<int>(planet.longitude) % 30;
            QTableWidgetItem *degreeItem = new QTableWidgetItem(QString::number(degree) + "°");

            int minute = static_cast<int>((planet.longitude - static_cast<int>(planet.longitude)) * 60);
            QTableWidgetItem *minuteItem = new QTableWidgetItem(QString::number(minute) + "'");

            QTableWidgetItem *houseItem = new QTableWidgetItem(planet.house);

            m_table->setItem(row, 0, planetItem);
            m_table->setItem(row, 1, signItem);
            m_table->setItem(row, 2, degreeItem);
            m_table->setItem(row, 3, minuteItem);
            m_table->setItem(row, 4, houseItem);

            for (int col = 0; col < m_table->columnCount(); ++col) {
                m_table->item(row, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }

    for (const PlanetData &planet : chartData.planets) {
        if (!orderedPlanets.contains(planet.id)) {
            int row = m_table->rowCount();
            m_table->insertRow(row);

            QTableWidgetItem *planetItem = new QTableWidgetItem(planet.id);

            QString signName = planet.sign.split(' ').first();
            QTableWidgetItem *signItem = new QTableWidgetItem(signName);
            signItem->setBackground(getColorForSign(signName));

            int degree = static_cast<int>(planet.longitude) % 30;
            QTableWidgetItem *degreeItem = new QTableWidgetItem(QString::number(degree) + "°");

            int minute = static_cast<int>((planet.longitude - static_cast<int>(planet.longitude)) * 60);
            QTableWidgetItem *minuteItem = new QTableWidgetItem(QString::number(minute) + "'");

            QTableWidgetItem *houseItem = new QTableWidgetItem(planet.house);

            m_table->setItem(row, 0, planetItem);
            m_table->setItem(row, 1, signItem);
            m_table->setItem(row, 2, degreeItem);
            m_table->setItem(row, 3, minuteItem);
            m_table->setItem(row, 4, houseItem);

            for (int col = 0; col < m_table->columnCount(); ++col) {
                m_table->item(row, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setMinimumSectionSize(1);
    m_table->verticalHeader()->setMinimumSectionSize(1);
}

QString PlanetListWidget::getSymbolForPlanet(const QString &planetId)
{
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
    if (planetId == "Lilith") return "⚸";
    if (planetId == "Ceres") return "⚳";
    if (planetId == "Pallas") return "⚴";
    if (planetId == "Juno") return "⚵";
    if (planetId == "Vesta") return "⚶";
    if (planetId == "Vertex") return "⊗";
    if (planetId == "East Point") return "⊙";
    if (planetId == "Part of Spirit") return "⊖";

    return "";
}

QString PlanetListWidget::getSymbolForSign(const QString &sign)
{
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
    if (sign == "Aries" || sign == "Leo" || sign == "Sagittarius") {
        return QColor(255, 200, 200);  // Light red for Fire
    } else if (sign == "Taurus" || sign == "Virgo" || sign == "Capricorn") {
        return QColor(255, 245, 160);  // Light golden

    } else if (sign == "Gemini" || sign == "Libra" || sign == "Aquarius") {
        return QColor(200, 255, 200);  // Light green for Air 200, 255, 200

    } else if (sign == "Cancer" || sign == "Scorpio" || sign == "Pisces") {
        return QColor(200, 200, 255);  // Light blue for Water
    }

    return QColor(240, 240, 240);  // Light gray default
}
