#include "rssnotificationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QXmlStreamReader>
#include <QSettings>
#include <QDebug>
#include <QSet>
#include<QJsonDocument>
#include<QJsonArray>
#include<QJsonObject>
#include<QFile>
#include<QStandardPaths>
#include<QDir>

RssNotificationDialog::RssNotificationDialog(QWidget *parent)
    : QDialog(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_hasNewContent(false)
    , m_enabled(false)
    , m_currentIndex(0)
    , m_feedUrl("https://alamahant.github.io/Asteria/asteria-feed.xml")
{
    setWindowTitle("Notifications, Hints and Tips");
    setMinimumSize(600, 400);
    resize(700, 500);

    QSettings settings;
    m_enabled = settings.value("Rss/enabled", false).toBool();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *cardLayout = new QVBoxLayout();

    m_textBrowser = new QTextBrowser(this);
    cardLayout->addWidget(m_textBrowser);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_progressLabel);

    QHBoxLayout *navLayout = new QHBoxLayout();
    m_prevButton = new QPushButton("◀ Previous", this);
    m_nextButton = new QPushButton("Next ▶", this);
    navLayout->addWidget(m_prevButton);
    navLayout->addWidget(m_nextButton);
    cardLayout->addLayout(navLayout);

    QHBoxLayout *markLayout = new QHBoxLayout();
    m_markReadCheckBox = new QCheckBox("Mark as read", this);
    m_markReadButton = new QPushButton("Apply", this);
    markLayout->addWidget(m_markReadCheckBox);
    markLayout->addWidget(m_markReadButton);
    markLayout->addStretch();
    cardLayout->addLayout(markLayout);

    mainLayout->addLayout(cardLayout);

    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_enableCheckBox = new QCheckBox("Enable RSS feed", this);
    m_enableCheckBox->setChecked(m_enabled);
    connect(m_enableCheckBox, &QCheckBox::toggled, this, &RssNotificationDialog::onEnableToggled);
    bottomLayout->addWidget(m_enableCheckBox);

    bottomLayout->addStretch();

    m_refreshButton = new QPushButton("Refresh", this);
    connect(m_refreshButton, &QPushButton::clicked, this, &RssNotificationDialog::onRefreshClicked);
    bottomLayout->addWidget(m_refreshButton);

    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    bottomLayout->addWidget(m_closeButton);

    mainLayout->addLayout(bottomLayout);

    connect(m_prevButton, &QPushButton::clicked, this, &RssNotificationDialog::onPrevClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &RssNotificationDialog::onNextClicked);
    connect(m_markReadButton, &QPushButton::clicked, this, &RssNotificationDialog::onMarkCurrentRead);


    loadCache();
    if (m_enabled && shouldCheckForUpdates()) {
        fetchFeed();
    }
}

RssNotificationDialog::~RssNotificationDialog()
{
}

/*
void RssNotificationDialog::checkForUpdates()
{
    if (m_enabled) {
        fetchFeed();
    }
}
*/

void RssNotificationDialog::fetchFeed()
{
    QSettings settings;
    settings.setValue("Rss/lastCheck", QDateTime::currentDateTime().toString(Qt::ISODate));
    QUrl url(m_feedUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &RssNotificationDialog::onReplyFinished);
}

void RssNotificationDialog::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        parseFeed(reply->readAll());
    } else {
        m_textBrowser->setHtml("<p style='color:red;'>Failed to fetch feed: " +
                               reply->errorString() + "</p>");
    }
    reply->deleteLater();
}

void RssNotificationDialog::parseFeed(const QByteArray &data)
{
    m_items.clear();
    m_readIndices.clear();
    QXmlStreamReader xml(data);

    bool foundNew = false;
    int newIndex = 0;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "item") {
            RssItem item;
            item.isNew = false;

            while (!(xml.isEndElement() && xml.name() == "item")) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == "title") {
                        item.title = xml.readElementText();
                    } else if (xml.name() == "description") {
                        item.description = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                    } else if (xml.name() == "link") {
                        item.link = xml.readElementText();
                    } else if (xml.name() == "pubDate") {
                        QString dateStr = xml.readElementText();
                        item.pubDate = QDateTime::fromString(dateStr, Qt::RFC2822Date);
                        if (!item.pubDate.isValid()) {
                            item.pubDate = QDateTime::fromString(dateStr, Qt::ISODate);
                        }
                    }
                }
            }

            item.isNew = true;
            foundNew = true;

            m_items.append(item);
        }
    }

    if (xml.hasError()) {
        m_textBrowser->setHtml("<p style='color:red;'>Error parsing feed: " +
                               xml.errorString() + "</p>");
        return;
    }

    loadReadStatus();

    bool hasUnread = false;
    for (int i = 0; i < m_items.size(); ++i) {
        if (!m_readIndices.contains(i)) {
            hasUnread = true;
            break;
        }
    }

    bool previousHasNew = m_hasNewContent;
    m_hasNewContent = hasUnread;

    if (previousHasNew != m_hasNewContent) {
        emit newContentAvailable(m_hasNewContent);
    }

    m_currentIndex = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        if (!m_readIndices.contains(i)) {
            m_currentIndex = i;
            break;
        }
    }
    saveCache();
    updateDisplay();
}

void RssNotificationDialog::updateDisplay()
{
    if (m_items.isEmpty()) {
        m_textBrowser->setHtml("<p>No teachings or events at this time. Check back later.</p>");
        m_progressLabel->setText("");
        m_prevButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_markReadCheckBox->setEnabled(false);
        m_markReadButton->setEnabled(false);
        return;
    }

    m_progressLabel->setText(QString("Item %1 of %2")
                             .arg(m_currentIndex + 1)
                             .arg(m_items.size()));

    m_prevButton->setEnabled(m_currentIndex > 0);
    m_nextButton->setEnabled(m_currentIndex < m_items.size() - 1);

    m_markReadCheckBox->setEnabled(true);
    m_markReadButton->setEnabled(true);

    const RssItem &item = m_items[m_currentIndex];

    QString isReadMark = m_readIndices.contains(m_currentIndex) ? " ✓ (Read)" : "";
    QString html = "<html><body>";
    html += "<div style='padding: 20px;'>";
    html += "<h2>" + item.title.toHtmlEscaped() + isReadMark + "</h2>";
    html += "<p style='font-size: 1.1em; margin-top: 20px;'>" + item.description + "</p>";
    html += "<p style='color: #888; margin-top: 20px;'>📅 " +
            item.pubDate.toString("ddd, dd MMM yyyy HH:mm") + " GMT</p>";
    if (!item.link.isEmpty()) {
        html += "<p><a href='" + item.link.toHtmlEscaped() +
                "' style='color: #3a6ea5;'>🔗 Open Link</a></p>";
    }
    html += "</div></body></html>";

    m_textBrowser->setHtml(html);
    m_textBrowser->setOpenExternalLinks(true);

    m_markReadCheckBox->setChecked(m_readIndices.contains(m_currentIndex));
    m_markReadCheckBox->setText("Mark as read");  // Always say "Mark as read"
}

void RssNotificationDialog::onPrevClicked()
{
    if (m_currentIndex > 0) {
        m_currentIndex--;
        updateDisplay();
    }
}

void RssNotificationDialog::onNextClicked()
{
    if (m_currentIndex < m_items.size() - 1) {
        m_currentIndex++;
        updateDisplay();
    }
}


void RssNotificationDialog::onMarkCurrentRead()
{
    bool wantRead = m_markReadCheckBox->isChecked();
    bool isCurrentlyRead = m_readIndices.contains(m_currentIndex);

    if (wantRead && !isCurrentlyRead) {
        m_readIndices.insert(m_currentIndex);
    } else if (!wantRead && isCurrentlyRead) {
        m_readIndices.remove(m_currentIndex);
    } else {
        return;
    }

    saveReadStatus();
    updateDisplay();

    bool hasUnread = false;
    for (int i = 0; i < m_items.size(); ++i) {
        if (!m_readIndices.contains(i)) {
            hasUnread = true;
            break;
        }
    }

    if (m_hasNewContent != hasUnread) {
        m_hasNewContent = hasUnread;
        emit newContentAvailable(m_hasNewContent);
    }
}

void RssNotificationDialog::saveReadStatus()
{
    QSettings settings;
    QStringList readIds;
    QList<int> indices = m_readIndices.values();
    std::sort(indices.begin(), indices.end());
    for (int i : indices) {
        readIds.append(QString::number(i));
    }
    settings.setValue("Rss/readIndices", readIds.join(","));
}

void RssNotificationDialog::loadReadStatus()
{
    QSettings settings;
    m_readIndices.clear();
    QString readStr = settings.value("Rss/readIndices", "").toString();
    if (readStr.isEmpty()) return;
    QStringList parts = readStr.split(",");
    for (const QString &part : parts) {
        bool ok;
        int idx = part.toInt(&ok);
        if (ok && idx >= 0 && idx < m_items.size()) {
            m_readIndices.insert(idx);
        }
    }
}

void RssNotificationDialog::markAsRead()
{
    for (int i = 0; i < m_items.size(); ++i) {
        m_readIndices.insert(i);
    }
    saveReadStatus();

    bool previousHasNew = m_hasNewContent;
    m_hasNewContent = false;

    if (previousHasNew != m_hasNewContent) {
        emit newContentAvailable(false);
    }

    updateDisplay();
}

void RssNotificationDialog::onEnableToggled(bool enabled)
{
    m_enabled = enabled;
    QSettings settings;
    settings.setValue("Rss/enabled", enabled);

    if (enabled) {
        fetchFeed();
    }
}

void RssNotificationDialog::onRefreshClicked()
{
    if (m_enabled) {
        QSettings settings;
        settings.setValue("Rss/lastCheck", QDateTime::currentDateTime().toString(Qt::ISODate));
        fetchFeed();
    }
}

bool RssNotificationDialog::shouldCheckForUpdates() const
{
    QSettings settings;
    QString lastCheck = settings.value("Rss/lastCheck").toString();

    if (lastCheck.isEmpty()) {
        return true;  // Never checked before
    }

    QDateTime last = QDateTime::fromString(lastCheck, Qt::ISODate);
    qint64 hours = last.secsTo(QDateTime::currentDateTime()) / 3600;

    return hours >= 24;  // Only check if 24+ hours have passed

}

QString RssNotificationDialog::getCacheFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataPath + "/rss_cache.json";
}

void RssNotificationDialog::loadCache()
{
    QFile file(getCacheFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return;

    m_items.clear();
    for (const QJsonValue& val : doc.array()) {
        QJsonObject obj = val.toObject();
        RssItem item;
        item.title = obj["title"].toString();
        item.description = obj["description"].toString();
        item.link = obj["link"].toString();
        item.pubDate = QDateTime::fromString(obj["pubDate"].toString(), Qt::ISODate);
        m_items.append(item);
    }

    loadReadStatus();
    updateDisplay();
}

void RssNotificationDialog::saveCache()
{
    QJsonArray itemsArray;
    for (const RssItem& item : m_items) {
        QJsonObject obj;
        obj["title"] = item.title;
        obj["description"] = item.description;
        obj["link"] = item.link;
        obj["pubDate"] = item.pubDate.toString(Qt::ISODate);
        itemsArray.append(obj);
    }

    QFile file(getCacheFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(itemsArray).toJson(QJsonDocument::Indented));
        file.close();
    }
}

