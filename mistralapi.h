#ifndef MISTRALAPI_H
#define MISTRALAPI_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>

class MistralAPI : public QObject
{
    Q_OBJECT
public:
    explicit MistralAPI(QObject *parent = nullptr);
    ~MistralAPI();

    // API key management
    void setApiKey(const QString &key);
    bool saveApiKey(const QString &key);
    bool loadApiKey();
    bool hasValidApiKey() const;
    void clearApiKey();

    // Chart interpretation
    void interpretChart(const QJsonObject &chartData);

    // Get the last error message
    QString getLastError() const;
    QString getApiKey() const { return m_apiKey; }


signals:
    void interpretationReady(const QString &interpretation);
    void error(const QString &errorMessage);
    void transitInterpretationReady(const QString &interpretation);


private slots:
    void handleNetworkReply(QNetworkReply *reply);

private:
    // Helper methods
    QJsonObject createPrompt(const QJsonObject &chartData);
    QString formatInterpretation(const QJsonObject &response);

    // Settings management
    QString getSettingsPath() const;

    // Network management
    QNetworkAccessManager *m_networkManager;

    // API configuration
    QString m_apiKey;
    QString m_apiEndpoint;
    QString m_model;

    // State
    QString m_lastError;
    bool m_requestInProgress;
    QString m_language;

public:
    QJsonObject createTransitPrompt(const QJsonObject &transitData);

public slots:
    void interpretTransits(const QJsonObject &transitData);
    void setLanguage(const QString& language) {
        m_language = language;
    }
};

#endif // MISTRALAPI_H
