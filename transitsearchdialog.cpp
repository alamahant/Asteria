#include "transitsearchdialog.h"
#include<QVBoxLayout>

TransitSearchDialog::TransitSearchDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Transit Search");
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_dateFilter = new QLineEdit(this);
    m_transitPlanetFilter = new QLineEdit(this);
    m_aspectFilter = new QLineEdit(this);
    m_natalPlanetFilter = new QLineEdit(this);
    m_maxOrbFilter = new QLineEdit(this);


    m_dateFilter->setPlaceholderText("Filter by date (yyyy-mm-dd)");
    m_dateFilter->setToolTip("Filter by date patterns. Examples:\n"
                             "• 2024-01 (January 2024)\n"
                             "• 2024-01-15 (specific date)\n"
                             "• 2024 (entire year)\n"
                             "Supports regex patterns");

    m_transitPlanetFilter->setPlaceholderText("Filter by transit planet");
    m_transitPlanetFilter->setToolTip("Filter by transiting planets. Examples:\n"
                                      "PLANETS:\n"
                                      "• Sun, Moon, Mercury, Venus, Mars\n"
                                      "• Jupiter, Saturn, Uranus, Neptune, Pluto\n"
                                      "• Chiron (wounded healer)\n"
                                      "NODES & POINTS:\n"
                                      "• North Node (life purpose)\n"
                                      "• South Node (past life karma)\n"
                                      "ASTEROIDS:\n"
                                      "• Juno (marriage, partnerships)\n"
                                      "• Ceres (nurturing, motherhood)\n"
                                      "• Pallas (wisdom, strategy)\n"
                                      "• Vesta (devotion, sacred sexuality)\n"
                                      "SPECIAL POINTS:\n"
                                      "• Pars Fortuna (Part of Fortune - material luck)\n"
                                      "• Part of Spirit (Part of Spirit - spiritual purpose)\n"
                                      "• East Point (East Point - identity)\n"
                                      "• Vertex (fated encounters)\n"
                                      "•Lilith (Black Moon Lilith - shadow feminine, repressed power)");

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

    m_natalPlanetFilter->setPlaceholderText("Filter by natal planet");
    m_natalPlanetFilter->setToolTip("Filter by natal planets being aspected.\n"
                                    "Same planet list as transit planets.\n"
                                    "Shows which part of your birth chart\n"
                                    "is being activated by the transit.");

    m_maxOrbFilter->setPlaceholderText("Max Orb (degrees)");
    m_maxOrbFilter->setToolTip("Set the maximum allowed orb (in degrees) for aspects.\n"
                               "Examples:\n"
                               "• 6   (show only aspects with orb ≤ 6°)\n"
                               "• 2.5 (show only aspects with orb ≤ 2.5°)\n"
                               "Leave empty for default orb.");

    // Add single exclude field
    m_excludeFilter = new QLineEdit(this);
    m_excludeFilter->setPlaceholderText("Exclude Filter");
    m_excludeFilter->setToolTip("Exclude specific terms (comma-separated).\n"
                                "Searches across ALL columns. Examples:\n"
                                "COMMON EXCLUSIONS:\n"
                                "• (R) - hide all retrograde transits\n"
                                "• QUI, SSX, SSQ - hide minor aspects\n"
                                "• Lilith, Vesta - hide specific asteroids\n"
                                "• OPP, SQR - hide challenging aspects\n"
                                "• 2024-01 - hide January 2024\n"
                                "COMBINATIONS:\n"
                                "• (R), QUI, Lilith - multiple exclusions\n"
                                "• Moon, Mercury - hide fast-moving planets\n"
                                "• SSQ, SSX - hide semi-aspects\n"
                                "• CON, OPP, SQR - show only soft aspects");

    m_applyButton = new QPushButton("Apply Filter", this);
    m_applyButton->setToolTip("Depending on the volume of data\nfiltering may take some time.\nPlease be patient!");
    mainLayout->addWidget(m_dateFilter);
    mainLayout->addWidget(m_transitPlanetFilter);
    mainLayout->addWidget(m_aspectFilter);
    mainLayout->addWidget(m_natalPlanetFilter);
    mainLayout->addWidget(m_maxOrbFilter);

    mainLayout->addWidget(m_excludeFilter);

    mainLayout->addWidget(m_applyButton);

    connect(m_applyButton, &QPushButton::clicked, this, &TransitSearchDialog::emitFilter);

    m_clearButton = new QPushButton("Clear All", this);

    mainLayout->addWidget(m_clearButton);
    connect(m_clearButton, &QPushButton::clicked, this, &TransitSearchDialog::clearFilters);

    // status label

    // --- Add status label at the bottom ---
    statusLabel = new QLabel(this);
    statusLabel->setText(""); // Initially empty
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #2980b9; font-weight: bold;"); // Optional styling
    mainLayout->addWidget(statusLabel);
}

void TransitSearchDialog::emitFilter() {
    emit filterChanged(m_dateFilter->text(),
                       m_transitPlanetFilter->text(),
                       m_aspectFilter->text(),
                       m_natalPlanetFilter->text(),
                       m_maxOrbFilter->text(),

                       m_excludeFilter->text());

}

void TransitSearchDialog::clearFilters() {
    m_dateFilter->clear();
    m_transitPlanetFilter->clear();
    m_aspectFilter->clear();
    m_natalPlanetFilter->clear();
    m_maxOrbFilter->clear();

    m_excludeFilter->clear();

    emitFilter();  // This will apply the empty filters
}

