// aspectsearchdialog.h
#ifndef ASPECTSEARCHDIALOG_H
#define ASPECTSEARCHDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class AspectSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AspectSearchDialog(QWidget *parent = nullptr);

    QLabel *statusLabel;

signals:
    void filterChanged(const QString &planet1Pattern,
                       const QString &aspectPattern,
                       const QString &planet2Pattern,
                       const QString &maxOrbPattern,
                       const QString &excludePattern);

private slots:
    void emitFilter();
    void clearFilters();

private:
    QLineEdit *m_planet1Filter;
    QLineEdit *m_aspectFilter;
    QLineEdit *m_planet2Filter;
    QLineEdit *m_maxOrbFilter;
    QLineEdit *m_excludeFilter;
    QPushButton *m_applyButton;
    QPushButton *m_clearButton;
};

#endif // ASPECTSEARCHDIALOG_H