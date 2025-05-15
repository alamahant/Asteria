#include "aspectsettingsdialog.h"
#include "Globals.h" // Include your globals file with AspectSettings

#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
AspectSettingsDialog::AspectSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Aspect Display Settings");
    setupUI();
    loadCurrentSettings();
}

void AspectSettingsDialog::saveSettings()
{
    // Save show/hide setting
    AspectSettings::instance().setShowAspectLines(m_showAspectsCheckbox->isChecked());

    // Save major aspect settings
    AspectSettings::instance().setMajorAspectWidth(m_majorWidthSpinBox->value());
    AspectSettings::instance().setMajorAspectStyle(penStyleFromIndex(m_majorStyleCombo->currentIndex()));

    // Save minor aspect settings
    AspectSettings::instance().setMinorAspectWidth(m_minorWidthSpinBox->value());
    AspectSettings::instance().setMinorAspectStyle(penStyleFromIndex(m_minorStyleCombo->currentIndex()));
    //use mainwindows settings functionality
    QSettings settings;
    AspectSettings::instance().saveToSettings(settings);

    accept();
}

void AspectSettingsDialog::resetDefaults()
{
    AspectSettings::instance().resetToDefaults();

    // Save defaults to disk
    QSettings settings;
    AspectSettings::instance().saveToSettings(settings);

    loadCurrentSettings();
}

void AspectSettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Show/Hide all aspects
    m_showAspectsCheckbox = new QCheckBox("Show Aspect Lines");
    mainLayout->addWidget(m_showAspectsCheckbox);

    // Create grid layout for settings
    QGridLayout* gridLayout = new QGridLayout();
    int row = 0;

    // Major aspect settings
    gridLayout->addWidget(new QLabel("<b>Major Aspects</b>"), row++, 0, 1, 2);

    gridLayout->addWidget(new QLabel("Line Width:"), row, 0);
    m_majorWidthSpinBox = new QDoubleSpinBox();
    m_majorWidthSpinBox->setRange(0.5, 5.0);
    m_majorWidthSpinBox->setSingleStep(0.5);
    gridLayout->addWidget(m_majorWidthSpinBox, row++, 1);

    gridLayout->addWidget(new QLabel("Line Style:"), row, 0);
    m_majorStyleCombo = new QComboBox();
    populateStyleCombo(m_majorStyleCombo);
    gridLayout->addWidget(m_majorStyleCombo, row++, 1);

    // Add some spacing
    gridLayout->addWidget(new QLabel(""), row++, 0);

    // Minor aspect settings
    gridLayout->addWidget(new QLabel("<b>Minor Aspects</b>"), row++, 0, 1, 2);

    gridLayout->addWidget(new QLabel("Line Width:"), row, 0);
    m_minorWidthSpinBox = new QDoubleSpinBox();
    m_minorWidthSpinBox->setRange(0.5, 5.0);
    m_minorWidthSpinBox->setSingleStep(0.5);
    gridLayout->addWidget(m_minorWidthSpinBox, row++, 1);

    gridLayout->addWidget(new QLabel("Line Style:"), row, 0);
    m_minorStyleCombo = new QComboBox();
    populateStyleCombo(m_minorStyleCombo);
    gridLayout->addWidget(m_minorStyleCombo, row++, 1);

    mainLayout->addLayout(gridLayout);

    // Add some spacing
    mainLayout->addStretch();

    // Buttons at the bottom
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* resetButton = new QPushButton("Reset to Defaults");
    connect(resetButton, &QPushButton::clicked, this, &AspectSettingsDialog::resetDefaults);
    buttonLayout->addWidget(resetButton);

    buttonLayout->addStretch();

    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);

    QPushButton* saveButton = new QPushButton("Save");
    connect(saveButton, &QPushButton::clicked, this, &AspectSettingsDialog::saveSettings);
    buttonLayout->addWidget(saveButton);

    mainLayout->addLayout(buttonLayout);

    // Set a reasonable size
    resize(350, 300);
}

void AspectSettingsDialog::populateStyleCombo(QComboBox* combo)
{
    combo->addItem("Solid Line");
    combo->addItem("Dash Line");
    combo->addItem("Dot Line");
    combo->addItem("Dash Dot Line");
    combo->addItem("Dash Dot Dot Line");
    combo->addItem("No Pen");
}

void AspectSettingsDialog::loadCurrentSettings()
{
    // Load show/hide setting
    m_showAspectsCheckbox->setChecked(AspectSettings::instance().getShowAspectLines());

    // Load major aspect settings
    m_majorWidthSpinBox->setValue(AspectSettings::instance().getMajorAspectWidth());
    m_majorStyleCombo->setCurrentIndex(penStyleToIndex(AspectSettings::instance().getMajorAspectStyle()));

    // Load minor aspect settings
    m_minorWidthSpinBox->setValue(AspectSettings::instance().getMinorAspectWidth());
    m_minorStyleCombo->setCurrentIndex(penStyleToIndex(AspectSettings::instance().getMinorAspectStyle()));
}

int AspectSettingsDialog::penStyleToIndex(Qt::PenStyle style)
{
    switch (style) {
    case Qt::SolidLine: return 0;
    case Qt::DashLine: return 1;
    case Qt::DotLine: return 2;
    case Qt::DashDotLine: return 3;
    case Qt::DashDotDotLine: return 4;
    case Qt::NoPen: return 5;
    default: return 0;
    }
}

Qt::PenStyle AspectSettingsDialog::penStyleFromIndex(int index)
{
    switch (index) {
    case 0: return Qt::SolidLine;
    case 1: return Qt::DashLine;
    case 2: return Qt::DotLine;
    case 3: return Qt::DashDotLine;
    case 4: return Qt::DashDotDotLine;
    case 5: return Qt::NoPen;
    default: return Qt::SolidLine;
    }
}

