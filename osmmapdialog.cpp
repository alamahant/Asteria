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
    // Set window properties
    setWindowTitle(tr("Select Location"));
    resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create status bar

    // Search controls
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *searchLabel = new QLabel(tr("Search location:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchButton = new QPushButton(tr("Search"), this);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_searchButton);
    mainLayout->addLayout(searchLayout);

    // Results list (initially hidden)
    m_resultsListWidget = new QListWidget(this);
    m_resultsListWidget->setVisible(false);
    m_resultsListWidget->setMaximumHeight(150);
    mainLayout->addWidget(m_resultsListWidget);

    // Map widget
    m_mapWidget = new QQuickWidget(this);
    m_mapWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mainLayout->addWidget(m_mapWidget, 1);

    // Coordinates display
    m_coordinatesLabel = new QLabel(tr("Click on the map to select a location"), this);
    mainLayout->addWidget(m_coordinatesLabel);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_selectButton = new QPushButton(tr("Select"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_selectButton->setEnabled(false);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_selectButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Initialize network manager
    m_networkManager = new QNetworkAccessManager(this);

    // Connections
    connect(m_selectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_searchButton, &QPushButton::clicked, this, &OSMMapDialog::onSearchClicked);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &OSMMapDialog::onSearchClicked);

    // Connect results list selection
    connect(m_resultsListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;

        // Get the status bar

        // Get the stored coordinates
        QVariant latVar = item->data(Qt::UserRole);
        QVariant lonVar = item->data(Qt::UserRole + 1);

        if (latVar.isValid() && lonVar.isValid()) {
            double lat = latVar.toDouble();
            double lon = lonVar.toDouble();

            // Center the map on the selected location
            QQuickItem* rootItem = m_mapWidget->rootObject();
            if (rootItem) {
                QVariant returnValue;
                QMetaObject::invokeMethod(rootItem, "centerMap",
                                          Q_RETURN_ARG(QVariant, returnValue),
                                          Q_ARG(QVariant, lat),
                                          Q_ARG(QVariant, lon));

                // Also set the marker and update coordinates
                onMapClicked(lat, lon);

                // Show status message

            }
        }

        // Hide the results list
        m_resultsListWidget->setVisible(false);
    });

}




void OSMMapDialog::loadMap()
{
    // Register the QML type for coordinates
    qRegisterMetaType<QGeoCoordinate>("QGeoCoordinate");

    // Set up QML context to expose C++ functions to QML
    QQmlContext *context = m_mapWidget->rootContext();
    context->setContextProperty("mapDialog", this);

    // Load the QML file
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



// Implement the search button click handler
void OSMMapDialog::onSearchClicked()
{
    QString searchText = m_searchEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }

    performSearch(searchText);
}

// Implement the search functionality
void OSMMapDialog::performSearch(const QString& searchText)
{
    // Show a status message

    // Create the URL for Nominatim search API
    QUrl url("https://nominatim.openstreetmap.org/search");
    QUrlQuery query;
    query.addQueryItem("q", searchText);
    query.addQueryItem("format", "json");
    query.addQueryItem("limit", "5"); // Get up to 5 results
    url.setQuery(query);

    // Create the network request
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "AsteriaApp/1.0");

    // Send the request
    QNetworkReply* reply = m_networkManager->get(request);

    // Handle the response
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            return;
        }

        // Parse the JSON response
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray() || doc.array().isEmpty()) {
            return;
        }

        // Show the results
        showSearchResults(doc.array());
    });
}

// Show search results in a list
void OSMMapDialog::showSearchResults(const QJsonArray& results)
{
    // Clear previous results
    m_resultsListWidget->clear();

    // Add each result to the list
    for (const QJsonValue& value : results) {
        QJsonObject result = value.toObject();

        // Get the display name and coordinates
        QString displayName = result["display_name"].toString();
        double lat = result["lat"].toString().toDouble();
        double lon = result["lon"].toString().toDouble();

        // Create a list item
        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, lat);
        item->setData(Qt::UserRole + 1, lon);

        // Add to the list
        m_resultsListWidget->addItem(item);
    }

    // Show the results list if we have results
    if (m_resultsListWidget->count() > 0) {
        m_resultsListWidget->setVisible(true);
    } else {
        m_resultsListWidget->setVisible(false);
    }
}

