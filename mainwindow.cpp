#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTableWidget>
#include <QHeaderView>
#include <QDir>
#include <QStandardPaths>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QTextStream>
#include <QDebug>
#include<QMessageBox>
#include <QPrinter>
#include <QPrintDialog>

#ifdef FLATHUB_BUILD
// QPdfWriter is not available in Flathub
#else
#include <QPdfWriter>
#endif

#include <QTextDocument>
#include<QScrollBar>

extern QString g_astroFontFamily;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chartCalculated(false)
{
    // Set window title and size
    setWindowTitle("Asteria - Astrological Chart Analysis");
    setWindowIcon(QIcon(":/icons/asteria-icon-512.png"));
    // Setup UI components
    setupUi();

    // Load settings
    loadSettings();
    // Setup Python environment for chart calculations
#ifdef FLATHUB_BUILD
    // In Flatpak, use the fixed path
    QString venvPath = "/app/python/venv";
#else
    // For non-Flatpak builds, use the path relative to current directory
    QString venvPath = QDir::currentPath() + "/python/venv";
#endif
    m_chartDataManager.setPythonVenvPath(venvPath);
    // Check if calculator is available
    if (!m_chartDataManager.isCalculatorAvailable()) {
        //QMessageBox::warning(this, "Warning",
          //                   "Python environment is not properly set up. Chart calculations may not work.\n"
            //                 "Please check the path: " + venvPath);
    }

    /*
    if (!m_mistralApi.hasValidApiKey()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("API Key Required");
        msgBox.setText("No Mistral API key found. You need to set an API key to get chart interpretations.");
        msgBox.setInformativeText("Get your free key at Mistral AI website.");
        QPushButton *openUrlButton = msgBox.addButton("Visit Mistral AI", QMessageBox::ActionRole);
        QPushButton *closeButton = msgBox.addButton(QMessageBox::Close);

        msgBox.exec();

        if (msgBox.clickedButton() == openUrlButton) {
            QDesktopServices::openUrl(QUrl("https://mistral.ai"));
        }
    }
    */

    connect(languageComboBox, &QComboBox::currentTextChanged,
            &m_mistralApi, &MistralAPI::setLanguage);

    m_symbolsDialog = nullptr;
    m_howToUseDialog = nullptr;
    chartInfoOverlay->setVisible(false);

    // Set minimum size
    this->setMinimumSize(1200, 800);
    //this->setWindowState(Qt::WindowNoState);
    // Explicitly disable full screen

    // Force the window to be the size we want
    QTimer::singleShot(0, this, [this]() {
        this->resize(1200, 800);
    });
}

MainWindow::~MainWindow()
{
    // Save settings first
    saveSettings();

    // Delete dialogs that aren't part of the widget hierarchy
    if (m_symbolsDialog) {
        delete m_symbolsDialog;
        m_symbolsDialog = nullptr;
    }

    if (m_howToUseDialog) {
        delete m_howToUseDialog;
        m_howToUseDialog = nullptr;
    }

    m_aspectarianWidget = nullptr;
    m_modalityElementWidget = nullptr;
    m_planetListWidget = nullptr;
    m_centralTabWidget = nullptr;
    chartContainer = nullptr;
    m_chartView = nullptr;
    m_chartRenderer = nullptr;
    chartInfoOverlay = nullptr;
    m_inputDock = nullptr;
    m_interpretationDock = nullptr;
    m_chartDetailsWidget = nullptr;
}

void MainWindow::setupUi()
{
    // Setup central widget with tabs
    setupCentralWidget();
    // Setup dock widgets
    setupInputDock();
    setupInterpretationDock();
    // Setup menus
    setupMenus();
    // Setup signal/slot connections
    setupConnections();
    // Set dock widget sizes
    resizeDocks({m_inputDock, m_interpretationDock}, {250, 350}, Qt::Horizontal);

    this->setStyleSheet("QScrollBar:horizontal { height: 0px; background: transparent; }");
}

void MainWindow::setupCentralWidget() {
    // Create tab widget for central area
    m_centralTabWidget = new QTabWidget(this);

    // Create chart container widget with layout
    chartContainer = new QWidget(this);
    chartLayout = new QHBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);  // Remove margins for better splitter experience

    // Create main horizontal splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, chartContainer);

    // Create chart view and renderer
    m_chartView = new QGraphicsView(mainSplitter);

    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_chartView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    m_chartView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_chartView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // Create chart renderer
    m_chartRenderer = new ChartRenderer(this);
    m_chartView->setScene(m_chartRenderer->scene());

    // Create chart info overlay widget
    /*
    chartInfoOverlay = new QWidget(m_chartView);
    //chartInfoOverlay->setGeometry(10, 10, 250, 100); // Position at top-left with some padding
    chartInfoOverlay->setGeometry(10, 10, 250, 130); // Increased from 100 to 130


    chartInfoOverlay->setStyleSheet("background-color: rgba(235, 225, 200, 0);"); // Completely transparent background

    QVBoxLayout *infoLayout = new QVBoxLayout(chartInfoOverlay);
    infoLayout->setContentsMargins(5, 5, 5, 5);
    infoLayout->setSpacing(4);
    */

    /*
    chartInfoOverlay = new QWidget(m_chartView);
    chartInfoOverlay->setGeometry(10, 10, 250, 160); // Adjusted position and height
    chartInfoOverlay->setStyleSheet("background-color: rgba(235, 225, 200, 0);"); // Completely transparent background
    QVBoxLayout *infoLayout = new QVBoxLayout(chartInfoOverlay);
    infoLayout->setContentsMargins(5, 2, 5, 2); // Minimal margins all around
    infoLayout->setSpacing(4); // Return to original spacing
    */


    // Create chart info overlay widget
    chartInfoOverlay = new QWidget(m_chartView);
    chartInfoOverlay->setGeometry(10, 10, 250, 160); // Adjusted position and height
    chartInfoOverlay->setStyleSheet("background-color: rgba(235, 225, 200, 0);"); // Completely transparent background
    QVBoxLayout *infoLayout = new QVBoxLayout(chartInfoOverlay);
    infoLayout->setContentsMargins(0, 2, 0, 2); // Set both left and right margins to 0
    infoLayout->setSpacing(4); // Keep original spacing


    // Create labels for chart information
    m_nameLabel = new QLabel("Name",chartInfoOverlay);
    m_surnameLabel = new QLabel("Surname",chartInfoOverlay);
    m_birthDateLabel = new QLabel("Birth Date",chartInfoOverlay);
    m_birthTimeLabel = new QLabel("Birth Time",chartInfoOverlay);
    m_locationLabel = new QLabel("Birth Place",chartInfoOverlay);
    m_sunSignLabel = new QLabel(chartInfoOverlay);
    m_ascendantLabel = new QLabel(chartInfoOverlay);
    m_housesystemLabel = new QLabel(chartInfoOverlay);
    // Add labels to layout
    infoLayout->addWidget(m_nameLabel);
    infoLayout->addWidget(m_surnameLabel);
    infoLayout->addWidget(m_birthDateLabel);
    infoLayout->addWidget(m_birthTimeLabel);
    infoLayout->addWidget(m_locationLabel);
    infoLayout->addWidget(m_sunSignLabel);
    infoLayout->addWidget(m_ascendantLabel);
    infoLayout->addWidget(m_housesystemLabel);

    // Add chart view to main splitter
    mainSplitter->addWidget(m_chartView);

    // Create right sidebar with vertical splitter
    QSplitter *sidebarSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    // Create PlanetListWidget
    m_planetListWidget = new PlanetListWidget(sidebarSplitter);
    m_planetListWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    sidebarSplitter->addWidget(m_planetListWidget);

    // Create AspectarianWidget
    m_aspectarianWidget = new AspectarianWidget(sidebarSplitter);
    m_aspectarianWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    sidebarSplitter->addWidget(m_aspectarianWidget);

    // Create ModalityElementWidget
    m_modalityElementWidget = new ElementModalityWidget(sidebarSplitter);
    m_modalityElementWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);


    sidebarSplitter->addWidget(m_modalityElementWidget);

    // Set initial sizes for the sidebar splitter
    QList<int> sidebarSizes;
    sidebarSizes << 200 << 300 << 150;  // PlanetList: 200px, Aspectarian: 300px, ModalityElement: 150px
    sidebarSplitter->setSizes(sidebarSizes);


    // Add sidebar splitter to main splitter
    mainSplitter->addWidget(sidebarSplitter);

    // Set initial sizes for the main splitter (75% chart, 25% sidebar)
    QList<int> mainSizes;
    mainSizes << 75 << 25; // 75 << 25
    mainSplitter->setSizes(mainSizes);


    // Add main splitter to chart layout
    chartLayout->addWidget(mainSplitter);

    // Create chart details widget (table view of chart data)
    m_chartDetailsWidget = new QWidget(this);
    QVBoxLayout *detailsLayout = new QVBoxLayout(m_chartDetailsWidget);

    // Create tables for planets, houses, and aspects
    QTabWidget *detailsTabs = new QTabWidget(m_chartDetailsWidget);

    // Planets table
    QTableWidget *planetsTable = new QTableWidget(0, 4, detailsTabs);
    planetsTable->setObjectName("Planets");
    planetsTable->setHorizontalHeaderLabels({"Planet", "Sign", "Degree", "House"});
    planetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Houses table
    QTableWidget *housesTable = new QTableWidget(0, 3, detailsTabs);
    housesTable->setObjectName("Houses");
    housesTable->setHorizontalHeaderLabels({"House", "Sign", "Degree"});
    housesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Aspects table
    QTableWidget *aspectsTable = new QTableWidget(0, 4, detailsTabs);
    aspectsTable->setObjectName("Aspects");
    aspectsTable->setHorizontalHeaderLabels({"Planet 1", "Aspect", "Planet 2", "Orb"});
    aspectsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    //////////Prediction Data
    // Create a new tab for raw prediction data
    rawTransitTable = new QTableWidget(0, 4, detailsTabs);
    rawTransitTable->setObjectName("RawTransits");
    rawTransitTable->setHorizontalHeaderLabels({"Date", "Transit Planet", "Aspect", "Natal Planet (Orb)"});
    rawTransitTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Add tables to tabs
    detailsTabs->addTab(planetsTable, "Planets");
    detailsTabs->addTab(housesTable, "Houses");
    detailsTabs->addTab(aspectsTable, "Aspects");
    detailsTabs->addTab(rawTransitTable, "Raw Transit Data");
    detailsLayout->addWidget(detailsTabs);

    // Add widgets to central tab widget
    m_centralTabWidget->addTab(chartContainer, "Chart Wheel");
    m_centralTabWidget->addTab(m_chartDetailsWidget, "Chart Details");

    setCentralWidget(m_centralTabWidget);
}

void MainWindow::setupInputDock() {
    // Create input dock widget
    QLabel* titleLabel = new QLabel("☉☽☿♀♂♃♄⛢♆♇");
    titleLabel->setFont(QFont(g_astroFontFamily, 16));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("background-color: palette(window); padding: 2px;");

    m_inputDock = new QDockWidget("Birth Chart Input",this);
    m_inputDock->setObjectName("Birth Chart Input");

    m_inputDock->setTitleBarWidget(titleLabel);
    m_inputDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    //m_inputDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    m_inputDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QWidget *inputWidget = new QWidget(m_inputDock);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputWidget);

    // Birth information group
    QGroupBox *birthGroup = new QGroupBox("Birth Details", inputWidget);
    QFormLayout *birthLayout = new QFormLayout(birthGroup);

    // Date input as QLineEdit with regex validation
    m_birthDateEdit = new QLineEdit(birthGroup);
    m_birthDateEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format");

    m_birthDateEdit->setPlaceholderText("DD/MM/YYYY");
    // Create a validator for the date format
    QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(19|20)\\d\\d$");
    QValidator *dateValidator = new QRegularExpressionValidator(dateRegex, this);
    m_birthDateEdit->setValidator(dateValidator);
    // Set current date as default
    QDate currentDate = QDate::currentDate();
    m_birthDateEdit->setText(currentDate.toString("dd/MM/yyyy"));

    // Time input as QLineEdit with regex validation
    m_birthTimeEdit = new QLineEdit(birthGroup);
    m_birthTimeEdit->setToolTip("To set new time, highlight and delete the existing time and set desired with proper format");

    m_birthTimeEdit->setPlaceholderText("HH:MM (24-hour format)");
    // Create a validator for the time format
    QRegularExpression timeRegex("^([01]\\d|2[0-3]):([0-5]\\d)$");
    QValidator *timeValidator = new QRegularExpressionValidator(timeRegex, this);
    m_birthTimeEdit->setValidator(timeValidator);
    // Set current time as default
    QTime currentTime = QTime::currentTime();
    m_birthTimeEdit->setText(currentTime.toString("HH:mm"));

    // Latitude input with regex validation
    m_latitudeEdit = new QLineEdit(birthGroup);
    m_latitudeEdit->setReadOnly(true);
    m_latitudeEdit->setPlaceholderText("e.g: 40N42 (0-90 degrees)");
    m_latitudeEdit->setToolTip("e.g: 40N42 (0-90 degrees). Please prefer the 'From Google' field");

    // Create a validator for latitude format: degrees(0-90) + N/S + minutes(0-59)
    QRegularExpression latRegex("^([0-8]\\d|90)([NSns])([0-5]\\d)$");
    QValidator *latValidator = new QRegularExpressionValidator(latRegex, this);
    m_latitudeEdit->setValidator(latValidator);

    // Longitude input with regex validation
    m_longitudeEdit = new QLineEdit(birthGroup);
    m_longitudeEdit->setReadOnly(true);
    m_longitudeEdit->setPlaceholderText("e.g: 074W00 (0-180 degrees)");
    m_longitudeEdit->setToolTip("e.g:, 074W00 (0-180 degrees). Please prefer the 'From Google' field");

    // Create a validator for longitude format: degrees(0-180) + E/W + minutes(0-59)
    QRegularExpression longRegex("^(0\\d\\d|1[0-7]\\d|180)([EWew])([0-5]\\d)$");
    QValidator *longValidator = new QRegularExpressionValidator(longRegex, this);
    m_longitudeEdit->setValidator(longValidator);

    // Google coordinates input
    m_googleCoordsEdit = new QLineEdit(birthGroup);
    m_googleCoordsEdit->setPlaceholderText("e.g: 51.5072° N, 0.1276° W");
    m_googleCoordsEdit->setToolTip("Search for a location on Google, copy the coordinates, and paste them here");

    m_googleCoordsEdit->setStyleSheet(
        "QLineEdit {"
        "  background-color: #fff9c4;"  // soft yellow
        "  border: 2px solid #f9a825;"  // amber border
        "  border-radius: 5px;"
        "  padding: 4px;"
        "  font-weight: bold;"
        "}"
        );


    // Connect the Google coordinates field to parse and fill lat/long fields when text changes
    connect(m_googleCoordsEdit, &QLineEdit::textChanged, this, [=](const QString &text) {
        // Only try to parse if the text looks like it might be complete coordinates
        if (text.contains(',') &&
            (text.contains('N') || text.contains('n') || text.contains('S') || text.contains('s')) &&
            (text.contains('E') || text.contains('e') || text.contains('W') || text.contains('w'))) {
            // Remove all spaces to simplify parsing
            QString input = text;
            input.remove(' ');
            // Split into latitude and longitude parts
            int commaPos = input.indexOf(',');
            QString latPart = input.left(commaPos);
            QString longPart = input.mid(commaPos + 1);
            // Find the position of N/S in latitude
            int latDirPos = latPart.indexOf('N');
            if (latDirPos == -1) latDirPos = latPart.indexOf('n');
            if (latDirPos == -1) latDirPos = latPart.indexOf('S');
            if (latDirPos == -1) latDirPos = latPart.indexOf('s');
            // Find the position of E/W in longitude
            int longDirPos = longPart.indexOf('E');
            if (longDirPos == -1) longDirPos = longPart.indexOf('e');
            if (longDirPos == -1) longDirPos = longPart.indexOf('W');
            if (longDirPos == -1) longDirPos = longPart.indexOf('w');

            if (latDirPos != -1 && longDirPos != -1) {
                // Extract the numeric parts and direction indicators
                QString latNumStr = latPart.left(latDirPos).remove(QString::fromUtf8("°"));
                QString latDir = latPart.mid(latDirPos, 1).toUpper();
                QString longNumStr = longPart.left(longDirPos).remove(QString::fromUtf8("°"));
                QString longDir = longPart.mid(longDirPos, 1).toUpper();
                // Convert to double
                bool latOk, longOk;
                double latDegrees = latNumStr.toDouble(&latOk);
                double longDegrees = longNumStr.toDouble(&longOk);

                if (latOk && longOk) {
                    // Convert to degrees and minutes
                    int latWholeDegrees = static_cast<int>(latDegrees);
                    int latMinutes = static_cast<int>((latDegrees - latWholeDegrees) * 60 + 0.5);
                    int longWholeDegrees = static_cast<int>(longDegrees);
                    int longMinutes = static_cast<int>((longDegrees - longWholeDegrees) * 60 + 0.5);

                    // Handle minute overflow
                    if (latMinutes == 60) {
                        latWholeDegrees++;
                        latMinutes = 0;
                    }
                    if (longMinutes == 60) {
                        longWholeDegrees++;
                        longMinutes = 0;
                    }

                    // Format for flatlib
                    QString latFormatted = QString("%1%2%3")
                                               .arg(latWholeDegrees, 2, 10, QChar('0'))
                                               .arg(latDir)
                                               .arg(latMinutes, 2, 10, QChar('0'));
                    QString longFormatted = QString("%1%2%3")
                                                .arg(longWholeDegrees, 3, 10, QChar('0'))
                                                .arg(longDir)
                                                .arg(longMinutes, 2, 10, QChar('0'));

                    // Set the formatted coordinates
                    m_latitudeEdit->setText(latFormatted);
                    m_longitudeEdit->setText(longFormatted);
                    // Show a status message
                    statusBar()->showMessage("Coordinates converted successfully", 3000);
                }
            }
        }
    });
    // Google search Location coordinates
    locationSearchEdit = new QLineEdit(this);
    locationSearchEdit->setPlaceholderText("Enter location and press Enter to search coordinates");
    locationSearchEdit->setToolTip("Enter location, for example 'Athens Greece', and press Enter to search coordinates");

    // Connect Enter key press to the search function
    connect(locationSearchEdit, &QLineEdit::returnPressed, this, [this]() {
        searchLocationCoordinates(locationSearchEdit->text());
    });



    // UTC offset combo
    m_utcOffsetCombo = new QComboBox(birthGroup);
    for (int i = -12; i <= 14; i++) {
        QString offset = (i >= 0) ? QString("+%1:00").arg(i) : QString("%1:00").arg(i);
        m_utcOffsetCombo->addItem(offset, offset);
    }
    m_utcOffsetCombo->setCurrentText("+00:00");

    // House system combo
    m_houseSystemCombo = new QComboBox(birthGroup);
    m_houseSystemCombo->addItems({"Placidus", "Koch", "Equal", "Campanus", "Regiomontanus", "Whole Sign"});

    // Add widgets to form layout
    first_name = new QLineEdit(birthGroup);
    first_name->setPlaceholderText("optional");
    last_name = new QLineEdit(birthGroup);
    last_name->setPlaceholderText("optional");
    birthLayout->addRow("First Name:", first_name);
    birthLayout->addRow("Last Name:", last_name);
    birthLayout->addRow("Birth Date:", m_birthDateEdit);
    birthLayout->addRow("Birth Time:", m_birthTimeEdit);
    birthLayout->addRow("Latitude:", m_latitudeEdit);
    birthLayout->addRow("Longitude:", m_longitudeEdit);
    birthLayout->addRow("Paste from Google:", m_googleCoordsEdit);
    birthLayout->addRow("Search Google",locationSearchEdit);



    // Location selection with OSM map
    m_selectLocationButton = new QPushButton("Select on Map", birthGroup);
    connect(m_selectLocationButton, &QPushButton::clicked, this, &MainWindow::onOpenMapClicked);
    birthLayout->addRow(m_selectLocationButton);
    //////////////////////////////////////


    birthLayout->addRow("UTC Offset:", m_utcOffsetCombo);
    birthLayout->addRow("House System:", m_houseSystemCombo);

    // Calculate button
    m_calculateButton = new QPushButton("Calculate Chart", inputWidget);
    m_calculateButton->setIcon(QIcon::fromTheme("view-refresh"));

    // Add Predictive Astrology section
    QGroupBox *predictiveGroup = new QGroupBox("Predictive Astrology", inputWidget);
    QFormLayout *predictiveLayout = new QFormLayout(predictiveGroup);

    // From date input
    m_predictiveFromEdit = new QLineEdit(predictiveGroup);
    m_predictiveFromEdit->setPlaceholderText("DD/MM/YYYY");
    m_predictiveFromEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format");

    m_predictiveFromEdit->setValidator(dateValidator); // Reuse the same validator
    // Set current date as default
    m_predictiveFromEdit->setText(currentDate.toString("dd/MM/yyyy"));

    // To date input
    m_predictiveToEdit = new QLineEdit(predictiveGroup);
    m_predictiveToEdit->setPlaceholderText("DD/MM/YYYY");
    m_predictiveToEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format");

    m_predictiveToEdit->setValidator(dateValidator); // Reuse the same validator
    // Set default to current date + 30 days
    QDate defaultFutureDate = currentDate.addDays(1); // Just a default starting point
    m_predictiveFromEdit->setText(currentDate.toString("dd/MM/yyyy"));
    m_predictiveToEdit->setText(defaultFutureDate.toString("dd/MM/yyyy"));

    // Add to form layout
    predictiveLayout->addRow("From:", m_predictiveFromEdit);
    predictiveLayout->addRow("Up to:", m_predictiveToEdit);
    //Prediction Button
    getPredictionButton = new QPushButton("Get AI Prediction", inputWidget);
    getPredictionButton->setEnabled(false);
    getPredictionButton->setIcon(QIcon::fromTheme("view-refresh"));
    getPredictionButton->setStatusTip("The AI prediction will be appended at the end of any existing text. Scroll down and be patient!");


    // Add widgets to main layout
    inputLayout->addWidget(birthGroup);
    inputLayout->addWidget(m_calculateButton);
    inputLayout->addWidget(predictiveGroup);
    inputLayout->addWidget(getPredictionButton);
    inputLayout->addStretch();

    // Set widget as dock content
    m_inputDock->setWidget(inputWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_inputDock);
}


void MainWindow::setupInterpretationDock() {
    // Create interpretation dock widget
    m_interpretationDock = new QDockWidget("Chart Interpretation", this);
    m_interpretationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_interpretationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    //m_inputDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QWidget *interpretationWidget = new QWidget(m_interpretationDock);
    QVBoxLayout *interpretationLayout = new QVBoxLayout(interpretationWidget);

    // Get interpretation button
    m_getInterpretationButton = new QPushButton("Get Birth Chart From AI", interpretationWidget);
    m_getInterpretationButton->setIcon(QIcon::fromTheme("system-search"));
    m_getInterpretationButton->setEnabled(false);

    // Interpretation text area
    m_interpretationtextEdit = new QTextEdit(interpretationWidget);
    m_interpretationtextEdit->setReadOnly(true);
    m_interpretationtextEdit->setPlaceholderText("AI interpretation will appear here after you click the 'Get Birth Chart From AI' button.");

    // Add Language Button
    QHBoxLayout* languageLayout = new QHBoxLayout();
    QLabel* languageLabel = new QLabel("Language:", interpretationWidget);
    languageComboBox = new QComboBox(interpretationWidget);
    languageComboBox->addItem("English");
    languageComboBox->addItem("Spanish");
    languageComboBox->addItem("French");
    languageComboBox->addItem("German");
    languageComboBox->addItem("Italian");
    languageComboBox->addItem("Russian");
    languageComboBox->addItem("Greek");
    languageComboBox->addItem("Portuguese");
    languageComboBox->addItem("Hindi");
    languageComboBox->addItem("Chinese (Simplified)");
    languageComboBox->setCurrentIndex(0);
    languageComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(languageComboBox);

    // Add widgets to layout
    interpretationLayout->addWidget(m_getInterpretationButton);
    interpretationLayout->addWidget(m_interpretationtextEdit);
    interpretationLayout->addLayout(languageLayout);

    // Set widget as dock content
    m_interpretationDock->setWidget(interpretationWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_interpretationDock);
}



void MainWindow::setupMenus()
{

    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    // New/Open/Save group
    QAction *newAction = fileMenu->addAction("&New Chart", this, &MainWindow::newChart);
    newAction->setShortcut(QKeySequence::New);
    newAction->setIcon(QIcon::fromTheme("document-new"));

    QAction *openAction = fileMenu->addAction("&Open Chart...", this, &MainWindow::loadChart);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon(QIcon::fromTheme("document-open"));

    QAction *saveAction = fileMenu->addAction("&Save Chart...", this, &MainWindow::saveChart);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setIcon(QIcon::fromTheme("document-save"));

    fileMenu->addSeparator();

    // Export group
    QAction *exportChartAction = fileMenu->addAction("Export Chart as &Image...", this, &MainWindow::exportChartImage);
    exportChartAction->setIcon(QIcon::fromTheme("image-x-generic"));

    QAction *exportSvgAction = fileMenu->addAction("Export as &SVG...", this, &MainWindow::exportAsSvg);
    exportSvgAction->setIcon(QIcon::fromTheme("image-svg+xml"));

    QAction *exportPdfAction = fileMenu->addAction("Export as &PDF...", this, &MainWindow::exportAsPdf);
    exportPdfAction->setIcon(QIcon::fromTheme("application-pdf"));

    QAction *exportTextAction = fileMenu->addAction("Export &Interpretation as Text...", this, &MainWindow::exportInterpretation);
    exportTextAction->setIcon(QIcon::fromTheme("text-x-generic"));

    fileMenu->addSeparator();

    // Print group
    QAction *printAction = fileMenu->addAction("&Print...", this, &MainWindow::printChart);
    printAction->setShortcut(QKeySequence::Print);
    printAction->setIcon(QIcon::fromTheme("document-print"));

    fileMenu->addSeparator();

    // Exit
    QAction *exitAction = fileMenu->addAction("E&xit", this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setIcon(QIcon::fromTheme("application-exit"));

    // View menu
    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_inputDock->toggleViewAction());
    viewMenu->addAction(m_interpretationDock->toggleViewAction());

    // Settings menu
    QMenu *settingsMenu = menuBar()->addMenu("&Settings");
    QAction *apiKeyAction = settingsMenu->addAction("Configure &API Key...", this, &MainWindow::configureApiKey);
    apiKeyAction->setIcon(QIcon::fromTheme("dialog-password"));


    QAction *checkKeyAction = settingsMenu->addAction("Check Key &Status", this, [this]() {
        if (!m_mistralApi.hasValidApiKey()) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("API Key Required");
            msgBox.setText("No Mistral API key found. You need to set an API key to get chart interpretations.");
            msgBox.setInformativeText("Get your free key at Mistral AI website.");
            QPushButton *openUrlButton = msgBox.addButton("Visit Mistral AI", QMessageBox::ActionRole);
            QPushButton *closeButton = msgBox.addButton(QMessageBox::Close);
            msgBox.exec();

#ifdef FLATHUB_BUILD
            if (msgBox.clickedButton() == openUrlButton) {
                QMessageBox infoBox;
                infoBox.setWindowTitle("Browser Opening Not Supported");
                infoBox.setText("Opening an external browser is not supported in Flathub.");
                infoBox.setInformativeText("Please manually navigate to https://mistral.ai");
                infoBox.exec();
            }
#else
            if (msgBox.clickedButton() == openUrlButton) {
                QDesktopServices::openUrl(QUrl("https://mistral.ai"));
            }
#endif
        } else {
            QMessageBox::information(this, "API Key Status", "Mistral API key is configured and ready to use.");
        }
    });
    checkKeyAction->setIcon(QIcon::fromTheme("dialog-information"));

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About...", this, &MainWindow::showAboutDialog);
    aboutAction->setIcon(QIcon::fromTheme("help-about"));

    QAction *symbolsAction = helpMenu->addAction(tr("Astrological &Symbols"));
    connect(symbolsAction, &QAction::triggered, this, &MainWindow::showSymbolsDialog);

    QAction *howToUseAction = helpMenu->addAction(tr("&How to Use"));
    connect(howToUseAction, &QAction::triggered, this, &MainWindow::showHowToUseDialog);
}

void MainWindow::setupConnections()
{
    // Chart calculation
    connect(m_calculateButton, &QPushButton::clicked, this, &MainWindow::calculateChart);

    // Interpretation
    connect(m_getInterpretationButton, &QPushButton::clicked, this, &MainWindow::getInterpretation);

    // Connect to MistralAPI signals
    connect(&m_mistralApi, &MistralAPI::interpretationReady, this, &MainWindow::displayInterpretation);
    connect(&m_mistralApi, &MistralAPI::error, this, &MainWindow::handleError);

    // Connect to ChartDataManager signals
    connect(&m_chartDataManager, &ChartDataManager::error, this, &MainWindow::handleError);

    /////////////predictive
    connect(getPredictionButton, &QPushButton::clicked, this, &MainWindow::getPrediction);
    connect(&m_mistralApi, &MistralAPI::transitInterpretationReady,
            this, &MainWindow::displayTransitInterpretation);
}

void MainWindow::calculateChart()
{
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");


    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();
    QString houseSystem = m_houseSystemCombo->currentText();

    // Validate inputs
    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    // Reset chart state before new calculation
    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_chartRenderer->scene()->clear();

    // Calculate chart
    m_currentChartData = m_chartDataManager.calculateChartAsJson(
        birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {

        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        // Clear previous interpretation
        m_currentInterpretation.clear();
        m_interpretationtextEdit->clear();
        m_interpretationtextEdit->setPlaceholderText("Click 'Get AI Interpretation' to analyze this chart.");
        statusBar()->showMessage("Chart calculated successfully", 3000);
    } else {
        handleError("Chart calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        // Clear any partial chart data after error
        m_chartRenderer->scene()->clear();
    }
}

void MainWindow::displayChart(const QJsonObject &chartData) {

    // Convert QJsonObject to ChartData
    ChartData data = convertJsonToChartData(chartData);

    // Update chart renderer with new data
    m_chartRenderer->setChartData(data);
    m_chartRenderer->renderChart();

    // Update the sidebar widgets
    m_planetListWidget->updateData(data);
    m_aspectarianWidget->updateData(data);

    m_modalityElementWidget->updateData(data);

    // Fit the view to the chart
    //m_chartView->fitInView(m_chartRenderer->scene()->sceneRect(), Qt::KeepAspectRatio);

    // Update chart details tables
    updateChartDetailsTables(chartData);
    //info overlay
    chartInfoOverlay->setVisible(true);

    populateInfoOverlay();
    // Switch to chart tab
    m_centralTabWidget->setCurrentIndex(0);
}

void MainWindow::updateChartDetailsTables(const QJsonObject &chartData)
{
    // Get table widgets
    QTableWidget *planetsTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Planets");
    QTableWidget *housesTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Houses");
    QTableWidget *aspectsTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Aspects");

    if (!planetsTable || !housesTable || !aspectsTable) {
        return;
    }

    // Clear tables
    planetsTable->setRowCount(0);
    housesTable->setRowCount(0);
    aspectsTable->setRowCount(0);

    // Fill planets table
    if (chartData.contains("planets") && chartData["planets"].isArray()) {
        QJsonArray planets = chartData["planets"].toArray();
        planetsTable->setRowCount(planets.size());

        for (int i = 0; i < planets.size(); ++i) {
            QJsonObject planet = planets[i].toObject();

            QString planetName = planet["id"].toString();
            if (planet["isRetrograde"].toBool() && planetName != "North Node" && planetName != "South Node") {
                planetName += "   ℞"; // Using the official retrograde symbol (℞)
            }

            QTableWidgetItem *nameItem = new QTableWidgetItem(planetName);
            QTableWidgetItem *signItem = new QTableWidgetItem(planet["sign"].toString());
            QTableWidgetItem *degreeItem = new QTableWidgetItem(QString::number(planet["longitude"].toDouble(), 'f', 2) + "°");
            QTableWidgetItem *houseItem = new QTableWidgetItem(planet["house"].toString());

            planetsTable->setItem(i, 0, nameItem);
            planetsTable->setItem(i, 1, signItem);
            planetsTable->setItem(i, 2, degreeItem);
            planetsTable->setItem(i, 3, houseItem);
        }
    }

    // Fill houses table
    if (chartData.contains("houses") && chartData["houses"].isArray()) {
        QJsonArray houses = chartData["houses"].toArray();
        housesTable->setRowCount(houses.size());

        for (int i = 0; i < houses.size(); ++i) {
            QJsonObject house = houses[i].toObject();

            QTableWidgetItem *nameItem = new QTableWidgetItem(house["id"].toString());
            QTableWidgetItem *signItem = new QTableWidgetItem(house["sign"].toString());
            QTableWidgetItem *degreeItem = new QTableWidgetItem(QString::number(house["longitude"].toDouble(), 'f', 2) + "°");

            housesTable->setItem(i, 0, nameItem);
            housesTable->setItem(i, 1, signItem);
            housesTable->setItem(i, 2, degreeItem);
        }
    }

    // Fill aspects table
    /*
    if (chartData.contains("aspects") && chartData["aspects"].isArray()) {
        QJsonArray aspects = chartData["aspects"].toArray();
        aspectsTable->setRowCount(aspects.size());

        for (int i = 0; i < aspects.size(); ++i) {
            QJsonObject aspect = aspects[i].toObject();

            QTableWidgetItem *planet1Item = new QTableWidgetItem(aspect["planet1"].toString());
            QTableWidgetItem *aspectTypeItem = new QTableWidgetItem(aspect["aspectType"].toString());
            QTableWidgetItem *planet2Item = new QTableWidgetItem(aspect["planet2"].toString());
            QTableWidgetItem *orbItem = new QTableWidgetItem(QString::number(aspect["orb"].toDouble(), 'f', 2) + "°");

            aspectsTable->setItem(i, 0, planet1Item);
            aspectsTable->setItem(i, 1, aspectTypeItem);
            aspectsTable->setItem(i, 2, planet2Item);
            aspectsTable->setItem(i, 3, orbItem);
        }
    }
    */

    if (chartData.contains("aspects") && chartData["aspects"].isArray()) {
        QJsonArray aspects = chartData["aspects"].toArray();
        QJsonArray planets = chartData["planets"].toArray();
        aspectsTable->setRowCount(aspects.size());

        for (int i = 0; i < aspects.size(); ++i) {
            QJsonObject aspect = aspects[i].toObject();

            // Get planet names from the aspect
            QString planet1Name = aspect["planet1"].toString();
            QString planet2Name = aspect["planet2"].toString();

            // Check if planets are retrograde by looking them up in the planets array
            bool planet1Retrograde = false;
            bool planet2Retrograde = false;

            // Find retrograde status for both planets
            for (int j = 0; j < planets.size(); ++j) {
                QJsonObject planet = planets[j].toObject();
                if (planet["id"].toString() == planet1Name) {
                    planet1Retrograde = planet["isRetrograde"].toBool();
                }
                if (planet["id"].toString() == planet2Name) {
                    planet2Retrograde = planet["isRetrograde"].toBool();
                }
            }

            // Create display text with retrograde symbol if needed
            QString planet1Display = planet1Name;
            if (planet1Retrograde && planet1Name != "North Node" && planet1Name != "South Node") {
                planet1Display += " ℞";
            }

            QString planet2Display = planet2Name;
            if (planet2Retrograde && planet2Name != "North Node" && planet2Name != "South Node") {
                planet2Display += " ℞";
            }

            // Create table items
            QTableWidgetItem *planet1Item = new QTableWidgetItem(planet1Display);
            QTableWidgetItem *aspectTypeItem = new QTableWidgetItem(aspect["aspectType"].toString());
            QTableWidgetItem *planet2Item = new QTableWidgetItem(planet2Display);
            QTableWidgetItem *orbItem = new QTableWidgetItem(QString::number(aspect["orb"].toDouble(), 'f', 2) + "°");

            aspectsTable->setItem(i, 0, planet1Item);
            aspectsTable->setItem(i, 1, aspectTypeItem);
            aspectsTable->setItem(i, 2, planet2Item);
            aspectsTable->setItem(i, 3, orbItem);
        }
    }
}

void MainWindow::getInterpretation()
{
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }

    if (!m_mistralApi.hasValidApiKey()) {
        configureApiKey();
        if (!m_mistralApi.hasValidApiKey()) {
            return;  // User canceled API key entry
        }
    }

    // Show loading message
    m_interpretationtextEdit->append("Requesting interpretation from AI...\n");
    m_getInterpretationButton->setEnabled(false);
    statusBar()->showMessage("Requesting interpretation...");

    // Request interpretation
    m_mistralApi.interpretChart(m_currentChartData);
}


void MainWindow::displayInterpretation(const QString &interpretation)
{
    m_currentInterpretation += interpretation;
    m_interpretationtextEdit->append(
        "\nChart reading for " + first_name->text() + " " + last_name->text() +
        " born on " + m_birthDateEdit->text() +
        " at " + m_birthTimeEdit->text() +
        " in location " + m_googleCoordsEdit->text() + "\n"
        );
    m_interpretationtextEdit->append("\n" + interpretation);

    m_getInterpretationButton->setEnabled(true);

    m_interpretationtextEdit->append("\nReceived interpretation from AI...");
        statusBar()->showMessage("Interpretation received", 3000);
}

void MainWindow::newChart() {
    // Clear input fields
    m_birthDateEdit->setText(QDate::currentDate().toString("dd/MM/yyyy"));
    m_birthTimeEdit->setText(QTime::currentTime().toString("HH:mm"));
    m_latitudeEdit->clear();
    m_longitudeEdit->clear();
    m_googleCoordsEdit->clear();
    m_utcOffsetCombo->setCurrentText("+00:00");
    m_houseSystemCombo->setCurrentIndex(0);

    m_nameLabel->clear();
    m_surnameLabel->clear();
    m_birthDateLabel->clear();
    m_birthTimeLabel->clear();
    m_locationLabel->clear();
    m_sunSignLabel->clear();
    m_ascendantLabel->clear();
    m_housesystemLabel->clear();

    m_predictiveFromEdit->setPlaceholderText("DD/MM/YYYY");
    // Set current date as default
    m_predictiveFromEdit->setText(QDate::currentDate().toString("dd/MM/yyyy"));
    // To date input
    m_predictiveToEdit->setPlaceholderText("DD/MM/YYYY");
    // Set default to current date + 1 days
    QDate defaultFutureDate = QDate::currentDate().addDays(1); // Just a default starting point
    m_predictiveToEdit->setText(defaultFutureDate.toString("dd/MM/yyyy"));

    // Clear chart and interpretation
    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentInterpretation.clear();

    // Clear chart renderer
    m_chartRenderer->scene()->clear();

    // Clear interpretation text
    m_interpretationtextEdit->clear();
    m_interpretationtextEdit->setPlaceholderText("Calculate a chart and click 'Get AI Interpretation'");
    m_getInterpretationButton->setEnabled(false);

    // Clear sidebar widgets with empty data
    ChartData emptyData;
    if (m_planetListWidget) {
        m_planetListWidget->updateData(emptyData);
    }
    if (m_aspectarianWidget) {
        m_aspectarianWidget->updateData(emptyData);
    }
    if (m_modalityElementWidget) {
        m_modalityElementWidget->updateData(emptyData);
    }

    // Clear chart details tables
    QTabWidget *detailsTabs = m_chartDetailsWidget->findChild<QTabWidget*>();
    if (detailsTabs) {
        QTableWidget *planetsTable = detailsTabs->findChild<QTableWidget*>("Planets");
        QTableWidget *housesTable = detailsTabs->findChild<QTableWidget*>("Houses");
        QTableWidget *aspectsTable = detailsTabs->findChild<QTableWidget*>("Aspects");

        if (planetsTable) planetsTable->setRowCount(0);
        if (housesTable) housesTable->setRowCount(0);
        if (aspectsTable) aspectsTable->setRowCount(0);
    }

    statusBar()->showMessage("New chart", 3000);
}

void MainWindow::saveChart() {
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }

    QString filePath = getFilepath("astr");
    if (filePath.isEmpty())
        return;
    QString name = first_name->text().simplified();
    QString surname = last_name->text().simplified();
    // Create JSON document with chart data and interpretation
    QJsonObject saveData;
    saveData["chartData"] = m_currentChartData;
    saveData["interpretation"] = m_currentInterpretation;

    // Add birth information for reference
    QJsonObject birthInfo;
    birthInfo["firstName"] = name;
    birthInfo["lastName"] = surname;
    birthInfo["date"] = m_birthDateEdit->text();
    QTime time = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");
    birthInfo["time"] = time.toString(Qt::ISODate);
    birthInfo["latitude"] = m_latitudeEdit->text();
    birthInfo["longitude"] = m_longitudeEdit->text();
    birthInfo["utcOffset"] = m_utcOffsetCombo->currentText();
    birthInfo["houseSystem"] = m_houseSystemCombo->currentText();
    birthInfo["googleCoords"] = m_googleCoordsEdit->text();
    saveData["birthInfo"] = birthInfo;
    // Save to file
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(saveData);
        file.write(doc.toJson());
        file.close();
        statusBar()->showMessage("Chart saved to " + filePath, 3000);
    } else {
        QMessageBox::critical(this, "Save Error", "Could not save chart to " + filePath);
    }
}

void MainWindow::loadChart() {
    // Get app name for directory
    /*
    QString appName = QApplication::applicationName();

    // Set default directory to the app directory in user's home
    QString appDir = QDir::homePath() + "/" + appName;

    // Open file dialog starting in the app directory
    QString filePath = QFileDialog::getOpenFileName(this, "Load Chart",
                                                    appDir,
                                                    "Astrological Chart (*.astr)");
    */

    QString appName = QApplication::applicationName();
    QString appDir;

#ifdef FLATHUB_BUILD
        // In Flatpak, use the app-specific data directory
    appDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + appName;
#else
        // For local builds, use a directory in home
    appDir = QDir::homePath() + "/" + appName;
#endif

    // Create directory if it doesn't exist
    QDir dir;
    if (!dir.exists(appDir))
        dir.mkpath(appDir);

    // Open file dialog starting in the app directory
    QString filePath = QFileDialog::getOpenFileName(this, "Load Chart",
                                                    appDir,
                                                    "Astrological Chart (*.astr)");



    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject saveData = doc.object();

            // Load chart data
            if (saveData.contains("chartData") && saveData["chartData"].isObject()) {
                m_currentChartData = saveData["chartData"].toObject();
                displayChart(m_currentChartData);
                m_chartCalculated = true;
                m_getInterpretationButton->setEnabled(true);
            }

            // Load interpretation
            if (saveData.contains("interpretation") && saveData["interpretation"].isString()) {
                m_currentInterpretation = saveData["interpretation"].toString();
                m_interpretationtextEdit->setPlainText(m_currentInterpretation);
            }

            // Load birth information
            if (saveData.contains("birthInfo") && saveData["birthInfo"].isObject()) {
                QJsonObject birthInfo = saveData["birthInfo"].toObject();


                // Load first and last name
                if (birthInfo.contains("firstName")) {
                    first_name->setText(birthInfo["firstName"].toString());
                }

                if (birthInfo.contains("lastName")) {
                    last_name->setText(birthInfo["lastName"].toString());
                }

                if (birthInfo.contains("date")) {
                    m_birthDateEdit->setText(birthInfo["date"].toString());
                }

                if (birthInfo.contains("time")) {
                    QTime time = QTime::fromString(birthInfo["time"].toString(), Qt::ISODate);
                    m_birthTimeEdit->setText(time.toString("HH:mm"));
                }

                if (birthInfo.contains("latitude")) {
                    m_latitudeEdit->setText(birthInfo["latitude"].toString());
                }

                if (birthInfo.contains("longitude")) {
                    m_longitudeEdit->setText(birthInfo["longitude"].toString());
                }

                if (birthInfo.contains("utcOffset")) {
                    m_utcOffsetCombo->setCurrentText(birthInfo["utcOffset"].toString());
                }

                if (birthInfo.contains("houseSystem")) {
                    m_houseSystemCombo->setCurrentText(birthInfo["houseSystem"].toString());
                }

                if (birthInfo.contains("googleCoords")) {
                    m_googleCoordsEdit->setText(birthInfo["googleCoords"].toString());
                }
            }
            populateInfoOverlay();

            statusBar()->showMessage("Chart loaded from " + filePath, 3000);

        } else {
            QMessageBox::critical(this, "Load Error", "Invalid chart file format");
        }
    } else {
        QMessageBox::critical(this, "Load Error", "Could not open chart file " + filePath);
    }
}



void MainWindow::exportInterpretation()
{
    if (m_currentInterpretation.isEmpty()) {
        QMessageBox::warning(this, "No Interpretation", "Please get an interpretation first.");
        return;
    }

    QString filePath = getFilepath("txt");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << m_currentInterpretation;
        file.close();
        statusBar()->showMessage("Interpretation exported to " + filePath, 3000);
    } else {
        QMessageBox::critical(this, "Export Error", "Could not save interpretation to " + filePath);
    }
}


void MainWindow::printChart() {
#ifdef FLATHUB_BUILD
    QMessageBox::information(
        this,
        tr("Functionality Unavailable"),
        tr("This functionality is not available in the Flathub version of Asteria.")
        );
#else
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    if (printers.isEmpty()) {
        QMessageBox::warning(this, "No Printer Found",
                             "No active printer is available.\nPlease connect a printer or select 'Export as PDF' "
                             "from File menu and print the exported PDF manually at a later time");
        return;
    }

    // Connect signal to slot, single-shot so it disconnects after one call
    disconnect(this, &MainWindow::pdfExported, this, &MainWindow::printPdfFromPath); // prevent duplicates
    connect(this, &MainWindow::pdfExported, this, [this](const QString &path) {
        QTimer::singleShot(500, this, [this, path]() {
            printPdfFromPath(path);  // delayed to ensure PDF is ready
        });
    }, Qt::SingleShotConnection);

    exportAsPdf();
#endif
}



void MainWindow::configureApiKey() {
    QString currentKey = m_mistralApi.getApiKey();
    bool ok;

    // Create a masked version of the current key for display
    QString maskedKey;
    if (!currentKey.isEmpty()) {
        // Show only first 4 and last 4 characters, mask the rest with asterisks
        if (currentKey.length() > 8) {
            maskedKey = currentKey.left(4) + QString(currentKey.length() - 8, '*') + currentKey.right(4);
        } else {
            maskedKey = QString(currentKey.length(), '*');
        }
    }

    QString apiKey = QInputDialog::getText(this, "Mistral API Key",
                                           "Enter your Mistral API key:",
                                           QLineEdit::Password, maskedKey, &ok);
    if (ok) {
        if (apiKey.isEmpty()) {
            QMessageBox::warning(this, "Warning", "No API key provided. "
                                                  "Chart interpretation will not be available.");
        } else if (apiKey == maskedKey) {
            // User didn't change the masked key, keep using the current one
            statusBar()->showMessage("API key unchanged", 3000);
        } else {
            m_mistralApi.saveApiKey(apiKey);
            statusBar()->showMessage("API key saved", 3000);
        }
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, "About AdAstra",
                       "Asteria - Astrological Chart Analysis\n\n"
                       "Version 1.0\n\n"
                       "A tool for calculating and interpreting astrological charts "
                       "with AI-powered analysis.\n\n"
                       "© 2025 Alamahant");
}

void MainWindow::handleError(const QString &errorMessage)
{
    QMessageBox::critical(this, "Error", errorMessage);
    statusBar()->showMessage("Error: " + errorMessage, 5000);
    getPredictionButton->setEnabled(true);
    m_getInterpretationButton->setEnabled(true);
}

QString MainWindow::getChartFilePath(bool forSaving)
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filePath;

    if (forSaving) {
        filePath = QFileDialog::getSaveFileName(this, "Save Chart",
                                                directory, "Chart Files (*.chart)");
    } else {
        filePath = QFileDialog::getOpenFileName(this, "Open Chart",
                                                directory, "Chart Files (*.chart)");
    }

    return filePath;
}

void MainWindow::saveSettings()
{
    QSettings settings;

    // Save window state
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());

    // Save last used house system
    settings.setValue("chart/houseSystem", m_houseSystemCombo->currentText());

    // Save UTC offset
    settings.setValue("chart/utcOffset", m_utcOffsetCombo->currentText());
}

void MainWindow::loadSettings()
{
    QSettings settings;

    // Restore window state
    if (settings.contains("mainWindow/geometry")) {
        restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    }

    if (settings.contains("mainWindow/windowState")) {
        restoreState(settings.value("mainWindow/windowState").toByteArray());
    }

    // Restore last used house system
    if (settings.contains("chart/houseSystem")) {
        QString houseSystem = settings.value("chart/houseSystem").toString();
        int index = m_houseSystemCombo->findText(houseSystem);
        if (index >= 0) {
            m_houseSystemCombo->setCurrentIndex(index);
        }
    }

    // Restore UTC offset
    if (settings.contains("chart/utcOffset")) {
        QString utcOffset = settings.value("chart/utcOffset").toString();
        int index = m_utcOffsetCombo->findText(utcOffset);
        if (index >= 0) {
            m_utcOffsetCombo->setCurrentIndex(index);
        }
    }
}

QDate MainWindow::getBirthDate() const {
    QString dateText = m_birthDateEdit->text();
    QRegularExpression dateRegex("(\\d{2})/(\\d{2})/(\\d{4})");
    QRegularExpressionMatch match = dateRegex.match(dateText);

    if (match.hasMatch()) {
        int day = match.captured(1).toInt();
        int month = match.captured(2).toInt();
        int year = match.captured(3).toInt();

        QDate date(year, month, day);
        if (date.isValid()) {
            return date;
        }
    }
    // Return current date as fallback
    return QDate::currentDate();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{

    //QMainWindow::resizeEvent(event);

    // If we have a chart, make sure it fits in the view
    //if (m_chartCalculated && m_chartView && m_chartRenderer) {
        //m_chartView->fitInView(m_chartRenderer->scene()->sceneRect(), Qt::KeepAspectRatio);
    //}
}

ChartData MainWindow::convertJsonToChartData(const QJsonObject &jsonData)
{
    ChartData chartData;

    // Convert planets
    if (jsonData.contains("planets") && jsonData["planets"].isArray()) {
        QJsonArray planetsArray = jsonData["planets"].toArray();
        for (const QJsonValue &value : planetsArray) {
            QJsonObject planetObj = value.toObject();
            PlanetData planet;
            planet.id = planetObj["id"].toString();
            planet.longitude = planetObj["longitude"].toDouble();
            planet.latitude = planetObj.contains("latitude") ? planetObj["latitude"].toDouble() : 0.0;
            // Remove the speed line since PlanetData doesn't have this member
            planet.house = planetObj.contains("house") ? planetObj["house"].toString() : "";
            planet.sign = planetObj.contains("sign") ? planetObj["sign"].toString() : "";
            if (planetObj.contains("isRetrograde")) {
                planet.isRetrograde = planetObj["isRetrograde"].toBool();
            } else {
                planet.isRetrograde = false;
            }

            chartData.planets.append(planet);
        }
    }

    // Convert houses
    if (jsonData.contains("houses") && jsonData["houses"].isArray()) {
        QJsonArray housesArray = jsonData["houses"].toArray();
        for (const QJsonValue &value : housesArray) {
            QJsonObject houseObj = value.toObject();
            HouseData house;
            house.id = houseObj["id"].toString();
            house.longitude = houseObj["longitude"].toDouble();
            house.sign = houseObj.contains("sign") ? houseObj["sign"].toString() : "";
            chartData.houses.append(house);
        }
    }

    // Convert angles (Asc, MC, etc.)
    if (jsonData.contains("angles") && jsonData["angles"].isArray()) {
        QJsonArray anglesArray = jsonData["angles"].toArray();
        for (const QJsonValue &value : anglesArray) {
            QJsonObject angleObj = value.toObject();
            AngleData angle;
            angle.id = angleObj["id"].toString();
            angle.longitude = angleObj["longitude"].toDouble();
            angle.sign = angleObj.contains("sign") ? angleObj["sign"].toString() : "";

            chartData.angles.append(angle);
        }
    }

    // Convert aspects
    if (jsonData.contains("aspects") && jsonData["aspects"].isArray()) {
        QJsonArray aspectsArray = jsonData["aspects"].toArray();
        for (const QJsonValue &value : aspectsArray) {
            QJsonObject aspectObj = value.toObject();
            AspectData aspect;
            aspect.planet1 = aspectObj["planet1"].toString();
            aspect.planet2 = aspectObj["planet2"].toString();
            aspect.aspectType = aspectObj["aspectType"].toString();
            aspect.orb = aspectObj.contains("orb") ? aspectObj["orb"].toDouble() : 0.0;
            chartData.aspects.append(aspect);
        }
    }
    return chartData;
}

//////////////////////Predictions

void MainWindow::getPrediction() {
    // Only proceed if we have a calculated chart
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a birth chart first.");
        return;
    }

    // Get birth details
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();

    // Get transit date range
    QDate fromDate = QDate::fromString(m_predictiveFromEdit->text(), "dd/MM/yyyy");
    QDate toDate = QDate::fromString(m_predictiveToEdit->text(), "dd/MM/yyyy");

    // Validate dates
    if (!fromDate.isValid() || !toDate.isValid()) {
        QMessageBox::warning(this, "Input Error", "Please enter valid dates for prediction range.");
        return;
    }

    if (fromDate > toDate) {
        QMessageBox::warning(this, "Input Error", "From date must be before To date.");
        return;
    }

    // Calculate days between (inclusive)
    int transitDays = fromDate.daysTo(toDate) + 1;

    if (transitDays <= 0 || transitDays > 30) {
        QMessageBox::warning(this, "Input Error", "Prediction period must be between 1 and 30 days.");
        return;
    }

    // Clear previous interpretation
    //m_interpretationtextEdit->clear();
    m_interpretationtextEdit->setPlaceholderText("Calculating transits...");
    getPredictionButton->setEnabled(false);

    // Update status
    statusBar()->showMessage(QString("Calculating transits for %1 to %2...")
                                 .arg(fromDate.toString("yyyy-MM-dd"))
                                 .arg(toDate.toString("yyyy-MM-dd")));

    // Calculate transits
    QJsonObject transitData = m_chartDataManager.calculateTransitsAsJson(
        birthDate, birthTime, utcOffset, latitude, longitude, fromDate, transitDays);

    if (m_chartDataManager.getLastError().isEmpty()) {
        //populate tab
        displayRawTransitData(transitData);

        // Send to API for interpretation
        m_mistralApi.interpretTransits(transitData);
    } else {
        handleError("Transit calculation error: " + m_chartDataManager.getLastError());
        getPredictionButton->setEnabled(true);

    }

}

void MainWindow::displayTransitInterpretation(const QString &interpretation) {
    m_currentInterpretation += interpretation;

    statusBar()->showMessage("Transit interpretation requested...", 3000);
    m_interpretationtextEdit->append("Transit interpretation requested...\n");

    m_interpretationtextEdit->append(
        "Astrological Prediction reading for " + first_name->text() + " " + last_name->text() +
        " born on " + m_birthDateEdit->text() +
        " at " + m_birthTimeEdit->text() +
        " in location " + m_googleCoordsEdit->text() + " for the period from " +
        m_predictiveFromEdit->text() + " to " + m_predictiveToEdit->text() + "\n\n" +
        interpretation + "\n");
    statusBar()->showMessage("Transit interpretation complete", 3000);
    m_interpretationtextEdit->append("Transit interpretation received\n");
    getPredictionButton->setEnabled(true);

}

void MainWindow::populateInfoOverlay() {
    chartInfoOverlay->setVisible(true);
    m_nameLabel->setText(first_name->text());
    m_surnameLabel->setText(last_name->text());
    m_birthDateLabel->setText(m_birthDateEdit->text());
    m_birthTimeLabel->setText(m_birthTimeEdit->text());
    m_locationLabel->setText(m_googleCoordsEdit->text());

    // Add Sun and Ascendant information from chart data
    if (!m_currentChartData.isEmpty()) {
        // Get Sun information
        if (m_currentChartData.contains("planets") && m_currentChartData["planets"].isArray()) {
            QJsonArray planets = m_currentChartData["planets"].toArray();

            for (const QJsonValue &planetValue : planets) {
                QJsonObject planet = planetValue.toObject();
                QString planetId = planet["id"].toString();

                if (planetId.toLower() == "sun") {
                    QString sunSign = planet["sign"].toString();
                    double sunDegree = planet["longitude"].toDouble();
                    m_sunSignLabel->setText(QString("Sun: %1 %2°").arg(sunSign).arg(sunDegree, 0, 'f', 1));
                    break;
                }
            }
        }

        // Get Ascendant information
        if (m_currentChartData.contains("angles") && m_currentChartData["angles"].isArray()) {
            QJsonArray angles = m_currentChartData["angles"].toArray();

            for (const QJsonValue &angleValue : angles) {
                QJsonObject angle = angleValue.toObject();
                QString angleId = angle["id"].toString();

                if (angleId.toLower() == "asc") {
                    QString ascSign = angle["sign"].toString();
                    double ascDegree = angle["longitude"].toDouble();
                    m_ascendantLabel->setText(QString("Asc: %1 %2°").arg(ascSign).arg(ascDegree, 0, 'f', 1));
                    break;
                }
            }
        }
    }
    m_housesystemLabel->setText(m_houseSystemCombo->currentText());
}

void MainWindow::displayRawTransitData(const QJsonObject &transitData) {
    // Get the raw transit data
    QString rawData = transitData["rawTransitData"].toString();

    // Clear existing rows
    rawTransitTable->setRowCount(0);

    // Parse the raw data
    QStringList lines = rawData.split('\n', Qt::SkipEmptyParts);
    bool inTransitsSection = false;
    QDate currentDate;

    for (const QString &line : lines) {
        // Check if we're in the transits section
        if (line.startsWith("---TRANSITS---")) {
            inTransitsSection = true;
            continue;
        }

        if (!inTransitsSection) {
            continue;  // Skip until we reach the transits section
        }

        // Check if this is a date line
        if (line.contains(QRegularExpression("^\\d{4}/\\d{2}/\\d{2}:"))) {
            // Extract the date
            QRegularExpression dateRe("^(\\d{4}/\\d{2}/\\d{2}):");
            QRegularExpressionMatch dateMatch = dateRe.match(line);
            if (dateMatch.hasMatch()) {
                currentDate = QDate::fromString(dateMatch.captured(1), "yyyy/MM/dd");
            }

            // Extract all aspects from this line
            int startPos = line.indexOf(':') + 1;
            QString aspectsStr = line.mid(startPos).trimmed();
            QStringList aspects = aspectsStr.split(',', Qt::SkipEmptyParts);

            for (QString aspect : aspects) {
                aspect = aspect.trimmed();
                // Parse aspect like "Sun TRI Sun( 1.10°)" or "North Node (R) TRI Jupiter( 1.40°)"
                QRegularExpression aspectRe("(\\w+(?:\\s+\\w+)?(?:\\s+\\(R\\))?) (\\w+) (\\w+(?:\\s+\\w+)?)\\( ([\\d.]+)°\\)");
                QRegularExpressionMatch aspectMatch = aspectRe.match(aspect);

                if (aspectMatch.hasMatch()) {
                    int row = rawTransitTable->rowCount();
                    rawTransitTable->insertRow(row);

                    // Date
                    rawTransitTable->setItem(row, 0, new QTableWidgetItem(currentDate.toString("yyyy-MM-dd")));

                    // Transit Planet (with retrograde marker if present)
                    rawTransitTable->setItem(row, 1, new QTableWidgetItem(aspectMatch.captured(1)));

                    // Aspect
                    rawTransitTable->setItem(row, 2, new QTableWidgetItem(aspectMatch.captured(2)));

                    // Natal Planet (Orb)
                    QString natalPlanetOrb = aspectMatch.captured(3) + " (" + aspectMatch.captured(4) + "°)";
                    rawTransitTable->setItem(row, 3, new QTableWidgetItem(natalPlanetOrb));
                }
            }
        }
    }

        rawTransitTable->sortItems(0);
}


void MainWindow::exportChartImage()
{
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }

    QString filePath = getFilepath("png");
    if (filePath.isEmpty())
        return;

    // Ensure we're on the chart tab
    m_centralTabWidget->setCurrentIndex(0);

    // Create a pixmap to render the chart
    QPixmap pixmap(m_chartView->scene()->sceneRect().size().toSize());
    pixmap.fill(Qt::white);

    // Render the chart to the pixmap
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    m_chartView->scene()->render(&painter);
    painter.end();

    // Save image
    if (pixmap.save(filePath)) {
        statusBar()->showMessage("Chart image exported to " + filePath, 3000);
    } else {
        QMessageBox::critical(this, "Export Error", "Could not save image to " + filePath);
    }
}

void MainWindow::exportAsPdf() {
#ifdef FLATHUB_BUILD
    QMessageBox::information(
        this,
        tr("Functionality Unavailable"),
        tr("This functionality is not available in the Flathub version of Asteria.")
    );
#else
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }
    
    QString filePath = getFilepath("pdf");
    if (filePath.isEmpty())
        return;
    
    // Render the chart view into a transparent pixmap
    QPixmap pixmap(m_chartView->size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    m_chartView->render(&painter);
    painter.end();
    
    // Save debug image
    const QString debugImagePath = QDir::tempPath() + "/chart_debug_render.png";
    if (!pixmap.save(debugImagePath)) {
        QMessageBox::critical(this, "Error", "Failed to save debug image. Check rendering.");
        return;
    }
    
    // PDF setup
    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(300);
    pdfWriter.setTitle("Astrological Chart");
    QPainter pdfPainter(&pdfWriter);
    if (!pdfPainter.isActive()) {
        QMessageBox::critical(this, "Error", "PDF painter failed to initialize.");
        return;
    }
    
    drawPage0(pdfPainter, pdfWriter);
    pdfWriter.newPage();  // Proceed to rest of content
    
    const QRectF pageRect = pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    const QPixmap scaledPixmap = pixmap.scaled(pageRect.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const qreal x = (pageRect.width() - scaledPixmap.width()) / 2;
    const qreal y = (pageRect.height() - scaledPixmap.height()) / 2;
    pdfPainter.drawPixmap(QPointF(x, y), scaledPixmap);
    
    // ------- PAGE 2: PLANETS -------
    pdfWriter.newPage();
    QFont titleFont("Arial", 24, QFont::Bold);
    QFont headerFont("Arial", 18, QFont::Bold);
    QFont textFont("Arial", 16);
    const int margin = 30;
    const int pageWidth = pdfWriter.width();
    const int pageHeight = pdfWriter.height();
    const int rowHeight = 120;
    const int cellPadding = 15;
    const int tableX = margin;
    const int tableWidth = pageWidth - 2 * margin;
    const int colWidth = tableWidth / 4;
    int tableY = margin + 130;
    int currentY = tableY;
    
    auto drawTableText = [&](int col, int y, const QString &text, const QFont &font) {
        QRect cell(tableX + col * colWidth + cellPadding, y + cellPadding, colWidth - 2 * cellPadding, rowHeight - 2 * cellPadding);
        pdfPainter.setFont(font);
        pdfPainter.drawText(cell, Qt::AlignCenter, text);
    };
    
    // Title
    pdfPainter.setFont(titleFont);
    pdfPainter.drawText(QRect(0, margin, pageWidth, 70), Qt::AlignCenter, "Planets");
    
    // Table headers
    pdfPainter.setPen(QPen(Qt::black, 2.0));
    pdfPainter.drawLine(tableX, tableY, tableX + tableWidth, tableY);
    drawTableText(0, currentY, "Planet", headerFont);
    drawTableText(1, currentY, "Sign", headerFont);
    drawTableText(2, currentY, "Degree", headerFont);
    drawTableText(3, currentY, "House", headerFont);
    currentY += rowHeight;
    pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
    
    if (m_currentChartData.contains("planets") && m_currentChartData["planets"].isArray()) {
        const QJsonArray planets = m_currentChartData["planets"].toArray();
        for (const auto &planetValue : planets) {
            const QJsonObject planet = planetValue.toObject();
            QString planetName = planet["id"].toString();
            if (planet.contains("isRetrograde") && planet["isRetrograde"].toBool())
                planetName += " ℞";
            drawTableText(0, currentY, planetName, textFont);
            drawTableText(1, currentY, planet["sign"].toString(), textFont);
            drawTableText(2, currentY, QString::number(planet["longitude"].toDouble(), 'f', 2) + "°", textFont);
            drawTableText(3, currentY, planet["house"].toString(), textFont);
            currentY += rowHeight;
            pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
        }
    }
    
    for (int i = 0; i <= 4; ++i)
        pdfPainter.drawLine(tableX + i * colWidth, tableY, tableX + i * colWidth, currentY);
    
    // ------- PAGE 3: HOUSE CUSPS -------
    pdfWriter.newPage();
    pdfPainter.setFont(titleFont);
    pdfPainter.drawText(QRect(0, margin, pageWidth, 70), Qt::AlignCenter, "House Cusps");
    tableY = margin + 130;
    currentY = tableY;
    pdfPainter.setPen(QPen(Qt::black, 2.0));
    pdfPainter.drawLine(tableX, tableY, tableX + tableWidth, tableY);
    drawTableText(0, currentY, "House", headerFont);
    drawTableText(1, currentY, "Sign", headerFont);
    drawTableText(2, currentY, "Degree", headerFont);
    currentY += rowHeight;
    pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
    
    QMap<int, QJsonObject> houseMap;
    if (m_currentChartData.contains("houses") && m_currentChartData["houses"].isArray()) {
        for (const auto &houseValue : m_currentChartData["houses"].toArray()) {
            const QJsonObject house = houseValue.toObject();
            int id = house["id"].toString().mid(5).toInt();  // "house5" -> 5
            houseMap[id] = house;
        }
    }
    
    for (int i = 1; i <= 12; ++i) {
        const QJsonObject house = houseMap.value(i);
        drawTableText(0, currentY, QString::number(i), textFont);
        drawTableText(1, currentY, house["sign"].toString(), textFont);
        drawTableText(2, currentY, QString::number(house["longitude"].toDouble(), 'f', 2) + "°", textFont);
        currentY += rowHeight;
        pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
    }
    
    for (int i = 0; i <= 3; ++i)
        pdfPainter.drawLine(tableX + i * colWidth, tableY, tableX + i * colWidth, currentY);
    
    // ------- PAGE 4+: ASPECTS -------
    pdfWriter.newPage();
    pdfPainter.setFont(titleFont);
    pdfPainter.drawText(QRect(0, margin, pageWidth, 70), Qt::AlignCenter, "Aspects");
    tableY = margin + 130;
    currentY = tableY;
    pdfPainter.setFont(headerFont);
    drawTableText(0, currentY, "Planet 1", headerFont);
    drawTableText(1, currentY, "Aspect", headerFont);
    drawTableText(2, currentY, "Planet 2", headerFont);
    drawTableText(3, currentY, "Orb", headerFont);
    currentY += rowHeight;
    pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
    pdfPainter.setFont(textFont);
    
    const QJsonArray aspects = m_currentChartData["aspects"].toArray();
    for (const auto &aspectValue : aspects) {
        if (currentY + rowHeight > pageHeight - margin) {
            pdfWriter.newPage();
            currentY = tableY;
            pdfPainter.setFont(headerFont);
            drawTableText(0, currentY, "Planet 1", headerFont);
            drawTableText(1, currentY, "Aspect", headerFont);
            drawTableText(2, currentY, "Planet 2", headerFont);
            drawTableText(3, currentY, "Orb", headerFont);
            currentY += rowHeight;
            pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
            pdfPainter.setFont(textFont);
        }
        
        const QJsonObject aspect = aspectValue.toObject();
        drawTableText(0, currentY, aspect["planet1"].toString(), textFont);
        drawTableText(1, currentY, aspect["aspectType"].toString(), textFont);
        drawTableText(2, currentY, aspect["planet2"].toString(), textFont);
        drawTableText(3, currentY, QString::number(aspect["orb"].toDouble(), 'f', 2) + "°", textFont);
        currentY += rowHeight;
        pdfPainter.drawLine(tableX, currentY, tableX + tableWidth, currentY);
    }
    
    for (int i = 0; i <= 4; ++i)
        pdfPainter.drawLine(tableX + i * colWidth, tableY, tableX + i * colWidth, currentY);
    
    // ------- PAGE 5+: INTERPRETATION -------
    if (!m_interpretationtextEdit->toPlainText().isEmpty()) {
        pdfWriter.newPage();
        titleFont.setPointSize(22);
        pdfPainter.setFont(titleFont);
        pdfPainter.drawText(QRect(0, margin, pageWidth, 70), Qt::AlignCenter, "Interpretation");
        
        QTextDocument doc;
        textFont.setPointSize(16);
        doc.setDefaultFont(textFont);
        doc.setDocumentMargin(20);
        doc.setTextWidth(pageWidth - 2 * margin);
        doc.setPlainText(m_interpretationtextEdit->toPlainText());
        
        QAbstractTextDocumentLayout* layout = doc.documentLayout();
        qreal totalHeight = layout->documentSize().height();
        qreal yOffset = 0;
        
        while (yOffset < totalHeight) {
            if (yOffset > 0) {
                pdfWriter.newPage();
                pdfPainter.setFont(titleFont);
                pdfPainter.drawText(QRect(0, margin, pageWidth, 70),
                                    Qt::AlignCenter, "Interpretation (cont.)");
            }
            
            qreal pageSpace = pageHeight - margin - (yOffset > 0 ? 100 : 200);
            QRectF clipRect(0, yOffset, pageWidth - 2 * margin, pageSpace);
            
            pdfPainter.save();
            pdfPainter.translate(margin, margin + (yOffset > 0 ? 100 : 200));
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.clip = clipRect;
            layout->draw(&pdfPainter, ctx);
            pdfPainter.restore();
            
            yOffset += pageSpace;
            if (yOffset <= clipRect.top()) {
                qWarning() << "Pagination stuck at offset" << yOffset;
                break;
            }
        }
    } else {
        qWarning() << "No interpretation text to render";
    }
    
    pdfPainter.end();
    emit pdfExported(filePath);
#endif
}



void MainWindow::exportAsSvg() {

    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }

    QString filePath = getFilepath("svg");
    if (filePath.isEmpty())
        return;
    // Get the chart view and scene

    QGraphicsScene* scene = m_chartView->scene();

    // Save the current transform of the view
    QTransform originalTransform = m_chartView->transform();

    // Scale down the view temporarily (80% of original size)
    m_chartView->resetTransform();
    m_chartView->scale(0.8, 0.8);

    // Force update to ensure the scene reflects the new scale
    QApplication::processEvents();

    // Get the new scene rect after scaling
    QRectF sceneRect = scene->sceneRect();

    // Create SVG generator
    QSvgGenerator generator;
    generator.setFileName(filePath);
    generator.setSize(QSize(sceneRect.width(), sceneRect.height()));
    generator.setViewBox(sceneRect);
    generator.setTitle("Astrological Chart");
    generator.setDescription("Generated by Astrology Application");
    // Create painter
    QPainter painter;
    painter.begin(&generator);
    painter.setRenderHint(QPainter::Antialiasing);
    // Fill background with white
    painter.fillRect(sceneRect, Qt::white);
    // Render the scene
    scene->render(&painter, sceneRect, sceneRect);
    painter.end();
    // Restore the original transform
    m_chartView->setTransform(originalTransform);

    statusBar()->showMessage("Chart exported to " + filePath, 3000);
}

QString MainWindow::getFilepath(const QString &format)
{
    QString name = first_name->text().simplified();
    QString surname = last_name->text().simplified();

    if (name.isEmpty() || surname.isEmpty()) {
        QMessageBox::warning(this, "Missing Information", "Please enter both first name and last name to save the chart.");
        return QString();
    }

    QString appName = QApplication::applicationName();
    QString appDir;

#ifdef FLATHUB_BUILD
    // In Flatpak, use the app-specific data directory
    appDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + appName;
#else
    // For local builds, use a directory in home
    appDir = QDir::homePath() + "/" + appName;
#endif

    QDir dir;
    if (!dir.exists(appDir))
        dir.mkpath(appDir);

    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString currentTime = QTime::currentTime().toString("HHmm");
    QString baseName = QString("%1-%2-%3-%4-chart").arg(name, surname, currentDate, currentTime);
    QString defaultFilename = QString("%1.%2").arg(baseName, format);
    QString defaultPath = appDir + "/" + defaultFilename;

    QString filter;
    if (format == "svg") {
        filter = "SVG Files (*.svg)";
    } else if (format == "pdf") {
        filter = "PDF Files (*.pdf)";
    } else if (format == "png") {
        filter = "PNG Files (*.png)";
    } else if (format == "txt") {
        filter = "Text Files (*.txt)";
    } else if (format == "astr") {
        filter = "Asteria Files (*.astr)";
    } else {
        filter = "All Files (*)";
    }

    QString filePath = QFileDialog::getSaveFileName(this, "Export Chart", defaultPath, filter);

    if (filePath.isEmpty())
        return QString();

    // Force append extension if missing
    if (!filePath.endsWith("." + format, Qt::CaseInsensitive))
        filePath += "." + format;

    return filePath;
}



void MainWindow::printPdfFromPath(const QString& filePath) {
#ifdef FLATHUB_BUILD
    // Empty implementation for Flathub
    Q_UNUSED(filePath);
#else
    if (filePath.isEmpty()){
        return;
    }
    QPdfDocument pdf;
    pdf.load(filePath);
    if (pdf.status() != QPdfDocument::Status::Ready)
    {
        qWarning() << "PDF failed to load with status:" << pdf.status();
        qWarning() << "File path was:" << filePath;
        QMessageBox::critical(this, "Error", "Failed to load the exported PDF for printing.");
    }
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, "Error", "Failed to start printing.");
        return;
    }
    for (int i = 0; i < pdf.pageCount(); ++i) {
        if (i > 0)
            printer.newPage();
        QSizeF pageSize = printer.pageRect(QPrinter::DevicePixel).size();
        QImage image = pdf.render(i, pageSize.toSize());
        painter.drawImage(QPoint(0, 0), image);
    }
    painter.end();
    statusBar()->showMessage("Chart printed successfully", 3000);
#endif
}

void MainWindow::drawPage0(QPainter &painter, QPdfWriter &writer) {
#ifdef FLATHUB_BUILD
    // Empty implementation for Flathub
    Q_UNUSED(painter);
    Q_UNUSED(writer);
#else
    painter.save();
    const QRect pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    const int pageWidth = pageRect.width();
    const int margin = 50;
    // Title
    QFont titleFont("Times", 20, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    QString title = "Asteria - Astrological Chart Generation and Analysis Tool";
    QRect titleRect(margin, margin, pageWidth - 2 * margin, 60);
    painter.drawText(titleRect, Qt::AlignCenter, title);
    // Copyright (more vertical space and padding)
    QFont copyrightFont("Times", 10);
    painter.setFont(copyrightFont);
    int copyrightTop = margin + 100;
    QRect copyrightRect(margin, copyrightTop, pageWidth - 2 * margin, 30); // taller rect
    painter.drawText(copyrightRect, Qt::AlignCenter, "© 2025 Alamahant");
    // Star Banner
    int starTop = copyrightTop + 60;
    QRect starRect(pageWidth / 2 - 50, starTop, 100, 100);
    drawStarBanner(painter, starRect);
    // Labels
    QFont labelFont("Helvetica", 12);
    painter.setFont(labelFont);
    painter.setPen(Qt::darkBlue);
    QStringList labelTexts = {
        "Name: " + first_name->text(),
        "Surname: " + last_name->text(),
        "Birth Date: " + m_birthDateEdit->text(),
        "Birth Time: " + m_birthTimeEdit->text(),
        "Location: " + m_googleCoordsEdit->text(),
        "Sun Sign: " + m_sunSignLabel->text(),
        "Ascendant: " + m_ascendantLabel->text(),
        "House System: " + m_housesystemLabel->text()
    };
    // Start Y further down, aligned horizontally with star/copyright center
    int startY = starTop + 140;
    int spacing = 60;
    int extraSpacingAfterIndex = 3; // "Birth Time"
    int extraGap = 20;
    for (int i = 0; i < labelTexts.size(); ++i) {
        int yOffset = startY + i * spacing;
        if (i > extraSpacingAfterIndex) {
            yOffset += extraGap;
        }
        int boxHeight = spacing + 40;
        QRect textRect(margin, yOffset, pageWidth - 2 * margin, boxHeight);
        painter.drawText(textRect, Qt::AlignCenter, labelTexts[i]);
    }
    painter.restore();
#endif
}




void MainWindow::drawStarBanner(QPainter &painter, const QRect &rect) {
#ifdef FLATHUB_BUILD
    // Empty implementation for Flathub
    Q_UNUSED(painter);
    Q_UNUSED(rect);
#else
    painter.save();
    QPen pen(Qt::darkBlue);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(QBrush(QColor(255, 215, 0))); // golden color
    QPoint center = rect.center();
    int radius = qMin(rect.width(), rect.height()) / 2 - 5;
    QPolygon star;
    for (int i = 0; i < 10; ++i) {
        double angle = i * M_PI / 5.0;
        int r = (i % 2 == 0) ? radius : radius / 2;
        int x = center.x() + r * std::sin(angle);
        int y = center.y() - r * std::cos(angle);
        star << QPoint(x, y);
    }
    painter.drawPolygon(star);
    painter.restore();
#endif
}


/*
void MainWindow::searchLocationCoordinates(const QString& location) {
    if (location.isEmpty()) {
        return;
    }

    // Create the search URL
    QString searchQuery = QString("coordinates of %1").arg(location);
    QString encodedQuery = QUrl::toPercentEncoding(searchQuery);
    QUrl url(QString("https://www.google.com/search?q=%1").arg(QString(encodedQuery)));

    // Open the URL in the default browser
    QDesktopServices::openUrl(url);
    locationSearchEdit->clear();

}
*/


void MainWindow::searchLocationCoordinates(const QString& location) {
    if (location.isEmpty()) {
        return;
    }

#ifdef FLATHUB_BUILD
    QMessageBox::information(this, tr("Feature Unavailable"),
                             tr("This feature is not available in the Flathub version of Asteria.\n"
                                "Please manually search for the location coordinates."));
#else
    // Create the search URL
    QString searchQuery = QString("coordinates of %1").arg(location);
    QString encodedQuery = QUrl::toPercentEncoding(searchQuery);
    QUrl url(QString("https://www.google.com/search?q=%1").arg(QString(encodedQuery)));

    // Open the URL in the default browser
    QDesktopServices::openUrl(url);
    locationSearchEdit->clear();
#endif
}




void MainWindow::showSymbolsDialog()
{
    // Create the dialog only if it doesn't exist yet
    if (!m_symbolsDialog) {
        m_symbolsDialog = new SymbolsDialog(this, g_astroFontFamily);

        // Connect the dialog's finished signal to handle cleanup
        connect(m_symbolsDialog, &QDialog::finished, this, [this]() {
            // This ensures the dialog is properly deleted when closed
            m_symbolsDialog->deleteLater();
            m_symbolsDialog = nullptr;
        });
    }

    // Show and raise the dialog to bring it to the front
    m_symbolsDialog->show();
    m_symbolsDialog->raise();
    m_symbolsDialog->activateWindow();
}

/*
void MainWindow::showHowToUseDialog()
{
    // Create the dialog only if it doesn't exist yet
    if (!m_howToUseDialog) {
        m_howToUseDialog = new QDialog(this);
        m_howToUseDialog->setWindowTitle("How to Use AstroChart");
        m_howToUseDialog->setMinimumSize(500, 400);

        // Create layout
        QVBoxLayout *layout = new QVBoxLayout(m_howToUseDialog);

        // Create a text browser for rich text display
        QTextBrowser *textBrowser = new QTextBrowser(m_howToUseDialog);
        textBrowser->setOpenExternalLinks(true);

        // Set the help content
        QString helpText = R"(
        <h2>How to Use Asteria</h2>

        <h3>Getting Started</h3>
        <p>Asteria allows you to create and analyze astrological birth charts. Follow these steps to get started:</p>

        <ol>
            <li><b>Enter Birth Information:</b> Fill in the name, date, time, and location of birth in the input fields.</li>
            <li><b>Generate Chart:</b> Click the "Calculate Chart" button to create the astrological chart.</li>
            <li><b>View Chart:</b> The chart will appear in the main display area.</li>
            <li><b>Analyze Aspects:</b> The aspect grid shows relationships between planets.</li>
            <li><b>Get AI Interpretation:</b> Click "Get Birth Chart From AI" or "Get AI Prediction" to receive an interpretation of the chart or a prediction.</li>
        </ol>

        <h3>Chart Features</h3>
        <ul>
            <li><b>Planets:</b> The chart displays the positions of celestial bodies at the time of birth.</li>
            <li><b>Houses:</b> The twelve houses represent different areas of life.</li>
            <li><b>Aspects:</b> Lines connecting planets show their relationships (conjunctions, oppositions, etc.).</li>
            <li><b>Zodiac Signs:</b> The twelve signs of the zodiac form the outer wheel of the chart.</li>
        </ul>

        <h3>Tips</h3>
        <ul>
            <li>For accurate charts, ensure the birth time and location are as precise as possible.</li>
            <li>Use the "Astrological Symbols" reference to understand the symbols in the chart.</li>
            <li>Different house systems can be selected from the dropdown menu.</li>
            <li>You can save and print charts using the File menu.</li>
        </ul>

        <p>For more information about astrology and chart interpretation, consult astrological resources or books.</p>
        )";

        textBrowser->setHtml(helpText);
        layout->addWidget(textBrowser);

        // Add a close button at the bottom
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_howToUseDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // Connect the close button
        connect(closeButton, &QPushButton::clicked, m_howToUseDialog, &QDialog::close);

        // Connect the dialog's finished signal to handle cleanup
        connect(m_howToUseDialog, &QDialog::finished, this, [this]() {
            m_howToUseDialog->deleteLater();
            m_howToUseDialog = nullptr;
        });
    }

    // Show and raise the dialog to bring it to the front
    m_howToUseDialog->show();
    m_howToUseDialog->raise();
    m_howToUseDialog->activateWindow();
}
*/

void MainWindow::showHowToUseDialog() {
    // Create the dialog only if it doesn't exist yet
    if (!m_howToUseDialog) {
        m_howToUseDialog = new QDialog(this);
        m_howToUseDialog->setWindowTitle("How to Use Asteria");
        m_howToUseDialog->setMinimumSize(500, 400);

        // Create layout
        QVBoxLayout *layout = new QVBoxLayout(m_howToUseDialog);

        // Create a text browser for rich text display
        QTextBrowser *textBrowser = new QTextBrowser(m_howToUseDialog);
        textBrowser->setOpenExternalLinks(true);

        // Set the help content
        QString helpText = R"(
        <h2>How to Use Asteria</h2>
        <h3>Getting Started</h3>
        <p>Asteria allows you to create and analyze astrological birth charts. Follow these steps to get started:</p>
        <ol>
            <li><b>Enter Birth Information:</b> Fill in the name, date, time, and location of birth in the input fields.</li>
            <li><b>Generate Chart:</b> Click the "Calculate Chart" button to create the astrological chart.</li>
            <li><b>View Chart:</b> The chart will appear in the main display area.</li>
            <li><b>Analyze Aspects:</b> The aspect grid shows relationships between planets.</li>
            <li><b>Get AI Interpretation:</b> Click "Get Birth Chart From AI" or "Get AI Prediction" to receive an interpretation of the chart or a prediction.</li>
        </ol>
        <h3>Chart Features</h3>
        <ul>
            <li><b>Planets:</b> The chart displays the positions of celestial bodies at the time of birth.</li>
            <li><b>Houses:</b> The twelve houses represent different areas of life.</li>
            <li><b>Aspects:</b> Lines connecting planets show their relationships (conjunctions, oppositions, etc.).</li>
            <li><b>Zodiac Signs:</b> The twelve signs of the zodiac form the outer wheel of the chart.</li>
        </ul>
        <h3>Tips & Features</h3>
        <ul>
            <li>Hover your mouse over planets, signs, and houses on the chart to see detailed tooltips containing valuable information.</li>
            <li>You can also hover at the edges of the different panels of the app (such as the Planets-Aspectarian-Elements panel, the AI interpretation panel etc. Just hover you mouse and find out). When you see the mouse cursor change to a resize cursor, you can click and drag to resize these panels for better viewing and a more comfortable layout.</li>
            <li>Asteria can generate AI-powered future predictions for a period of up to 30 days. Play with it! You can set the starting date anytime in the future. Asteria can give insight into how future transits may affect your chart.</li>
            <li>You can select the language used by the AI from the dropdown menu at the bottom right corner of the UI. This feature is still experimental—feel free to explore, but English is recommended for best results.</li>
            <li>For the most immersive and clear view of the chart, it is best to use Asteria in full screen mode.</li>
        </ul>
        <p>For accurate charts, ensure the birth time and location are as precise as possible. Use the "Astrological Symbols" reference to understand the chart's symbols. You can also save or print charts from the File menu.</p>
        <p>For more information about astrology and chart interpretation, consult astrological resources or books.</p>
        )";

        textBrowser->setHtml(helpText);
        layout->addWidget(textBrowser);

        // Add a close button at the bottom
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_howToUseDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // Connect the close button
        connect(closeButton, &QPushButton::clicked, m_howToUseDialog, &QDialog::close);

        // Connect the dialog's finished signal to handle cleanup
        connect(m_howToUseDialog, &QDialog::finished, this, [this]() {
            m_howToUseDialog->deleteLater();
            m_howToUseDialog = nullptr;
        });
    }

    // Show and raise the dialog to bring it to the front
    m_howToUseDialog->show();
    m_howToUseDialog->raise();
    m_howToUseDialog->activateWindow();
}


void MainWindow::onOpenMapClicked()
{
    OSMMapDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QGeoCoordinate coords = dialog.selectedCoordinates();

        // Determine direction
        QString latDir = (coords.latitude() >= 0) ? "N" : "S";
        QString longDir = (coords.longitude() >= 0) ? "E" : "W";

        // Update only the Google coordinates field
        m_googleCoordsEdit->setText(QString("%1° %2, %3° %4")
                                        .arg(qAbs(coords.latitude()), 0, 'f', 4)
                                        .arg(latDir)
                                        .arg(qAbs(coords.longitude()), 0, 'f', 4)
                                        .arg(longDir));

        // The lat/long edits will be automatically updated by your existing onTextChanged handler
    }
}

