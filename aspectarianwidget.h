#ifndef ASPECTARIANWIDGET_H
#define ASPECTARIANWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "chartcalculator.h"

class AspectarianWidget : public QWidget {
    Q_OBJECT
public:
    explicit AspectarianWidget(QWidget *parent = nullptr);
    void updateData(const ChartData &chartData);
    QString planetSymbol(const QString &planetName);


private:
    QTableWidget *m_table;
    QLabel *m_titleLabel;

    void setupUi();
    QColor aspectColor(const QString &aspectType);
    QString aspectSymbol(const QString &aspectType);
};

#endif // ASPECTARIANWIDGET_H




