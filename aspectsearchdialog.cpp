#include "aspectsearchdialog.h"
#include <QVBoxLayout>

AspectSearchDialog::AspectSearchDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Aspect Search");
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_planet1Filter = new QLineEdit(this);
    m_aspectFilter = new QLineEdit(this);
    m_planet2Filter = new QLineEdit(this);
    m_maxOrbFilter = new QLineEdit(this);
    m_excludeFilter = new QLineEdit(this);

    m_planet1Filter->setPlaceholderText("Filter by Planet 1");
    m_planet1Filter->setToolTip("Filter by first planet in the aspect.\n"
                                "Same planet list as transit planets.\n"
                                "Sun, Moon, Mercury, Venus, Mars,\n"
                                "Jupiter, Saturn, Uranus, Neptune, Pluto,\n"
                                "Chiron, North Node, South Node,\n"
                                "Pars Fortuna, Part of Spirit, East Point");

    m_aspectFilter->setPlaceholderText("Filter by aspect");
    m_aspectFilter->setToolTip("Filter by aspect types. Examples:\n"
                               "MAJOR ASPECTS:\n"
                               "• CON (Conjunction 0°) - fusion, new beginnings\n"
                               "• OPP (Opposition 180°) - tension, awareness, balance\n"
                               "• TRI (Trine 120°) - harmony, ease, flow\n"
                               "• SQR (Square 90°) - challenge, action, growth\n"
                               "• SEX (Sextile 60°) - opportunity, cooperation\n"
                               "MINOR ASPECTS:\n"
                               "• QUI (Quincunx 150°) - adjustment, awkwardness\n"
                               "• SSX (Semisextile 30°) - subtle opportunity\n"
                               "• SQQ (Sesquiquadrate 135°) - Challenge overcoming\n"
                               "• SSQ (Semisquare 45°) - minor friction");

    m_planet2Filter->setPlaceholderText("Filter by Planet 2");
    m_planet2Filter->setToolTip("Filter by second planet in the aspect.\n"
                                "Same planet list as Planet 1.");

    m_maxOrbFilter->setPlaceholderText("Max Orb (degrees)");
    m_maxOrbFilter->setToolTip("Set the maximum allowed orb (in degrees) for aspects.\n"
                               "Examples:\n"
                               "• 6   (show only aspects with orb ≤ 6°)\n"
                               "• 2.5 (show only aspects with orb ≤ 2.5°)\n"
                               "Leave empty for default orb.");

    m_excludeFilter->setPlaceholderText("Exclude Filter");
    m_excludeFilter->setToolTip("Exclude specific terms (comma-separated).\n"
                                "Searches across ALL columns. Examples:\n"
                                "COMMON EXCLUSIONS:\n"
                                "• (R) - hide all retrograde planets\n"
                                "• QUI, SSX, SSQ - hide minor aspects\n"
                                "• Lilith, Vesta - hide specific asteroids\n"
                                "• OPP, SQR - hide challenging aspects\n"
                                "COMBINATIONS:\n"
                                "• (R), QUI, Lilith - multiple exclusions\n"
                                "• Moon, Mercury - hide fast-moving planets\n"
                                "• SSQ, SSX - hide semi-aspects\n"
                                "• CON, OPP, SQR - show only soft aspects");

    m_applyButton = new QPushButton("Apply Filter", this);
    m_applyButton->setToolTip("Depending on the volume of data\n"
                              "filtering may take some time.\n"
                              "Please be patient!");

    m_clearButton = new QPushButton("Clear All", this);

    mainLayout->addWidget(m_planet1Filter);
    mainLayout->addWidget(m_aspectFilter);
    mainLayout->addWidget(m_planet2Filter);
    mainLayout->addWidget(m_maxOrbFilter);
    mainLayout->addWidget(m_excludeFilter);
    mainLayout->addWidget(m_applyButton);
    mainLayout->addWidget(m_clearButton);

    statusLabel = new QLabel(this);
    statusLabel->setText("");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #2980b9; font-weight: bold;");
    mainLayout->addWidget(statusLabel);

    connect(m_applyButton, &QPushButton::clicked, this, &AspectSearchDialog::emitFilter);
    connect(m_clearButton, &QPushButton::clicked, this, &AspectSearchDialog::clearFilters);
}

void AspectSearchDialog::emitFilter()
{
    emit filterChanged(m_planet1Filter->text(),
                       m_aspectFilter->text(),
                       m_planet2Filter->text(),
                       m_maxOrbFilter->text(),
                       m_excludeFilter->text());
}

void AspectSearchDialog::clearFilters()
{
    m_planet1Filter->clear();
    m_aspectFilter->clear();
    m_planet2Filter->clear();
    m_maxOrbFilter->clear();
    m_excludeFilter->clear();

    emitFilter();
}