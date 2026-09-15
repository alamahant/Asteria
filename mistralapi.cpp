#include "mistralapi.h"
#include <QDir>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDebug>
#include"Globals.h"

MistralAPI::MistralAPI(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_requestInProgress(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &MistralAPI::handleNetworkReply);

    AsteriaFlags::activeModelLoaded = loadActiveModel();
}

MistralAPI::~MistralAPI()
{
}

void MistralAPI::interpretChart(const QJsonObject &chartData)
{
    if (m_requestInProgress) {
        m_lastError = "A request is already in progress";
        emit error(m_lastError);
        return;
    }

    if (!AsteriaFlags::activeModelLoaded) {
        m_lastError = "No active AI model configured. Please configure one in Settings → Configure AI Models.";;
        emit error(m_lastError);
        return;
    }


    QJsonObject prompt = createPrompt(chartData);

    QUrl url(m_apiEndpoint);
    QNetworkRequest request{url};  // Using braces instead of parentheses
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QJsonDocument doc(prompt);
    QByteArray data = doc.toJson();

    m_networkManager->post(request, data);
    m_requestInProgress = true;

}


void MistralAPI::handleNetworkReply(QNetworkReply *reply) {
    m_requestInProgress = false;

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = "Network error: " + reply->errorString();
        emit error(m_lastError);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        m_lastError = "Invalid JSON response";
        emit error(m_lastError);
        reply->deleteLater();
        return;
    }

    QJsonObject responseObj = doc.object();

    QString formattedResponse = formatInterpretation(responseObj);
    if (formattedResponse.isEmpty()) {
        m_lastError = "Failed to extract response from API";
        emit error(m_lastError);
    } else {
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

    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return appDataPath + "/settings.ini";
}

QString MistralAPI::getLastError() const
{
    return m_lastError;
}


void MistralAPI::interpretTransits(const QJsonObject &transitData) {
    if (m_requestInProgress) {
        m_lastError = "A request is already in progress";
        emit error(m_lastError);
        return;
    }

    if (!AsteriaFlags::activeModelLoaded) {
        m_lastError = "No active AI model configured. Please configure one in Settings → Configure AI Models.";;
        emit error(m_lastError);
        return;
    }

    QJsonObject prompt = createTransitPrompt(transitData);

    QUrl url(m_apiEndpoint);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QJsonDocument doc(prompt);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, data);
    reply->setProperty("isTransitRequest", true);
    m_requestInProgress = true;

}


QJsonObject MistralAPI::createPrompt(const QJsonObject &chartData) {
    QJsonArray messages;

    QJsonObject systemMessage;
    systemMessage["role"] = "system";


    if (AsteriaFlags::lastGeneratedChartType == "Zodiac Signs") {
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
            ).arg(AsteriaFlags::lastGeneratedChartType).arg(m_language);
    }
    else if (AsteriaFlags::lastGeneratedChartType == "Secondary Progression") {
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
            ).arg(AsteriaFlags::lastGeneratedChartType).arg(m_language);
    }
    else if (AsteriaFlags::lastGeneratedChartType == "Davison Relationship") {
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
            ).arg(AsteriaFlags::lastGeneratedChartType).arg(m_language);

    }else if (AsteriaFlags::lastGeneratedChartType == "Synastry") {
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of Synastry charts. "
            "Analyze the relationship between Person A and Person B based on their planetary aspects and house overlays. "
            "Explain the strengths, challenges, emotional dynamics, communication style, and long-term potential of this relationship. "
            "Pay special attention to personal planets (Sun, Moon, Venus, Mars), outer planets (Jupiter, Saturn, Uranus, Neptune, Pluto), "
            "and the house overlays where each person's planets fall in the other's houses. "
            "Provide practical insights into how the partners can nurture harmony, overcome obstacles, and grow together. "
            "Make the narrative detailed, blending psychological insight with grounded relationship advice. "
            "IMPORTANT: Format the output in Markdown or plain text in %1. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(m_language);
    } else {
        systemMessage["content"] = QString(
            "You are an expert astrologer providing detailed and insightful interpretations of %1 charts. "
            "Analyze the following chart data and provide a comprehensive reading covering personality traits, "
            "strengths, challenges, and life path insights. Be specific about what each planet position, house placement, "
            "and major aspect means for the individual. IMPORTANT: Your entire response must be in %2, using Markdown or plain text only. "
            "Do NOT output JSON, XML, YAML, or any other structured data formats."
            ).arg(AsteriaFlags::lastGeneratedChartType).arg(m_language);
    }

    messages.append(systemMessage);

    QJsonObject userMessage;
    userMessage["role"] = "user";

    if (m_language != "English") {
        userMessage["content"] = QString("Please interpret this astrological chart in %1: %2")
        .arg(m_language)
            .arg(QString(QJsonDocument(chartData).toJson()));
    } else {
        userMessage["content"] = QString("Please interpret this astrological chart: %1")
        .arg(QString(QJsonDocument(chartData).toJson()));
    }

    messages.append(userMessage);

    QJsonObject requestObj;
    requestObj["model"] = m_model;
    requestObj["messages"] = messages;
    requestObj["temperature"] = m_temperature;  // Use member variable
    requestObj["max_tokens"] = m_maxTokens;     // Use member variable

    return requestObj;
}

QJsonObject MistralAPI::createTransitPrompt(const QJsonObject &transitData) {
    QJsonArray messages;

    QJsonObject systemMessage;
    systemMessage["role"] = "system";

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
                              .arg(AsteriaFlags::lastGeneratedChartType)
                              .arg(transitData["transitStartDate"].toString())
                              .arg(QDate::fromString(transitData["transitStartDate"].toString(), "yyyy/MM/dd")
                                       .addDays(transitData["numberOfDays"].toInt() - 1)
                                       .toString("yyyy/MM/dd"))
                              .arg(transitData["numberOfDays"].toInt());

    if (m_language != "English") {
        systemMessage["content"] = baseContent + QString(" IMPORTANT: Your entire response must be in %1.")
        .arg(m_language);
    } else {
        systemMessage["content"] = baseContent;
    }

    messages.append(systemMessage);

    QJsonObject userMessage;
    userMessage["role"] = "user";

    QString rawTransitData = transitData["rawTransitData"].toString();

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

    QJsonObject requestObj;
    requestObj["model"] = m_model;
    requestObj["messages"] = messages;
    requestObj["temperature"] = m_temperature;  // Use member variable
    requestObj["max_tokens"] = m_maxTokens;     // Use member variable

    return requestObj;
}

bool MistralAPI::loadActiveModel()
{
    QSettings settings;
    settings.beginGroup("Models");

    QString activeModelName = settings.value("ActiveModel").toString();
    if (activeModelName.isEmpty()) {
        m_lastError = "No active model selected";
        return false;
    }

    settings.beginGroup(activeModelName);
    m_apiEndpoint = settings.value("endpoint").toString();
    m_apiKey = settings.value("apiKey").toString();
    m_model = settings.value("modelName").toString();
    m_temperature = settings.value("temperature", 0.7).toDouble();
    m_maxTokens = settings.value("maxTokens", 8192).toInt();


    
    settings.endGroup();
    settings.endGroup();

    return !m_apiEndpoint.isEmpty() && !m_model.isEmpty();
}
