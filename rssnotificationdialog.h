#ifndef RSSNOTIFICATIONDIALOG_H
#define RSSNOTIFICATIONDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QDateTime>
#include <QTextBrowser>
#include<QCheckBox>
#include<QLabel>

struct RssItem {
    QString title;
    QString description;
    QString link;
    QDateTime pubDate;
    bool isNew;
};

class RssNotificationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RssNotificationDialog(QWidget *parent = nullptr);
    ~RssNotificationDialog();

    // Called from MainWindow at startup
    //void checkForUpdates();

    // Returns true if new content was found since last check
    bool hasNewContent() const { return m_hasNewContent; }

    // Call this after user views the dialog
    void markAsRead();

signals:
    void newContentAvailable(bool hasNew);  // MainWindow listens to turn button red/gray

private slots:
    void fetchFeed();
    void onReplyFinished();
    void onEnableToggled(bool enabled);
    void onRefreshClicked();

private:
    void parseFeed(const QByteArray &data);
    void saveLastSeenDate(const QString &date);
    QString loadLastSeenDate() const;
    void updateDisplay();

    QNetworkAccessManager *m_networkManager;
    QList<RssItem> m_items;
    bool m_hasNewContent;
    bool m_enabled;
    QString m_feedUrl;
    QTextBrowser *m_textBrowser;
    QCheckBox *m_enableCheckBox;
    QPushButton *m_refreshButton;
    QPushButton *m_closeButton;

    //
private:
    int m_currentIndex;
    QLabel *m_progressLabel;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QCheckBox *m_markReadCheckBox;
    QPushButton *m_markReadButton;
    QSet<int> m_readIndices;

private slots:
    void onPrevClicked();
    void onNextClicked();
    void onMarkCurrentRead();

private:
    void saveReadStatus();
    void loadReadStatus();
     bool shouldCheckForUpdates() const;
     QString getCacheFilePath() const;
     void loadCache();
     void saveCache();
};

#endif // RSSNOTIFICATIONDIALOG_H
