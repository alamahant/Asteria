#ifndef ELEMENTMODALITYWIDGET_H
#define ELEMENTMODALITYWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include "chartcalculator.h"

class ElementModalityWidget : public QWidget {
    Q_OBJECT
public:
    explicit ElementModalityWidget(QWidget *parent = nullptr);
    void updateData(const ChartData &chartData);

private:
    QLabel *m_titleLabel;
    QGridLayout *m_gridLayout;

    // Grid cells for each sign
    QMap<QString, QLabel*> m_signLabels;

    // Totals for elements and modalities
    QLabel *m_fireTotal;
    QLabel *m_earthTotal;
    QLabel *m_airTotal;
    QLabel *m_waterTotal;
    QLabel *m_cardinalTotal;
    QLabel *m_fixedTotal;
    QLabel *m_mutableTotal;

    void setupUi();
    QString getElement(const QString &sign);
    QString getModality(const QString &sign);
    QColor getElementColor(const QString &element);
    QString getSignGlyph(const QString &sign);
    QString getPlanetGlyph(const QString &planetId);
};

#endif // ELEMENTMODALITYWIDGET_H
