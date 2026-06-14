#include "socialshare.h"
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QBuffer>
#include <QMessageBox>
#include<QCoreApplication>
#include"Globals.h"

SocialShare::SocialShare(QObject *parent)
    : QObject(parent)
    , m_appName(QCoreApplication::applicationName())
    , m_screenshotDir(GlobalFlags::sharesDirPath)
    , m_includeScreenshot(true)
{
}

void SocialShare::setAppName(const QString &name)
{
    m_appName = name;
}

void SocialShare::setScreenshotDirectory(const QString &path)
{
    m_screenshotDir = path;
    QDir dir;
    if (!dir.exists(m_screenshotDir)) {
        dir.mkpath(m_screenshotDir);
    }
}

void SocialShare::setIncludeScreenshot(bool include)
{
    m_includeScreenshot = include;
}

void SocialShare::shareTo(Platform platform, const QString &text, const QPixmap &image)
{
    emit shareStarted(platform);
    
    switch (platform) {
    case Twitter:
        shareToTwitter(text, image);
        break;
    case Facebook:
        shareToFacebook(text);
        break;
    case Instagram:
        shareToInstagram(text, image);
        break;
    case WhatsApp:
        shareToWhatsApp(text, image);
        break;

    case Reddit:
        shareToReddit(text, image);
        break;
    case Telegram:
        shareToTelegram(text, image);
        break;
    case Email:
        shareViaEmail(text, image);
        break;
    case CopyToClipboard:
        onCopyToClipboard(text, image);
        break;
    case SaveToFile:
        onSaveToFile(text, image);
        break;
    case OpenFolder:
        onOpenFolder(image);
        break;
    }
}

void SocialShare::shareTo(Platform platform, const QString &text, const QString &imagePath)
{
    QPixmap image(imagePath);
    shareTo(platform, text, image);
}

QHash<SocialShare::Platform, QString> SocialShare::platformNames() const
{
    return {
        {Twitter, "Twitter/X"},
        {Facebook, "Facebook"},
        {Reddit, "Reddit"},
        {WhatsApp, "WhatsApp"},
        {Instagram, "Instagram"},
        {Telegram, "Telegram"},
        {Email, "Email"},
        {CopyToClipboard, "Copy to Clipboard"},
        {SaveToFile, "Save as Image"},
        {OpenFolder, "Open Screenshots Folder"}
    };
}

QString SocialShare::platformDisplayName(Platform platform) const
{
    return platformNames().value(platform, "Unknown");
}

QString SocialShare::generateShareText(const QString &text) const
{
    QString baseText;
    if (text.isEmpty()) {
        baseText = QString("Check out my %1 Chart\n").arg(m_appName);
    } else {
        baseText = text;
    }

    QString githubLink = QString("https://github.com/alamahant/%1").arg(m_appName);
    QString poweredBy = QString("\n\n✨ %1\n%2").arg(m_appName).arg(githubLink);

    return baseText + poweredBy;
}

QString SocialShare::generateFilename() const
{
    return QString("%1/%2_%3.png")
        .arg(m_screenshotDir)
        .arg(m_appName.toLower().replace(" ", "_"))
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
}

QString SocialShare::saveTempImage(const QPixmap &image) const
{
    if (image.isNull()) {
        return QString();
    }
    
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString fileName = tempPath + "/share_" + 
                       QDateTime::currentDateTime().toString("yyyyMMdd_hhmmsszzz") + 
                       ".png";
    
    image.save(fileName, "PNG");
    return fileName;
}

void SocialShare::shareToTwitter(const QString &text, const QPixmap &image)
{
    //Q_UNUSED(image);
    QMessageBox::information(nullptr, "Share to X",
        "✓ Screenshot copied to clipboard\n\n"
        "X will now open.\n"
        "1. Create a new post\n"
        "2. Press Ctrl+V to paste the screenshot\n");
    QString shareText = generateShareText(text);
    QUrl url("https://twitter.com/intent/tweet");
    QUrlQuery query;
    query.addQueryItem("text", shareText);
    url.setQuery(query);
    
    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(Twitter, success, success ? "Opened Twitter" : "Failed to open Twitter");
}

void SocialShare::shareToFacebook(const QString &text)
{
    QMessageBox::information(nullptr, "Share to Facebook",
        "✓ Screenshot copied to clipboard\n\n"
        "Facebook will now open.\n"
        "1. Create a new post\n"
        "2. Press Ctrl+V to paste the screenshot\n");
    QString shareText = generateShareText(text);
    QUrl url("https://www.facebook.com/sharer/sharer.php");
    QUrlQuery query;
    //query.addQueryItem("quote", shareText);
    //query.addQueryItem("u", QString("https://github.com/alamahant/%1").arg(QCoreApplication::applicationName()));
    //url.setQuery(query);
    
    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(Facebook, success, success ? "Opened Facebook" : "Failed to open Facebook");
}

void SocialShare::shareToReddit(const QString &text, const QPixmap &image)
{
    Q_UNUSED(image);
    QMessageBox::information(nullptr, "Share to Reddit",
        "✓ Screenshot copied to clipboard\n\n"
        "Reddit will now open.\n"
        "1. Create a new post\n"
        "2. Press Ctrl+V to paste the screenshot\n");
    QString shareText = generateShareText(text);
    QUrl url("https://www.reddit.com/submit");
    QUrlQuery query;
    query.addQueryItem("title", shareText);
    url.setQuery(query);
    
    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(Reddit, success, success ? "Opened Reddit" : "Failed to open Reddit");
}

void SocialShare::shareToWhatsApp(const QString &text, const QPixmap &image)
{
    QMessageBox::information(nullptr, "Share to WhatsApp",
        "✓ Screenshot copied to clipboard\n\n"
        "WhatsApp will now open.\n"
        "1. Create a new post\n"
        "2. Press Ctrl+V to paste the screenshot\n");
    QString shareText = generateShareText(text);
    QUrl url("https://wa.me/");
    QUrlQuery query;
    query.addQueryItem("text", shareText);
    url.setQuery(query);

    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(WhatsApp, success, success ? "Opened WhatsApp" : "Failed to open WhatsApp");
}

void SocialShare::shareToTelegram(const QString &text, const QPixmap &image)
{
    QMessageBox::information(nullptr, "Share to Telegram",
        "✓ Screenshot copied to clipboard\n\n"
        "Telegram will now open.\n"
        "1. Create a new post\n"
        "2. Press Ctrl+V to paste the screenshot\n");
    QString shareText = generateShareText(text);
    QUrl url("https://t.me/share/url");
    QUrlQuery query;
    query.addQueryItem("text", shareText);
    url.setQuery(query);
    
    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(Telegram, success, success ? "Opened Telegram" : "Failed to open Telegram");
}

void SocialShare::shareViaEmail(const QString &text, const QPixmap &image)
{
    Q_UNUSED(image);

    QString shareText = generateShareText(text);
    QString subject = QString("%1 Reading").arg(m_appName);

    // Properly encode
    QString encodedSubject = QUrl::toPercentEncoding(subject);
    QString encodedBody = QUrl::toPercentEncoding(shareText);

    // Use "?to=" to force Thunderbird to recognize it as compose
    QUrl url(QString("mailto:?to=&subject=%1&body=%2").arg(encodedSubject).arg(encodedBody));

    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(Email, success,
        success ? "Email client opened" : "Failed to open email client");
}

void SocialShare::shareToInstagram(const QString &text, const QPixmap &image)
{
    Q_UNUSED(image);

    QMessageBox::information(nullptr, "Share to Instagram",
        "✓ Screenshot copied to clipboard\n\n"
        "Instagram will now open.\n"
        "1. Create a new post\n"
        "2. Press Ctrl+V to paste the screenshot\n");

    QString shareText = generateShareText(text);
    QUrl url("https://www.instagram.com");
    //QUrl url("https://www.instagram.com/create/post");
    bool success = QDesktopServices::openUrl(url);
    emit shareCompleted(Instagram, success, success ? "Opened Instagram" : "Failed to open Instagram");
}


void SocialShare::onCopyToClipboard(const QString &text, const QPixmap &image)
{
    QClipboard *clipboard = QApplication::clipboard();
    
    if (!text.isEmpty()) {
        clipboard->setText(text);
    }
    
    if (!image.isNull()) {
        clipboard->setImage(image.toImage());
    }
    
    QString message = text.isEmpty() && image.isNull() ? "Nothing to copy" : "Copied to clipboard";
    emit shareCompleted(CopyToClipboard, true, message);
}

void SocialShare::onSaveToFile(const QString &text, const QPixmap &image)
{
    if (image.isNull()) {
        emit shareCompleted(SaveToFile, false, "No image to save");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(nullptr, 
        "Save Screenshot",
        generateFilename(),
        "PNG Image (*.png);;All Files (*)");
    
    if (fileName.isEmpty()) {
        emit shareCompleted(SaveToFile, false, "Save cancelled");
        return;
    }
    
    bool success = image.save(fileName, "PNG");
    QString message = success ? QString("Saved to %1").arg(fileName) : "Failed to save";
    emit shareCompleted(SaveToFile, success, message);
}

void SocialShare::onOpenFolder(const QPixmap &image)
{
    Q_UNUSED(image);
    
    QUrl folderUrl = QUrl::fromLocalFile(m_screenshotDir);
    bool success = QDesktopServices::openUrl(folderUrl);
    emit shareCompleted(OpenFolder, success, success ? "Opened folder" : "Failed to open folder");
}
