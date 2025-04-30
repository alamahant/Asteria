#ifndef OSMMAPDIALOG_H
#define OSMMAPDIALOG_H

#include <QDialog>
#include <QGeoCoordinate>
#include <QQuickWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include<QListWidget>
#include<QNetworkAccessManager>
#include<QJsonObject>
#include<QJsonDocument>
#include<QJsonArray>
#include<QNetworkReply>
#include<QUrlQuery>
#include<QUrl>

class OSMMapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OSMMapDialog(QWidget *parent = nullptr);
    ~OSMMapDialog();

    QGeoCoordinate selectedCoordinates() const;

public slots:
    void onMapClicked(double latitude, double longitude);
    void onSearchClicked();

private:
    void setupUi();
    void loadMap();

    QQuickWidget *m_mapWidget;
    QPushButton *m_selectButton;
    QPushButton *m_cancelButton;
    QPushButton *m_searchButton;
    QLineEdit *m_searchEdit;
    QLabel *m_coordinatesLabel;
    QGeoCoordinate m_selectedCoordinates;
    bool m_locationSelected;

private:
    QNetworkAccessManager* m_networkManager;
    void performSearch(const QString& searchText);
    void showSearchResults(const QJsonArray& results);
    QListWidget* m_resultsListWidget;
};

#endif // OSMAPDIALOG_H

