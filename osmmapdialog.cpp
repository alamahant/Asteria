#include "osmmapdialog.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QHBoxLayout>
#include <QMessageBox>
#include<QMetaObject>
#include <QQuickItem>

OSMMapDialog::OSMMapDialog(QWidget *parent)
    : QDialog(parent)
    , m_mapWidget(nullptr)
    , m_selectButton(nullptr)
    , m_cancelButton(nullptr)
    , m_searchButton(nullptr)
    , m_searchEdit(nullptr)
    , m_coordinatesLabel(nullptr)
    , m_locationSelected(false)

{
    setWindowTitle(tr("Select Location"));
    setMinimumSize(800, 600);
    setModal(true);

    setupUi();
    loadMap();
}

OSMMapDialog::~OSMMapDialog()
{
}

QGeoCoordinate OSMMapDialog::selectedCoordinates() const
{
    return m_selectedCoordinates;
}


void OSMMapDialog::setupUi()
{
    setWindowTitle(tr("Select Location"));
    resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);


    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *searchLabel = new QLabel(tr("Search location:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchButton = new QPushButton(tr("Search"), this);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_searchButton);
    mainLayout->addLayout(searchLayout);

    m_resultsListWidget = new QListWidget(this);
    m_resultsListWidget->setVisible(false);
    m_resultsListWidget->setMaximumHeight(150);
    mainLayout->addWidget(m_resultsListWidget);

    m_mapWidget = new QQuickWidget(this);
    m_mapWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mainLayout->addWidget(m_mapWidget, 1);

    m_coordinatesLabel = new QLabel(tr("Click on the map to select a location"), this);
    mainLayout->addWidget(m_coordinatesLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_selectButton = new QPushButton(tr("Select"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_selectButton->setEnabled(false);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_selectButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    m_networkManager = new QNetworkAccessManager(this);

    connect(m_selectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_searchButton, &QPushButton::clicked, this, &OSMMapDialog::onSearchClicked);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &OSMMapDialog::onSearchClicked);

    connect(m_resultsListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;


        QVariant latVar = item->data(Qt::UserRole);
        QVariant lonVar = item->data(Qt::UserRole + 1);

        if (latVar.isValid() && lonVar.isValid()) {
            double lat = latVar.toDouble();
            double lon = lonVar.toDouble();

            QQuickItem* rootItem = m_mapWidget->rootObject();
            if (rootItem) {
                QVariant returnValue;
                QMetaObject::invokeMethod(rootItem, "centerMap",
                                          Q_RETURN_ARG(QVariant, returnValue),
                                          Q_ARG(QVariant, lat),
                                          Q_ARG(QVariant, lon));

                onMapClicked(lat, lon);


            }
        }

        m_resultsListWidget->setVisible(false);
    });

}




void OSMMapDialog::loadMap()
{
    qRegisterMetaType<QGeoCoordinate>("QGeoCoordinate");

    QQmlContext *context = m_mapWidget->rootContext();
    context->setContextProperty("mapDialog", this);

    m_mapWidget->setSource(QUrl("qrc:/map.qml"));
}

void OSMMapDialog::onMapClicked(double latitude, double longitude)
{
    m_selectedCoordinates = QGeoCoordinate(latitude, longitude);
    m_locationSelected = true;
    m_selectButton->setEnabled(true);

    m_coordinatesLabel->setText(tr("Selected: %1, %2")
                                    .arg(latitude, 0, 'f', 6)
                                    .arg(longitude, 0, 'f', 6));
}



void OSMMapDialog::onSearchClicked()
{
    QString searchText = m_searchEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }

    performSearch(searchText);
}

void OSMMapDialog::performSearch(const QString& searchText)
{

    QUrl url("https://nominatim.openstreetmap.org/search");
    QUrlQuery query;
    query.addQueryItem("q", searchText);
    query.addQueryItem("format", "json");
    query.addQueryItem("limit", "5"); // Get up to 5 results
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "AsteriaApp/1.0");

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray() || doc.array().isEmpty()) {
            return;
        }

        showSearchResults(doc.array());
    });
}

void OSMMapDialog::showSearchResults(const QJsonArray& results)
{
    m_resultsListWidget->clear();

    for (const QJsonValue& value : results) {
        QJsonObject result = value.toObject();

        QString displayName = result["display_name"].toString();
        double lat = result["lat"].toString().toDouble();
        double lon = result["lon"].toString().toDouble();

        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, lat);
        item->setData(Qt::UserRole + 1, lon);

        m_resultsListWidget->addItem(item);
    }

    if (m_resultsListWidget->count() > 0) {
        m_resultsListWidget->setVisible(true);
    } else {
        m_resultsListWidget->setVisible(false);
    }
}

