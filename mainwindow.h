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
//#include<QtPrintSupport/QtPrintSupport>
#include <QGraphicsView>
#include <QTemporaryFile>
#include <QImageReader>
#include <QTimer>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QTextBrowser>
#include <QDialog>

#if defined(FLATHUB_BUILD) || defined(GENTOO_BUILD)
// QtPdf is not available in Flathub
#else
#include <QtPdf/QPdfDocument>
#include<QtPrintSupport/QtPrintSupport>
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
#include<QItemSelection>
#include "transitsearchdialog.h"
#include<QJsonArray>
#include<QJsonDocument>
#include<QJsonObject>
#include<QProgressDialog>
#include<QPoint>
#include"donationdialog.h"

struct ParsedDate {
    int year;   // Astronomical year (negative for BCE, 0 for 1 BCE, etc.)
    int month;
    int day;
    bool valid;
};


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
    //ParsedDate getBirthDate() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
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
#ifndef FLATHUB_BUILD
    void drawPage0(QPainter &painter, QPdfWriter &writer);
#endif
    //void drawPage0(QPainter &painter, QPdfWriter &writer);

    QString getFilepath(const QString& format);
    QComboBox* languageComboBox;
    void searchLocationCoordinates(const QString& location);
    QLineEdit* locationSearchEdit;
    SymbolsDialog *m_symbolsDialog;
    void showSymbolsDialog();
    QDialog *m_howToUseDialog = nullptr;
    void showHowToUseDialog();
    QDialog *m_relationshipChartsDialog = nullptr;
    QDialog* m_showChangelogDialog = nullptr;
signals:
    void pdfExported(const QString& filePath);

private slots:
    void onOpenMapClicked();

private:
    // Existing members...
    QPushButton *m_selectLocationButton;
    QString getOrbDescription(double orb);
    void preloadMapResources();

//testing additional bodies checkbox
private:
    QCheckBox *m_additionalBodiesCB;
    QJsonObject loadChartForRelationships(const QString &filePath);
    //QPushButton* m_clearAllButton;
    QJsonObject m_currentRelationshipInfo;


private slots:
    void showAspectSettings();
    // Relationship chart slots
    void createCompositeChart();
    void createDavisonChart();
    void createSynastryChart();
    void showRelationshipChartsDialog();
    void showChangelog();
    // transit charts
    //void showTransitChart();
    void CalculateTransits();
    void CalculateEclipses();

private:
    QPushButton* getTransitsButton;
    QDialog* m_transitDialog;

    // Search Dialog
private slots:
    void applyTransitFilter(const QString &datePattern,
                            const QString &transitPattern,
                            const QString &aspectPattern,
                            const QString &natalPattern,
                            const QString &maxOrbPattern,

                             const QString &excludePattern);
    void openTransitFilter();
    void calculateSolarReturn();
    void calculateLunarReturn();
    void calculateSaturnReturn();
    void calculateJupiterReturn();
    // Venus return
    void calculateVenusReturn();
    // Mars Return
    void calculateMarsReturn();
    // Mercury Return
    void calculateMercuryReturn();
    void calculateUranusReturn();
    void calculateNeptuneReturn();
    void calculatePlutoReturn();
private:
    void doLunarReturnCalculation(const QDate& birthDate, const QTime& birthTime, const QDate& targetDate);
    void doSolarReturnCalculation(const QDate& birthDate, const QTime& birthTime, int year);
    void doSaturnReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doJupiterReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doVenusReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doMarsReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doMercuryReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doUranusReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doNeptuneReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    void doPlutoReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber);
    // Secondary Progression Chart
    void calculateSecondaryProgression();
    void doSecondaryProgressionCalculation(int progressionYear);

    int m_savedScrollPosition;
    QItemSelection m_savedSelection;
    TransitSearchDialog *m_transitSearchDialog = nullptr;

private slots:
    void exportChartData();
    void showNewFeaturesDialog();

private:
    bool solarEclipses = true;
    bool lunarEclipses = true;
    void displayRawEclipseData(const QJsonArray &eclipseData);

private:
    QDate julianToGregorian(int year, int month, int day) const;


    QDate checkAndConvertJulian(const QDate& date, bool useJulian) const;


    bool validateDateFormat(const QString& dateText, QWidget* parentWidget);
    QAction *useJulianForPre1582Action = nullptr;
    static const QRegularExpression dateRegex;
    QDialog* m_showNewFeaturesDialog = nullptr;

private slots:
    void toggleChartOnlyView(bool chartOnly);

private:
    QAction *m_chartOnlyAction = nullptr;
    bool m_showInfoOverlay;
    QAction *showOverlayAction = nullptr;
    QPoint m_dragStartPosition;
    void startChartDrag();
    void importChartInputData(const QJsonObject &inputData);
    QString markdownToHtml(const QString &markdown);
    QString plainTextToHtml(const QString &plainText);
private slots:
    void calculateZodiacSignsChart();
    void copySavePath();
private:

};
#endif // MAINWINDOW_H
