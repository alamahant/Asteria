#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QDate>
#include <QTime>
#include <QJsonObject>
#include <QSettings>
#include <QTabWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include<QtSvg/QSvgGenerator>
#include<QtPrintSupport/QtPrintSupport>
#include <QGraphicsView>
#include <QTemporaryFile>
#include <QImageReader>


#ifdef FLATHUB_BUILD
// QtPdf is not available in Flathub
#else
#include <QtPdf/QPdfDocument>
#endif

#include <QDesktopServices>
#include <QUrl>
#include<QDialog>
#include "chartdatamanager.h"
#include "mistralapi.h"
#include"chartcalculator.h"
#include"chartrenderer.h"
#include "planetlistwidget.h"
#include "aspectarianwidget.h"
#include "elementmodalitywidget.h"
#include "symbolsdialog.h"
#include"osmmapdialog.h"
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    // Chart calculation and display
    void calculateChart();
    void displayChart(const QJsonObject &chartData);

    // AI interpretation
    void getInterpretation();
    void displayInterpretation(const QString &interpretation);

    // Menu actions
    void newChart();
    void saveChart();
    void loadChart();
    void exportChartImage();
    void exportInterpretation();
    void printChart();
    void showAboutDialog();

    // Settings
    void configureApiKey();

    // Error handling
    void handleError(const QString &errorMessage);

private:
    // UI setup methods
    void setupUi();
    void setupMenus();
    void setupInputDock();
    void setupInterpretationDock();
    void setupCentralWidget();
    void setupConnections();

    // Helper methods
    void saveSettings();
    void loadSettings();
    QString getChartFilePath(bool forSaving = false);

    // UI components
    QDockWidget *m_inputDock;
    QDockWidget *m_interpretationDock;
    QTabWidget *m_centralTabWidget;

    // Chart widget and data
    //ChartWidget *m_chartWidget;
    QGraphicsView *m_chartView;
    ChartRenderer *m_chartRenderer;
    QWidget *m_chartDetailsWidget;
    // Input widgets
    QLineEdit* first_name;
    QLineEdit* last_name;
    QLineEdit *m_birthDateEdit;
    QLineEdit *m_birthTimeEdit;
    QLineEdit *m_latitudeEdit;
    QLineEdit *m_longitudeEdit;
    QLineEdit* m_googleCoordsEdit;
    QComboBox *m_utcOffsetCombo;
    QComboBox *m_houseSystemCombo;
    QPushButton *m_calculateButton;
    QHBoxLayout *chartLayout;
    QWidget *chartContainer;
    // Interpretation widgets
    QTextEdit *m_interpretationtextEdit;
    QPushButton *m_getInterpretationButton;
    // Data managers
    ChartDataManager m_chartDataManager;
    MistralAPI m_mistralApi;
    // Current chart data
    QJsonObject m_currentChartData;
    QString m_currentInterpretation;
    bool m_chartCalculated;
    QDate getBirthDate() const;
protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void updateChartDetailsTables(const QJsonObject &chartData);
private:
    // Helper method to convert QJsonObject to ChartData
    ChartData convertJsonToChartData(const QJsonObject &jsonData);
    // Existing members...
    PlanetListWidget *m_planetListWidget;
    AspectarianWidget *m_aspectarianWidget;
    ElementModalityWidget *m_modalityElementWidget;
    QLineEdit* m_predictiveFromEdit;
    QLineEdit* m_predictiveToEdit;
    QPushButton *getPredictionButton;
    QTableWidget *rawTransitTable;
    void displayRawTransitData(const QJsonObject &transitData);
private slots:
    void getPrediction();
    void displayTransitInterpretation(const QString &interpretation);
    void populateInfoOverlay();
    void exportAsSvg();
    void exportAsPdf();
    void printPdfFromPath(const QString& filePath);
private:
    QWidget *chartInfoOverlay;
    QLabel *m_nameLabel;
    QLabel* m_surnameLabel;
    QLabel* m_birthDateLabel;
    QLabel* m_birthTimeLabel;
    QLabel *m_locationLabel;
    QLabel *m_sunSignLabel;
    QLabel *m_ascendantLabel;
    QLabel *m_housesystemLabel;
    void drawStarBanner(QPainter &painter, const QRect &rect);
    void drawPage0(QPainter &painter, QPdfWriter &writer);
    QString getFilepath(const QString& format);
    QComboBox* languageComboBox;
    void searchLocationCoordinates(const QString& location);
    QLineEdit* locationSearchEdit;
    SymbolsDialog *m_symbolsDialog;
    void showSymbolsDialog();
    QDialog *m_howToUseDialog;
    void showHowToUseDialog();

signals:
    void pdfExported(const QString& filePath);

private slots:
    void onOpenMapClicked();

private:
    // Existing members...
    QPushButton *m_selectLocationButton;

};
#endif // MAINWINDOW_H
