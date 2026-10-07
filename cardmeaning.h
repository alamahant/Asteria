#ifndef CARDMEANING_H
#define CARDMEANING_H

#include<QString>
#include<QStringList>
#include<QFile>
#include<QJsonObject>
#include<QJsonArray>
#include<QJsonDocument>

class CardMeaning
{
public:
    CardMeaning();
    CardMeaning(const QString& name, int rank, const QString& suit);
    // Getters and setters
    QString getName() const { return name; }
    QStringList getKeywords() const { return keywords; }
    QStringList getLightMeanings() const { return lightMeanings; }
    QStringList getShadowMeanings() const { return shadowMeanings; }
    void setMeanings(const QStringList& light, const QStringList& shadow);
    static QMap<int, CardMeaning> loadFromJson(const QString& jsonPath);
private:
    QString name;
    int rank;
    QString suit;
    QStringList keywords;
    QStringList fortuneTelling;
    QStringList lightMeanings;    // upright
    QStringList shadowMeanings;   // reversed
};

#endif // CARDMEANING_H
