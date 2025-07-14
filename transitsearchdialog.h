#ifndef TRANSITSEARCHDIALOG_H
#define TRANSITSEARCHDIALOG_H

#include <QDialog>
#include<QLineEdit>
#include<QCheckBox>
#include<QPushButton>
#include<QLabel>

class TransitSearchDialog : public QDialog {
    Q_OBJECT
public:
    TransitSearchDialog(QWidget *parent = nullptr);
    QLabel *statusLabel; // Public status label for 'Searching...' or 'OK'

signals:
    void filterChanged(const QString &datePattern,
                       const QString &transitPattern,
                       const QString &aspectPattern,
                       const QString &natalPattern,
                       const QString &maxOrb,

                       const QString &excludePattern);

private:
    QLineEdit *m_dateFilter;
    QLineEdit *m_transitPlanetFilter;
    QLineEdit *m_aspectFilter;
    QLineEdit *m_natalPlanetFilter;
    QLineEdit *m_maxOrbFilter;

    QLineEdit* m_excludeFilter;
    //QCheckBox *m_liveFilterCheck;
    QPushButton *m_applyButton;
    QPushButton *m_clearButton;
    void clearFilters();
private slots:
    void emitFilter();

};
#endif // TRANSITSEARCHDIALOG_H
