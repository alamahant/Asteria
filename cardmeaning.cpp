#include "cardmeaning.h"

CardMeaning::CardMeaning() {}

CardMeaning::CardMeaning(const QString &name, int rank, const QString &suit)
     : name(name), rank(rank), suit(suit)
{

}

void CardMeaning::setMeanings(const QStringList& light, const QStringList& shadow) {
    lightMeanings = light;
    shadowMeanings = shadow;
}

QMap<int, CardMeaning> CardMeaning::loadFromJson(const QString& jsonPath) {
    QMap<int, CardMeaning> meanings;
    QFile file(jsonPath);
    int counter = 0;  // This will map directly to our 0-77 card numbers

    if (file.open(QIODevice::ReadOnly)) {

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray cards = doc["tarot_interpretations"].toArray();

        for (const QJsonValue& card : cards) {
            QJsonObject cardObj = card.toObject();
            CardMeaning meaning;
            meaning.name = cardObj["name"].toString();

            QJsonArray keywordsArray = cardObj["keywords"].toArray();
            for(const QJsonValue& keyword : keywordsArray) {
                meaning.keywords.append(keyword.toString());
            }

            QJsonObject meaningsObj = cardObj["meanings"].toObject();
            QJsonArray lightArray = meaningsObj["light"].toArray();
            QJsonArray shadowArray = meaningsObj["shadow"].toArray();

            for(const QJsonValue& light : lightArray) {
                meaning.lightMeanings.append(light.toString());
            }

            for(const QJsonValue& shadow : shadowArray) {
                meaning.shadowMeanings.append(shadow.toString());
            }


            meanings[counter] = meaning;
            counter++;
        }
    }
    return meanings;
}
