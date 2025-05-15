#ifndef ASPECTSETTINGSDIALOG_H
#define ASPECTSETTINGSDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>

class AspectSettingsDialog : public QDialog {
    Q_OBJECT

public:
    AspectSettingsDialog(QWidget* parent = nullptr);

private slots:
    void saveSettings();
    void resetDefaults();

private:
    void setupUI();
    void populateStyleCombo(QComboBox* combo);
    void loadCurrentSettings();
    int penStyleToIndex(Qt::PenStyle style);
    Qt::PenStyle penStyleFromIndex(int index);

    QCheckBox* m_showAspectsCheckbox;
    QDoubleSpinBox* m_majorWidthSpinBox;
    QComboBox* m_majorStyleCombo;
    QDoubleSpinBox* m_minorWidthSpinBox;
    QComboBox* m_minorStyleCombo;
};

#endif // ASPECTSETTINGSDIALOG_H
