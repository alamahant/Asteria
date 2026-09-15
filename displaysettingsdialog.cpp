#include "displaysettingsdialog.h"
#include "Globals.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QMessageBox>

DisplaySettingsDialog::DisplaySettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Display Settings"));

    QSettings s;

    m_chartSizeSpin = new QSpinBox(this);
    //m_chartSizeSpin->setToolTip("Set Default Chart Size");
    m_chartSizeSpin->setRange(400, 1200);
    m_chartSizeSpin->setSingleStep(10);
    m_chartSizeSpin->setSuffix(" px");
    m_chartSizeSpin->setValue(s.value("display/chartSize", AsteriaFlags::chartSize).toInt());

    m_wheelThicknessSpin = new QSpinBox(this);
    //m_wheelThicknessSpin->setToolTip("Set Chart Wheel Thickness");

    m_wheelThicknessSpin->setRange(10, 80);
    m_wheelThicknessSpin->setSingleStep(1);
    m_wheelThicknessSpin->setSuffix(" px");
    m_wheelThicknessSpin->setValue(s.value("display/wheelThickness", AsteriaFlags::wheelThickness).toInt());

    m_planetSizeSpin = new QSpinBox(this);
    //m_planetSizeSpin->setToolTip("Set Planet Size");

    m_planetSizeSpin->setRange(10, 60);
    m_planetSizeSpin->setSingleStep(1);
    m_planetSizeSpin->setSuffix(" px");
    m_planetSizeSpin->setValue(s.value("display/planetSize", AsteriaFlags::planetSize).toInt());

    m_pointSizeSpin = new QSpinBox(this);
    //m_pointSizeSpin->setToolTip("Sep Planet Glyph Size");

    m_pointSizeSpin->setRange(8, 32);
    m_pointSizeSpin->setSingleStep(1);
    m_pointSizeSpin->setSuffix(" pt");
    m_pointSizeSpin->setValue(s.value("display/pointSize", AsteriaFlags::pointSize).toInt());

    m_uiFontSizeSpin = new QSpinBox(this);
    //m_uiFontSizeSpin->setToolTip("Set UI Font Size");
    m_uiFontSizeSpin->setRange(8, 24);
    m_uiFontSizeSpin->setSingleStep(1);
    m_uiFontSizeSpin->setSuffix(" pt");
    m_uiFontSizeSpin->setValue(s.value("display/uiFontSize", AsteriaFlags::uiFontSize).toInt());

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("Chart size:"),          m_chartSizeSpin);
    form->addRow(tr("Wheel thickness:"),     m_wheelThicknessSpin);
    form->addRow(tr("Planet size:"),         m_planetSizeSpin);
    form->addRow(tr("Planet glyph size:"),   m_pointSizeSpin);
    form->addRow(tr("UI font size:"),        m_uiFontSizeSpin);

    QLabel *note = new QLabel(tr("\nChanges take effect after restarting Asteria.\n"
                                 "Ctrl + mousewheel to zoom in/out.\n"), this);
    note->setWordWrap(true);

    QPushButton *defaultsBtn = new QPushButton(tr("Restore Defaults"), this);
    QPushButton *cancelBtn   = new QPushButton(tr("Cancel"), this);
    QPushButton *okBtn       = new QPushButton(tr("OK"), this);
    okBtn->setDefault(true);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(defaultsBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addWidget(note);
    main->addLayout(btnLayout);

    connect(defaultsBtn, &QPushButton::clicked, this, &DisplaySettingsDialog::restoreDefaults);
    connect(cancelBtn,   &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn,       &QPushButton::clicked, this, &DisplaySettingsDialog::saveAndClose);
}


void DisplaySettingsDialog::restoreDefaults()
{
    m_chartSizeSpin->setValue(AsteriaFlags::chartSizeDefault);
    m_wheelThicknessSpin->setValue(AsteriaFlags::wheelThicknessDefault);
    m_planetSizeSpin->setValue(AsteriaFlags::planetSizeDefault);
    m_pointSizeSpin->setValue(AsteriaFlags::pointSizeDefault);
    m_uiFontSizeSpin->setValue(AsteriaFlags::uiFontSizeDefault);
}

void DisplaySettingsDialog::saveAndClose()
{
    QSettings s;
    s.setValue("display/chartSize",      m_chartSizeSpin->value());
    s.setValue("display/wheelThickness", m_wheelThicknessSpin->value());
    s.setValue("display/planetSize",     m_planetSizeSpin->value());
    s.setValue("display/pointSize",      m_pointSizeSpin->value());
    s.setValue("display/uiFontSize",     m_uiFontSizeSpin->value());
    s.sync();

    QMessageBox::information(this, tr("Display Settings"),
                             tr("Settings saved. Restart Asteria to apply."));
    accept();
}
