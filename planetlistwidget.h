#ifndef PLANETLISTWIDGET_H
#define PLANETLISTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "chartcalculator.h"

class PlanetListWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlanetListWidget(QWidget *parent = nullptr);
    void updateData(const ChartData &chartData);

private:
    QTableWidget *m_table;
    QLabel *m_titleLabel;

    void setupUi();
    QString getSymbolForPlanet(const QString &planetId);
    QString getSymbolForSign(const QString &sign);
    QColor getColorForSign(const QString &sign);
};

#endif // PLANETLISTWIDGET_H
