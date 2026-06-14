#ifndef SOCIALSHARE_H
#define SOCIALSHARE_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QUrl>
#include <QHash>

class SocialShare : public QObject
{
    Q_OBJECT

public:
    enum Platform {
        Twitter,
        Facebook,
        Reddit,
        WhatsApp,
        Telegram,
        Instagram,
        Email,
        CopyToClipboard,
        SaveToFile,
        OpenFolder
    };
    Q_ENUM(Platform)

    explicit SocialShare(QObject *parent = nullptr);

    // Main sharing methods
    void shareTo(Platform platform, const QString &text, const QPixmap &image = QPixmap());
    void shareTo(Platform platform, const QString &text, const QString &imagePath);
    
    // Configurable settings
    void setAppName(const QString &name);
    void setScreenshotDirectory(const QString &path);
    void setIncludeScreenshot(bool include);
    
    // Helper for bulk share buttons
    QHash<Platform, QString> platformNames() const;
    QString platformDisplayName(Platform platform) const;

signals:
    void shareStarted(Platform platform);
    void shareCompleted(Platform platform, bool success, const QString &message);

private slots:
    void onCopyToClipboard(const QString &text, const QPixmap &image);
    void onSaveToFile(const QString &text, const QPixmap &image);
    void onOpenFolder(const QPixmap &image);
    
private:
    QString m_appName;
    QString m_screenshotDir;
    bool m_includeScreenshot;
    
    QString generateShareText(const QString &text) const;
    QString generateFilename() const;
    QString saveTempImage(const QPixmap &image) const;
    
    void shareToTwitter(const QString &text, const QPixmap &image);
    void shareToFacebook(const QString &text);
    void shareToReddit(const QString &text, const QPixmap &image);
    void shareToWhatsApp(const QString &text, const QPixmap &image);
    void shareToTelegram(const QString &text, const QPixmap &image);
    void shareViaEmail(const QString &text, const QPixmap &image);
    void shareToInstagram(const QString &text, const QPixmap &image);


};

#endif // SOCIALSHARE_H
