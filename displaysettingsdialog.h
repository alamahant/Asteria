#ifndef DISPLAYSETTINGSDIALOG_H
#define DISPLAYSETTINGSDIALOG_H

#include <QDialog>

class QSpinBox;

class DisplaySettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DisplaySettingsDialog(QWidget *parent = nullptr);

private slots:
    void restoreDefaults();
    void saveAndClose();

private:
    QSpinBox *m_chartSizeSpin;
    QSpinBox *m_wheelThicknessSpin;
    QSpinBox *m_planetSizeSpin;
    QSpinBox *m_pointSizeSpin;
    QSpinBox *m_uiFontSizeSpin;
};

#endif // DISPLAYSETTINGSDIALOG_H