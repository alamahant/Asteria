#include "cardloader.h"
#include<QImageReader>
#include"Globals.h"

CardLoader::CardLoader(const QString& path) : cardPath(path) {
}



void CardLoader::loadCards()
{
    QDir dir(cardPath);
    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg";

    for(const QString& file : dir.entryList(filters)) {
        if (file.length() >= 6 && file[0].isDigit() && file[1].isDigit()) {
            int number = file.left(2).toInt();
            cardImages[number] = loadCrispPixmap(dir.filePath(file));
        }
    }
    preScaleCards();
}





QString CardLoader::formatName(const QString &rawName)
{
    if(rawName.contains("_")) {
        // Handle minor arcana
        QStringList parts = rawName.split("_");
        return QString("%1 of %2").arg(parts.last().at(0).toUpper() + parts.last().mid(1).toLower(),
                                       parts.first().at(0).toUpper() + parts.first().mid(1).toLower());
    } else {
        // Handle major arcana
        return QString("The %1").arg(rawName.at(0).toUpper() + rawName.mid(1).toLower());

    }
}




QPixmap CardLoader::getCardImage(int number) {
    //return cardImages.value(number).scaled(200, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // Return pre-scaled version if available
    if (scaledCardImages.contains(number)) {
        return scaledCardImages[number];
    }


    // Return empty pixmap if card not found
    qWarning() << "Card number not found:" << number;
    return QPixmap();

}



void CardLoader::preScaleCards() {
    // Pre-scale all cards for better performance
    const int h = AsteriaFlags::tarotCardHeight;
    const int w = h * 2 / 3;
    for (auto it = cardImages.begin(); it != cardImages.end(); ++it) {
        //scaledCardImages[it.key()] = it.value().scaled(getCardWidth(), getCardHeight(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        //scaledCardImages[it.key()] = it.value().scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        scaledCardImages[it.key()] = it.value().scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

}


void CardLoader::loadDeck(const QString& path)
{
    // Clear existing card images
    cardImages.clear();

    // Set the new path
    cardPath = path;

    // Load cards from the new path
    loadCards();
}

QPixmap CardLoader::loadCrispPixmap(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    reader.setQuality(100);

    QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << path << reader.errorString();
        return QPixmap();
    }

    // For high DPI displays
    qreal dpr = qApp->devicePixelRatio();
    if (!qFuzzyCompare(dpr, 1.0)) {
        image = image.scaled(image.size() * dpr,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
        image.setDevicePixelRatio(dpr);
    }

    return QPixmap::fromImage(image);

}
