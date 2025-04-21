#include "symbolsdialog.h"
#include <QHeaderView>
#include <QLabel>
#include <QFont>
#include <QColor>

SymbolsDialog::SymbolsDialog(QWidget *parent, const QString &astroFontFamily)
    : QDialog(parent), m_astroFontFamily(astroFontFamily)
{
    setWindowTitle("Astrological Symbols Reference");
    setMinimumSize(400, 500);
    setupUI();
}

SymbolsDialog::~SymbolsDialog()
{
}

void SymbolsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create tab widget
    m_tabWidget = new QTabWidget(this);

    // Create tables
    m_aspectTable = new QTableWidget(0, 4, this);  // Now 4 columns instead of 3
    m_aspectTable->setHorizontalHeaderLabels({"Aspect", "Abbr.", "Symbol", "Color"});
    m_aspectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_aspectTable->verticalHeader()->setVisible(false);

    m_planetTable = new QTableWidget(0, 2, this);
    m_planetTable->setHorizontalHeaderLabels({"Planet", "Symbol"});
    m_planetTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_planetTable->verticalHeader()->setVisible(false);

    m_signTable = new QTableWidget(0, 2, this);
    m_signTable->setHorizontalHeaderLabels({"Sign", "Symbol"});
    m_signTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_signTable->verticalHeader()->setVisible(false);

    // Add tables to tabs
    m_tabWidget->addTab(m_aspectTable, "Aspects");
    m_tabWidget->addTab(m_planetTable, "Planets");
    m_tabWidget->addTab(m_signTable, "Signs");

    mainLayout->addWidget(m_tabWidget);

    // Populate tables
    populateAspectTable();
    populatePlanetTable();
    populateSignTable();
}

void SymbolsDialog::populateAspectTable()
{
    struct AspectInfo {
        QString code;
        QString name;
        QString symbol;
        QColor color;
    };

    AspectInfo aspects[] = {
        {"CON", "Conjunction", "☌", QColor(128, 128, 128)},
        {"OPP", "Opposition", "☍", QColor(220, 20, 60)},
        {"SQR", "Square", "□", QColor(255, 69, 0)},
        {"TRI", "Trine", "△", QColor(30, 144, 255)},
        {"SEX", "Sextile", "✶", QColor(0, 206, 209)},
        {"QUI", "Quincunx", "⦻", QColor(138, 43, 226)},
        {"SSQ", "Semi-square", "∟", QColor(255, 165, 0)},
        {"SSX", "Semi-sextile", "⧫", QColor(218, 112, 214)}
    };

    int rowCount = sizeof(aspects) / sizeof(aspects[0]);
    m_aspectTable->setRowCount(rowCount);

    QFont symbolFont(m_astroFontFamily, 14);

    for (int i = 0; i < rowCount; i++) {
        // Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(aspects[i].name);
        m_aspectTable->setItem(i, 0, nameItem);

        // Abbreviation (code)
        QTableWidgetItem *codeItem = new QTableWidgetItem(aspects[i].code);
        m_aspectTable->setItem(i, 1, codeItem);

        // Symbol
        QTableWidgetItem *symbolItem = new QTableWidgetItem(aspects[i].symbol);
        symbolItem->setFont(symbolFont);
        symbolItem->setTextAlignment(Qt::AlignCenter);
        m_aspectTable->setItem(i, 2, symbolItem);

        // Color
        QTableWidgetItem *colorItem = new QTableWidgetItem();
        colorItem->setBackground(aspects[i].color);
        m_aspectTable->setItem(i, 3, colorItem);
    }
}

void SymbolsDialog::populatePlanetTable()
{
    struct PlanetInfo {
        QString name;
        QString symbol;
    };

    PlanetInfo planets[] = {
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
        {"Pars Fortuna", "⊗"},
        {"Syzygy", "☍"}
    };

    int rowCount = sizeof(planets) / sizeof(planets[0]);
    m_planetTable->setRowCount(rowCount);

    QFont symbolFont(m_astroFontFamily, 14);

    for (int i = 0; i < rowCount; i++) {
        // Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(planets[i].name);
        m_planetTable->setItem(i, 0, nameItem);

        // Symbol
        QTableWidgetItem *symbolItem = new QTableWidgetItem(planets[i].symbol);
        symbolItem->setFont(symbolFont);
        symbolItem->setTextAlignment(Qt::AlignCenter);
        m_planetTable->setItem(i, 1, symbolItem);
    }
}

void SymbolsDialog::populateSignTable()
{
    struct SignInfo {
        QString name;
        QString symbol;
    };

    SignInfo signs[] = {
        {"Aries", "♈"},
        {"Taurus", "♉"},
        {"Gemini", "♊"},
        {"Cancer", "♋"},
        {"Leo", "♌"},
        {"Virgo", "♍"},
        {"Libra", "♎"},
        {"Scorpio", "♏"},
        {"Sagittarius", "♐"},
        {"Capricorn", "♑"},
        {"Aquarius", "♒"},
        {"Pisces", "♓"}
    };

    int rowCount = sizeof(signs) / sizeof(signs[0]);
    m_signTable->setRowCount(rowCount);

    QFont symbolFont(m_astroFontFamily, 14);

    for (int i = 0; i < rowCount; i++) {
        // Name
        QTableWidgetItem *nameItem = new QTableWidgetItem(signs[i].name);
        m_signTable->setItem(i, 0, nameItem);

        // Symbol
        QTableWidgetItem *symbolItem = new QTableWidgetItem(signs[i].symbol);
        symbolItem->setFont(symbolFont);
        symbolItem->setTextAlignment(Qt::AlignCenter);
        m_signTable->setItem(i, 1, symbolItem);
    }
}

