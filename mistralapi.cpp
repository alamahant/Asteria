#include "mistralapi.h"
#include <QDir>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDebug>
#include"Globals.h"

//#include<QNetworkRequest>
//#include<QByteArray>
MistralAPI::MistralAPI(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_apiEndpoint("https://api.mistral.ai/v1/chat/completions")
    , m_model("mistral-medium-latest")
    , m_requestInProgress(false)
{
    // Connect network reply signal
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &MistralAPI::handleNetworkReply);

    // Try to load API key from settings
    loadApiKey();
}

MistralAPI::~MistralAPI()
{
    // QObject parent-child relationship will handle deletion
}

void MistralAPI::setApiKey(const QString &key)
{
    m_apiKey = key;
}

bool MistralAPI::saveApiKey(const QString &key) {
    // Set the key in memory
    m_apiKey = key;

    // Save to settings using default path
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Asteria", "Asteria");

    settings.beginGroup("Authentication");
    settings.setValue("MistralApiKey", m_apiKey);
    settings.endGroup();

    // Force write to disk
    settings.sync();

    return settings.status() == QSettings::NoError;
}


bool MistralAPI::loadApiKey() {
    // Using default settings path since getSettingsPath() doesn't exist
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Asteria", "Asteria");

    settings.beginGroup("Authentication");
    m_apiKey = settings.value("MistralApiKey").toString();
    settings.endGroup();

    return !m_apiKey.isEmpty();
}

bool MistralAPI::hasValidApiKey() const
{
    return !m_apiKey.isEmpty();
}
/*
void MistralAPI::clearApiKey()
{
    m_apiKey.clear();

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    settings.beginGroup("Authentication");
    settings.remove("MistralApiKey");
    settings.endGroup();
}
*/

void MistralAPI::clearApiKey() {
    m_apiKey.clear();
    // Use the same approach as saveApiKey and loadApiKey
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Asteria", "Asteria");
    settings.beginGroup("Authentication");
    settings.remove("MistralApiKey");
    settings.endGroup();
}



void MistralAPI::interpretChart(const QJsonObject &chartData)
{
    if (m_requestInProgress) {
        m_lastError = "A request is already in progress";
        emit error(m_lastError);
        return;
    }

    if (!hasValidApiKey()) {
        m_lastError = "No API key set";
        emit error(m_lastError);
        return;
    }

    /*
    // Debug: Print the current chart type before making the request
    qDebug() << "Interpretation requested for chart type:" << GlobalFlags::lastGeneratedChartType;

    // For testing: stop here so you can check the output
    return;
    */


    // Create the prompt for Mistral
    QJsonObject prompt = createPrompt(chartData);

    // Prepare the network request - Fix: use braces instead of parentheses
    QUrl url(m_apiEndpoint);
    QNetworkRequest request{url};  // Using braces instead of parentheses
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // Convert prompt to JSON document
    QJsonDocument doc(prompt);
    QByteArray data = doc.toJson();

    // Send the request
    m_networkManager->post(request, data);
    m_requestInProgress = true;

}


void MistralAPI::handleNetworkReply(QNetworkReply *reply) {
    m_requestInProgress = false;

    // Check for network errors
    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = "Network error: " + reply->errorString();
        emit error(m_lastError);
        reply->deleteLater();
        return;
    }

    // Read and parse the response
    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        m_lastError = "Invalid JSON response";
        emit error(m_lastError);
        reply->deleteLater();
        return;
    }

    QJsonObject responseObj = doc.object();

    // Format the response
    QString formattedResponse = formatInterpretation(responseObj);
    if (formattedResponse.isEmpty()) {
        m_lastError = "Failed to extract response from API";
        emit error(m_lastError);
    } else {
        // Determine which signal to emit based on the request type
        if (reply->property("isTransitRequest").toBool()) {
            emit transitInterpretationReady(formattedResponse);
        } else {
            emit interpretationReady(formattedResponse);
        }
    }

    reply->deleteLater();
}

QString MistralAPI::formatInterpretation(const QJsonObject &response)
{
    // Extract the interpretation from the Mistral API response
    if (!response.contains("choices") || !response["choices"].isArray()) {
        return QString();
    }

    QJsonArray choices = response["choices"].toArray();
    if (choices.isEmpty() || !choices[0].isObject()) {
        return QString();
    }

    QJsonObject choice = choices[0].toObject();
    if (!choice.contains("message") || !choice["message"].isObject()) {
        return QString();
    }

    QJsonObject message = choice["message"].toObject();
    if (!message.contains("content") || !message["content"].isString()) {
        return QString();
    }

    return message["content"].toString();
}

QString MistralAPI::getSettingsPath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);

    // Create directory if it doesn't exist
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return appDataPath + "/settings.ini";
}

QString MistralAPI::getLastError() const
{
    return m_lastError;
}

///////////////////////Predictions

void MistralAPI::interpretTransits(const QJsonObject &transitData) {
    if (m_requestInProgress) {
        m_lastError = "A request is already in progress";
        emit error(m_lastError);
        return;
    }

    if (!hasValidApiKey()) {
        m_lastError = "No API key set";
        emit error(m_lastError);
        return;
    }

    // Create the prompt for Mistral
    QJsonObject prompt = createTransitPrompt(transitData);

    // Prepare the network request
    QUrl url(m_apiEndpoint);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // Convert prompt to JSON document
    QJsonDocument doc(prompt);
    QByteArray data = doc.toJson();

    // Send the request
    QNetworkReply *reply = m_networkManager->post(request, data);
    reply->setProperty("isTransitRequest", true);
    m_requestInProgress = true;

}

/////////////////////////////////////////////////////////////////////

QJsonObject MistralAPI::createPrompt(const QJsonObject &chartData) {
    // Create the messages array for the chat completion
    QJsonArray messages;

    // System message to instruct the model
    QJsonObject systemMessage;
    systemMessage["role"] = "system";

    /*
    // Add language instruction if not English
    if (m_language != "English") {
        systemMessage["content"] = QString("You are an expert astrologer providing detailed and insightful "
                                           "interpretations of %1 charts. Analyze the following chart data "
                                           "and provide a comprehensive reading covering personality traits, "
                                           "strengths, challenges, and life path insights. Be specific about "
                                           "what each planet position, house placement, and major aspect means for "
                                           "the individual. IMPORTANT: Your entire response must be in %2, using Markdown or plain text only. "
                                           "Do NOT output JSON, XML, YAML, or any other structured data formats.")
                                       .arg(GlobalFlags::lastGeneratedChartType)
                                       .arg(m_language);
    } else {
        systemMessage["content"] = QString("You are an expert astrologer providing detailed and insightful "
                                   "interpretations of %1 charts. Analyze the following chart data "
                                   "and provide a comprehensive reading covering personality traits, "
                                   "strengths, challenges, and life path insights. Be specific about "
                                   "what each planet position, house placement, and major aspect means for the individual. "
                                   "IMPORTANT: Your entire response must be in Markdown or plain text only. "
                                   "Do NOT output JSON, XML, YAML, or any other structured data formats.")
                                       .arg(GlobalFlags::lastGeneratedChartType);

    }


    if (GlobalFlags::lastGeneratedChartType == "Zodiac Signs") {
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Analyze the following planetary chart data and provide detailed insights for each of the 12 zodiac signs (Aries through Pisces). "
            "For each sign, treat it as the focal point:\n"
            "- Consider which planets are currently in that sign.\n"
            "- Consider which aspects involve the planet ruling that sign (e.g., Mars for Aries, Venus for Taurus, etc.).\n"
            "- Describe how these planetary positions and aspects influence the sign's strengths, challenges, personality traits, life path, career/work, family and finances.\n"
            "Make each sign’s narrative concise but detailed (about 12–15 sentences), like a magazine-style horoscope, with practical advice where appropriate.\n"
            "IMPORTANT: Format the output in Markdown or plain text in %2, with each zodiac sign clearly separated as its own paragraph or section. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(GlobalFlags::lastGeneratedChartType).arg(m_language);
    } else {
        // All other charts use the unified template
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Analyze the following chart data and provide a comprehensive reading covering personality traits, "
            "strengths, challenges, and life path insights. Be specific about what each planet position, house placement, "
            "and major aspect means for the individual. IMPORTANT: Your entire response must be in %2, using Markdown or plain text only. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(GlobalFlags::lastGeneratedChartType).arg(m_language);
    }
    */

    if (GlobalFlags::lastGeneratedChartType == "Zodiac Signs") {
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Analyze the following planetary chart data and provide detailed insights for each of the 12 zodiac signs (Aries through Pisces). "
            "For each sign, treat it as the focal point:\n"
            "- Consider which planets are currently in that sign.\n"
            "- Consider which aspects involve the planet ruling that sign (e.g., Mars for Aries, Venus for Taurus, etc.).\n"
            "- Describe how these planetary positions and aspects influence the sign's strengths, challenges, personality traits, life path, career/work, family and finances.\n"
            "Make each sign’s narrative concise but detailed (about 12–15 sentences), like a magazine-style horoscope, with practical advice where appropriate.\n"
            "IMPORTANT: Format the output in Markdown or plain text in %2, with each zodiac sign clearly separated as its own paragraph or section. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(GlobalFlags::lastGeneratedChartType).arg(m_language);
    }
    else if (GlobalFlags::lastGeneratedChartType == "Secondary Progression") {
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Secondary progressions represent the symbolic unfolding of the natal chart, where each day after birth corresponds to a year of life. "
            "Analyze the progressed planets, houses, and aspects in the provided data, and explain how they reflect the person’s inner development, "
            "psychological growth, and key life themes at this stage of their journey. "
            "Be specific in describing how the Sun, Moon, and personal planets shift over time and how their progressed positions interact with the natal chart. "
            "Offer insights into career, relationships, emotional life, and personal transformation. "
            "Make the narrative detailed, providing both symbolic meaning and practical advice. "
            "IMPORTANT: Format the output in Markdown or plain text in %2. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(GlobalFlags::lastGeneratedChartType).arg(m_language);
    }
    else if (GlobalFlags::lastGeneratedChartType == "Davison Relationship") {
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Davison charts are calculated by finding the exact midpoint in time and space between two individuals, creating a unique chart for the relationship itself. "
            "Analyze the planets, houses, and aspects in the provided data as if this chart represents the living ‘entity’ of the relationship. "
            "Explain the relationship’s strengths, challenges, emotional dynamics, communication style, and long-term potential. "
            "Pay attention to the Sun, Moon, Venus, and Mars placements, as well as angles and major aspects. "
            "Provide practical insights into how the partners can nurture harmony, overcome obstacles, and grow together. "
            "Make the narrative detailed, blending psychological insight with grounded relationship advice. "
            "IMPORTANT: Format the output in Markdown or plain text in %2. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(GlobalFlags::lastGeneratedChartType).arg(m_language);
    }
    else {
        // All other charts use the unified template
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Analyze the following chart data and provide a comprehensive reading covering personality traits, "
            "strengths, challenges, and life path insights. Be specific about what each planet position, house placement, "
            "and major aspect means for the individual. IMPORTANT: Your entire response must be in %2, using Markdown or plain text only. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(GlobalFlags::lastGeneratedChartType).arg(m_language);
    }

    messages.append(systemMessage);

    // User message with the chart data
    QJsonObject userMessage;
    userMessage["role"] = "user";

    // Add language instruction to user message as well for emphasis
    if (m_language != "English") {
        userMessage["content"] = QString("Please interpret this astrological chart in %1: %2")
        .arg(m_language)
            .arg(QString(QJsonDocument(chartData).toJson()));
    } else {
        userMessage["content"] = QString("Please interpret this astrological chart: %1")
        .arg(QString(QJsonDocument(chartData).toJson()));
    }

    messages.append(userMessage);

    // Create the complete request object
    QJsonObject requestObj;
    requestObj["model"] = m_model;
    requestObj["messages"] = messages;
    requestObj["temperature"] = 0.7;
    requestObj["max_tokens"] = 8192;

    return requestObj;
}

QJsonObject MistralAPI::createTransitPrompt(const QJsonObject &transitData) {
    // Create the messages array for the chat completion
    QJsonArray messages;

    // System message to instruct the model
    QJsonObject systemMessage;
    systemMessage["role"] = "system";

    // Base content with dates
    QString baseContent = QString("You are an expert astrologer providing detailed and insightful "
                                  "interpretations of planetary transits on %1 charts. The data provided contains "
                                  "transits for EACH DAY from %2 to %3 (a full %4-day period). "
                                  "Analyze the ENTIRE PERIOD, not just the first day. "
                                  "\n\nProvide a comprehensive reading covering the significant transits "
                                  "throughout this period, their exact dates of occurrence, their meanings, "
                                  "and potential effects on the individual's life. "
                                  "\n\nBegin with an overview of the major themes for this period. Then analyze "
                                  "how the transits evolve and develop over time, noting important dates when "
                                  "aspects perfect (reach 0° orb) or when multiple significant transits occur "
                                  "simultaneously. "
                                  "\n\nPay special attention to: "
                                  "\n- Outer planet transits (Jupiter through Pluto) to personal planets "
                                  "\n- Transits to angles (Ascendant, Midheaven) "
                                  "\n- Transits that perfect (reach exact aspect) during this period "
                                  "\n- Transits that repeat due to retrograde motion "
                                  "\n\nOrganize your response as a COHERENT NARRATIVE with clear sections for "
                                  "different themes or time periods. Conclude with practical "
                                  "advice for navigating these energies."
                                  "IMPORTANT: Your entire response must be in Markdown or plain text only. "
                                  "Do NOT output JSON, XML, YAML, or any other structured data formats.")
                              .arg(GlobalFlags::lastGeneratedChartType)
                              .arg(transitData["transitStartDate"].toString())
                              .arg(QDate::fromString(transitData["transitStartDate"].toString(), "yyyy/MM/dd")
                                       .addDays(transitData["numberOfDays"].toInt() - 1)
                                       .toString("yyyy/MM/dd"))
                              .arg(transitData["numberOfDays"].toInt());

    // Add language instruction if not English
    if (m_language != "English") {
        systemMessage["content"] = baseContent + QString(" IMPORTANT: Your entire response must be in %1.")
        .arg(m_language);
    } else {
        systemMessage["content"] = baseContent;
    }

    messages.append(systemMessage);

    // User message with the transit data
    QJsonObject userMessage;
    userMessage["role"] = "user";

    // Extract the raw transit data from the JSON
    QString rawTransitData = transitData["rawTransitData"].toString();

    // Create the prompt with the raw data
    QString prompt;
    if (m_language != "English") {
        prompt = QString("Please interpret these astrological transits in %1 for a person born on %2 at %3, "
                         "at latitude %4 and longitude %5. The transits cover the period from %6 for %7 days:\n\n%8")
                     .arg(m_language)
                     .arg(transitData["birthDate"].toString())
                     .arg(transitData["birthTime"].toString())
                     .arg(transitData["latitude"].toString())
                     .arg(transitData["longitude"].toString())
                     .arg(transitData["transitStartDate"].toString())
                     .arg(transitData["numberOfDays"].toString())
                     .arg(rawTransitData);
    } else {
        prompt = QString("Please interpret these astrological transits for a person born on %1 at %2, "
                         "at latitude %3 and longitude %4. The transits cover the period from %5 for %6 days:\n\n%7")
                     .arg(transitData["birthDate"].toString())
                     .arg(transitData["birthTime"].toString())
                     .arg(transitData["latitude"].toString())
                     .arg(transitData["longitude"].toString())
                     .arg(transitData["transitStartDate"].toString())
                     .arg(transitData["numberOfDays"].toString())
                     .arg(rawTransitData);
    }

    userMessage["content"] = prompt;
    messages.append(userMessage);

    // Create the complete request object
    QJsonObject requestObj;
    requestObj["model"] = m_model;
    requestObj["messages"] = messages;
    requestObj["temperature"] = 0.7;
    requestObj["max_tokens"] = 8192;

    return requestObj;
}

