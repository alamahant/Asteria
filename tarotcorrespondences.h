#ifndef TAROTCORRESPONDENCES_H
#define TAROTCORRESPONDENCES_H

#include <QString>

namespace TarotCorrespondences {

int planetMajorNumber(const QString &planetId);
int signMajorNumber(const QString &sign);
int decanMinorNumber(const QString &sign, int decanIndex);
QString cardName(int number);
int courtCardNumber(const QString &sign, int decanIndex);
int aceForSign(const QString &sign);
int pageForSign(const QString &sign);
} // namespace TarotCorrespondences

#endif
