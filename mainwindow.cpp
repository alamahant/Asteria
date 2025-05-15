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
//#include <QPrinter>
//#include <QPrintDialog>

#ifdef FLATHUB_BUILD
// QPdfWriter is not available in Flathub
#else
#include <QPdfWriter>
#include <QPrinter>
#include <QPrintDialog>
#endif

#include <QTextDocument>
#include<QScrollBar>
#include"Globals.h"
#include"aspectsettingsdialog.h"
#include <QCheckBox>  // Add this include if still needed


extern QString g_astroFontFamily;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chartCalculated(false)
{
    preloadMapResources();
    // Set window title and size
    setWindowTitle("Asteria - Astrological Chart Analysis");
    setWindowIcon(QIcon(":/icons/asteria-icon-512.png"));
    // Setup UI components
    setupUi();

    // Load settings
    loadSettings();
    if (!m_chartDataManager.isCalculatorAvailable()) {
    }

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
    if (m_relationshipChartsDialog) {
        delete m_relationshipChartsDialog;
        m_relationshipChartsDialog = nullptr;
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

    m_inputDock = new QDockWidget(this);
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
    ///////////////// Create a validator for the date format
    /*
#ifdef FLATHUB_BUILD
    // For Flatpak builds with complete ephemeris files, allow dates from 1000-2399
    QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(1[0-9]|2[0-3])\\d\\d$");
#else
    // For standard builds, restrict to 1900-2099 for compatibility
    QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(19|20)\\d\\d$");
#endif
    */

#ifdef FLATHUB_BUILD
    // For Flatpak builds with complete ephemeris files
    // Allow dates from 3000 BCE to 3000 CE
    // Format: DD/MM/±YYYY where ± is optional for positive years and required for negative (BCE)
    QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(-3000|-[1-2]\\d{3}|-[1-9]\\d{0,2}|[0-2]\\d{3}|3000)$");
#else
    // For standard builds, restrict to 1900-2099 for compatibility
    //QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(19|20)\\d\\d$");
    QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(-3000|-[1-2]\\d{3}|-[1-9]\\d{0,2}|[0-2]\\d{3}|3000)$");

#endif


    //QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(19|20)\\d\\d$");
    QValidator *dateValidator = new QRegularExpressionValidator(dateRegex, this);
    m_birthDateEdit->setValidator(dateValidator);
    //////////////////////////
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
    //m_latitudeEdit->setPlaceholderText("e.g: 40N42 (0-90 degrees)");
    m_latitudeEdit->setToolTip("Please prefer the 'From Google' field or the 'Select on Map' button.");

    // Create a validator for latitude format: degrees(0-90) + N/S + minutes(0-59)
    //QRegularExpression latRegex("^([0-8]\\d|90)([NSns])([0-5]\\d)$");
    //QValidator *latValidator = new QRegularExpressionValidator(latRegex, this);
    //m_latitudeEdit->setValidator(latValidator);

    // Longitude input with regex validation
    m_longitudeEdit = new QLineEdit(birthGroup);
    m_longitudeEdit->setReadOnly(true);
    //m_longitudeEdit->setPlaceholderText("e.g: 074W00 (0-180 degrees)");
    //m_longitudeEdit->setToolTip("e.g:, 074W00 (0-180 degrees). Please prefer the 'From Google' field");
    m_longitudeEdit->setToolTip("Please prefer the 'From Google' field or the 'Select on Map' button.");

    // Create a validator for longitude format: degrees(0-180) + E/W + minutes(0-59)
    //QRegularExpression longRegex("^(0\\d\\d|1[0-7]\\d|180)([EWew])([0-5]\\d)$");
    //QValidator *longValidator = new QRegularExpressionValidator(longRegex, this);
    //m_longitudeEdit->setValidator(longValidator);

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
                    // For Swiss Ephemeris, we need decimal degrees with sign
                    // Negative for South latitude and West longitude
                    double latDecimal = latDegrees;
                    if (latDir == "S") latDecimal = -latDecimal;

                    double longDecimal = longDegrees;
                    if (longDir == "W") longDecimal = -longDecimal;

                    // Set the decimal coordinates directly
                    m_latitudeEdit->setText(QString::number(latDecimal, 'f', 6));
                    m_longitudeEdit->setText(QString::number(longDecimal, 'f', 6));

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
    /*
    m_utcOffsetCombo = new QComboBox(birthGroup);
    for (int i = -12; i <= 14; i++) {
        QString offset = (i >= 0) ? QString("+%1:00").arg(i) : QString("%1:00").arg(i);
        m_utcOffsetCombo->addItem(offset, offset);
    }
    m_utcOffsetCombo->setCurrentText("+00:00");
    */
    m_utcOffsetCombo = new QComboBox(birthGroup);

    // Create a list to hold all offsets
    QList<QPair<QString, double>> offsetsWithValues;

    // Add whole hour offsets
    for (int i = -12; i <= 14; i++) {
        QString offset = (i >= 0) ? QString("+%1:00").arg(i) : QString("%1:00").arg(i);
        double value = i;
        offsetsWithValues.append(qMakePair(offset, value));
    }

    // Add common half-hour and 45-minute offsets
    QMap<QString, double> specialOffsets = {
        {"-9:30", -9.5},   // Marquesas Islands
        {"-3:30", -3.5},   // Newfoundland, Canada
        {"+3:30", 3.5},    // Iran
        {"+4:30", 4.5},    // Afghanistan
        {"+5:30", 5.5},    // India, Sri Lanka
        {"+5:45", 5.75},   // Nepal
        {"+6:30", 6.5},    // Myanmar, Cocos Islands
        {"+8:45", 8.75},   // Western Australia (Eucla)
        {"+9:30", 9.5},    // South Australia, Northern Territory (Australia)
        {"+10:30", 10.5},  // Lord Howe Island (Australia)
        {"+12:45", 12.75}  // Chatham Islands (New Zealand)
    };

    // Add special offsets to the list
    for (auto it = specialOffsets.begin(); it != specialOffsets.end(); ++it) {
        offsetsWithValues.append(qMakePair(it.key(), it.value()));
    }

    // Sort by numeric value
    std::sort(offsetsWithValues.begin(), offsetsWithValues.end(),
              [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
                  return a.second < b.second;
              });

    // Add sorted items to combo box
    for (const auto& pair : offsetsWithValues) {
        m_utcOffsetCombo->addItem(pair.first);
    }

    m_utcOffsetCombo->setCurrentText("+00:00");
    m_utcOffsetCombo->setToolTip("Select the UTC offset for the birth location.\n"
                                 "Remember to account for Daylight Saving Time if applicable.\n"
                                 "For accurate charts, you need to determine if DST was in effect\n"
                                 "at the time and location of birth.");



    // House system combo
    m_houseSystemCombo = new QComboBox(birthGroup);
    m_houseSystemCombo->addItems({"Placidus", "Koch", "Porphyrius", "Regiomontanus", "Campanus", "Equal", "Whole Sign"});

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
   // m_selectLocationButton = new QPushButton("Select on Map", birthGroup);
    //m_selectLocationButton->setIcon(QIcon::fromTheme("view-refresh"));

    //connect(m_selectLocationButton, &QPushButton::clicked, this, &MainWindow::onOpenMapClicked);
    //birthLayout->addRow(m_selectLocationButton);

    m_selectLocationButton = new QPushButton("Select on Map", birthGroup);
    m_selectLocationButton->setIcon(QIcon::fromTheme("view-refresh"));
    connect(m_selectLocationButton, &QPushButton::clicked, this, &MainWindow::onOpenMapClicked);

    // Set fixed size policy to match QLineEdit width
    m_selectLocationButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Force the button to take the full available width
    m_selectLocationButton->setMinimumWidth(200);  // Set a reasonable minimum width

    // Add to form layout
    birthLayout->addRow("Location:", m_selectLocationButton);

    // After all widgets are added to the layout and the form is shown,
    // you might need to call this in the showEvent or after setup:
    m_selectLocationButton->setMinimumWidth(locationSearchEdit->width());


    birthLayout->addRow("UTC Offset:", m_utcOffsetCombo);
    birthLayout->addRow("House System:", m_houseSystemCombo);
    //orbmax slider
    QWidget *orbContainer = new QWidget(inputWidget);
    QVBoxLayout *orbLayout = new QVBoxLayout(orbContainer);
    orbLayout->setContentsMargins(0, 0, 0, 0);

    // Create a horizontal layout for the slider and value label
    QHBoxLayout *sliderLayout = new QHBoxLayout();

    // Create the slider
    QSlider *orbSlider = new QSlider(Qt::Horizontal, orbContainer);
    orbSlider->setRange(0, 24);  // 0 to 12 in 0.5° increments (multiply by 2)
    orbSlider->setValue(static_cast<int>(getOrbMax() * 2)); // Convert current value to slider range
    orbSlider->setTickInterval(4); // Tick marks every 2° (multiply by 2)
    orbSlider->setTickPosition(QSlider::TicksBelow);
    orbSlider->setMinimumWidth(150);

    // Create value label
    QLabel *orbValueLabel = new QLabel(QString::number(getOrbMax(), 'f', 1) + "°", orbContainer);
    orbValueLabel->setMinimumWidth(40);
    orbValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Add slider and value label to the horizontal layout
    sliderLayout->addWidget(orbSlider);
    sliderLayout->addWidget(orbValueLabel);

    // Create a label for the description
    QLabel *orbDescriptionLabel = new QLabel(getOrbDescription(getOrbMax()), orbContainer);
    orbDescriptionLabel->setAlignment(Qt::AlignCenter);

    // Add both layouts to the container
    orbLayout->addLayout(sliderLayout);
    orbLayout->addWidget(orbDescriptionLabel);

    // Connect slider value changes
    connect(orbSlider, &QSlider::valueChanged, [=](int value) {
        double orb = value / 2.0;
        orbValueLabel->setText(QString::number(orb, 'f', 1) + "°");
        orbDescriptionLabel->setText(getOrbDescription(orb));
        setOrbMax(orb); // Update the global setting
    });

    // Add the container to the form layout
    birthLayout->addRow("Aspect Orbs:", orbContainer);

    //add additionalbodies checkbox

    m_additionalBodiesCB = new QCheckBox("Include Additional Bodies", this);
    m_additionalBodiesCB->setToolTip("Include Lilith, Ceres, Pallas, Juno, Vesta, Vertex, East Point and Part of Spirit");
    connect(m_additionalBodiesCB, &QCheckBox::toggled, this, [this]() {
        if (m_chartCalculated) {
            displayChart(m_currentChartData);
        }
    });
    birthLayout->addRow(m_additionalBodiesCB);
    //clear button
    //m_clearAllButton = new QPushButton("Refresh", this);
    //m_clearAllButton->setIcon(QIcon::fromTheme("view-refresh"));
    //connect(m_clearAllButton, &QPushButton::clicked, this, &MainWindow::newChart);
    //birthLayout->addRow(m_clearAllButton);

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
    getPredictionButton = new QPushButton("Get AI Prediction", predictiveGroup);
    getPredictionButton->setEnabled(false);
    getPredictionButton->setIcon(QIcon::fromTheme("view-refresh"));
    getPredictionButton->setStatusTip("The AI prediction will be appended at the end of any existing text. Scroll down and be patient!");
    predictiveLayout->addRow(getPredictionButton);



    // Add widgets to main layout
    inputLayout->addWidget(birthGroup);
    inputLayout->addWidget(m_calculateButton);
    //inputLayout->addWidget(m_clearAllButton);

    inputLayout->addWidget(predictiveGroup);
    //inputLayout->addWidget(getPredictionButton);
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

    // Create an action for aspect settings
    QAction *aspectSettingsAction = new QAction("&Aspect Display Settings...", this);
    // Connect the action to a slot that will open the dialog
    connect(aspectSettingsAction, &QAction::triggered, this, &MainWindow::showAspectSettings);
    // Add the action to the settings menu
    settingsMenu->addAction(aspectSettingsAction);
    //

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

    // Create Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("Tools");

    // Create Relationship Charts submenu
    QMenu *relationshipMenu = toolsMenu->addMenu("Relationship Charts");

    // Create actions for relationship chart types
    QAction *compositeAction = new QAction("Composite Chart", this);
    QAction *davisonAction = new QAction("Davison Relationship Chart", this);
    QAction *synastryAction = new QAction("Synastry Chart", this);

    // Add actions to the relationship menu
    relationshipMenu->addAction(compositeAction);
    relationshipMenu->addAction(davisonAction);
    relationshipMenu->addAction(synastryAction);

    // Disable synastry for future implementation
    synastryAction->setEnabled(false);

    // Connect actions to slots
    connect(compositeAction, &QAction::triggered, this, &MainWindow::createCompositeChart);
    connect(davisonAction, &QAction::triggered, this, &MainWindow::createDavisonChart);
    connect(synastryAction, &QAction::triggered, this, &MainWindow::createSynastryChart);


    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About...", this, &MainWindow::showAboutDialog);
    aboutAction->setIcon(QIcon::fromTheme("help-about"));

    QAction *symbolsAction = helpMenu->addAction(tr("Astrological &Symbols"));
    connect(symbolsAction, &QAction::triggered, this, &MainWindow::showSymbolsDialog);

    QAction *howToUseAction = helpMenu->addAction(tr("&How to Use"));
    connect(howToUseAction, &QAction::triggered, this, &MainWindow::showHowToUseDialog);

    QAction *relationshipChartsAction = helpMenu->addAction(tr("About &Relationship Charts"));
    connect(relationshipChartsAction, &QAction::triggered, this, &MainWindow::showRelationshipChartsDialog);
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
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info

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

    // Define which bodies are considered "additional"
    QStringList additionalBodies = {
        "Ceres", "Pallas", "Juno", "Vesta", "Lilith",
        "Vertex", "Part of Spirit", "East Point"
    };

    // Create a filtered copy of the chart data
    QJsonObject filteredChartData = chartData;

    // Filter out additional bodies if checkbox is not checked
    if (!m_additionalBodiesCB->isChecked()) {
        // Filter planets
        QJsonArray planets = filteredChartData["planets"].toArray();
        QJsonArray filteredPlanets;

        for (int i = 0; i < planets.size(); i++) {
            QJsonObject planet = planets[i].toObject();
            QString planetId = planet["id"].toString();

            // Keep the planet if it's not in the additional bodies list
            if (!additionalBodies.contains(planetId)) {
                filteredPlanets.append(planet);
            }
        }
        filteredChartData["planets"] = filteredPlanets;

        // Filter aspects
        QJsonArray aspects = filteredChartData["aspects"].toArray();
        QJsonArray filteredAspects;

        for (int i = 0; i < aspects.size(); i++) {
            QJsonObject aspect = aspects[i].toObject();
            QString planet1 = aspect["planet1"].toString();
            QString planet2 = aspect["planet2"].toString();

            // Keep the aspect if neither planet is an additional body
            if (!additionalBodies.contains(planet1) && !additionalBodies.contains(planet2)) {
                filteredAspects.append(aspect);
            }
        }
        filteredChartData["aspects"] = filteredAspects;
    }

    // Convert QJsonObject to ChartData
    ChartData data = convertJsonToChartData(filteredChartData);

    // Update chart renderer with new data
    m_chartRenderer->setChartData(data);
    m_chartRenderer->renderChart();

    // Update the sidebar widgets
    m_planetListWidget->updateData(data);
    m_aspectarianWidget->updateData(data);
    m_modalityElementWidget->updateData(data);

    // Update chart details tables
    updateChartDetailsTables(filteredChartData);

    //info overlay
    chartInfoOverlay->setVisible(true);
    populateInfoOverlay();

    // Switch to chart tab
    m_centralTabWidget->setCurrentIndex(0);
   // this->setWindowTitle("Asteria - Astrological Chart Analysis - Birth Chart");
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


void MainWindow::getInterpretation() {
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

    // Create filtered chart data based on additional bodies checkbox
    QJsonObject dataToSend = m_currentChartData;

    // Define which bodies are considered "additional"
    QStringList additionalBodies = {
        "Ceres", "Pallas", "Juno", "Vesta", "Lilith",
        "Vertex", "Part of Spirit", "East Point"
    };

    // Filter out additional bodies if checkbox is not checked
    if (!m_additionalBodiesCB->isChecked()) {
        // Filter planets
        QJsonArray planets = dataToSend["planets"].toArray();
        QJsonArray filteredPlanets;
        for (int i = 0; i < planets.size(); i++) {
            QJsonObject planet = planets[i].toObject();
            QString planetId = planet["id"].toString();
            // Keep the planet if it's not in the additional bodies list
            if (!additionalBodies.contains(planetId)) {
                filteredPlanets.append(planet);
            }
        }
        dataToSend["planets"] = filteredPlanets;

        // Filter aspects - remove any aspect that involves an additional body
        QJsonArray aspects = dataToSend["aspects"].toArray();
        QJsonArray filteredAspects;
        for (int i = 0; i < aspects.size(); i++) {
            QJsonObject aspect = aspects[i].toObject();
            QString planet1 = aspect["planet1"].toString();
            QString planet2 = aspect["planet2"].toString();

            // Keep the aspect if neither planet is an additional body
            if (!additionalBodies.contains(planet1) && !additionalBodies.contains(planet2)) {
                filteredAspects.append(aspect);
            }
        }
        dataToSend["aspects"] = filteredAspects;
    }

    // Show loading message
    m_interpretationtextEdit->append("Requesting interpretation from AI...\n");
    m_getInterpretationButton->setEnabled(false);
    statusBar()->showMessage("Requesting interpretation...");

    // Request interpretation with filtered data
    m_mistralApi.interpretChart(dataToSend);
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
    first_name->clear();  // Clear first name field
    last_name->clear();   // Clear last name field
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
    m_currentRelationshipInfo = QJsonObject();  // Reset relationship info

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
    this->setWindowTitle("Asteria - Astrological Chart Analysis");
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

    // Check if this is a relationship chart (Composite or Davison)
    // and add relationship info if it exists
    if (m_currentRelationshipInfo.isEmpty() == false) {
        saveData["relationshipInfo"] = m_currentRelationshipInfo;
    }

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

    // Clear all previous chart data before loading a new one
    newChart();

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

            // Load relationship information if it exists
            if (saveData.contains("relationshipInfo") && saveData["relationshipInfo"].isObject()) {
                m_currentRelationshipInfo = saveData["relationshipInfo"].toObject();

                // Set window title based on relationship info
                if (m_currentRelationshipInfo.contains("displayName")) {
                    setWindowTitle("Asteria - Astrological Chart Analysis - " +
                                   m_currentRelationshipInfo["displayName"].toString());
                }
            } else {
                // Clear any existing relationship info
                m_currentRelationshipInfo = QJsonObject();

                // Set default window title for natal chart
                QString name = first_name->text();
                QString surname = last_name->text();
                if (!name.isEmpty() || !surname.isEmpty()) {
                    setWindowTitle("Asteria - Astrological Chart Analysis - " + name + " " + surname);
                } else {
                    setWindowTitle("Asteria - Astrological Chart Analysis - Birth Chart");
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
    // add aditional bodies or not
    //settings.setValue("chart/includeAdditionalBodies", m_additionalBodiesCB->isChecked());
    // Save aspect display settings
    AspectSettings::instance().saveToSettings(settings);

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
    // Restore additional bodies setting
    //m_additionalBodiesCB->setChecked(settings.value("chart/includeAdditionalBodies", false).toBool());

    AspectSettings::instance().loadFromSettings(settings);

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

    // Debug the raw data

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
                } else {
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
    //appDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + appName;

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

#ifndef FLATHUB_BUILD
void MainWindow::drawPage0(QPainter &painter, QPdfWriter &writer) {
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
}
#endif


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

QString MainWindow::getOrbDescription(double orb) {
    if (orb <= 7.0)
        return "Conservative/Tight";
    else if (orb <= 9.0)
        return "Moderate";
    else if (orb <= 10.5)
        return "Standard";
    else
        return "Liberal/Wide";
}



void MainWindow::preloadMapResources() {
    // Create a hidden instance of the map dialog to preload QML
    OSMMapDialog *preloadDialog = new OSMMapDialog(this);
    preloadDialog->hide();  // Make sure it's hidden

    // Schedule deletion after a short delay to ensure QML is fully loaded
    QTimer::singleShot(1000, [preloadDialog]() {
        preloadDialog->deleteLater();
    });
}

void MainWindow::showAspectSettings()
{
    AspectSettingsDialog dialog(this);

    // If the user accepts the dialog (clicks Save)
    if (dialog.exec() == QDialog::Accepted) {
        if (m_chartCalculated) {
            displayChart(m_currentChartData);
        }
    }
}


/////////////////////////////////////Relationship charts


void MainWindow::createCompositeChart() {
    // Show info message
    QMessageBox::information(this, "Select Charts",
                             "Please select two natal charts to create a composite chart.");
    // Get app directory for file dialog
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

    // Open file dialog for selecting two charts
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this, "Select Two Charts", appDir, "Astrological Chart (*.astr)");

    // Validate selection
    if (filePaths.size() != 2) {
        QMessageBox::warning(this, "Invalid Selection",
                             "You must select exactly two charts.");
        return;
    }

    // Load the charts
    QJsonObject saveData1;
    QJsonObject saveData2;
    // Load first chart
    QFile file1(filePaths[0]);
    if (file1.open(QIODevice::ReadOnly)) {
        QByteArray data = file1.readAll();
        file1.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            saveData1 = doc.object();
        } else {
            QMessageBox::critical(this, "Load Error", "Invalid chart file format: " + filePaths[0]);
            return;
        }
    } else {
        QMessageBox::critical(this, "Load Error", "Could not open chart file " + filePaths[0]);
        return;
    }

    // Load second chart
    QFile file2(filePaths[1]);
    if (file2.open(QIODevice::ReadOnly)) {
        QByteArray data = file2.readAll();
        file2.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            saveData2 = doc.object();
        } else {
            QMessageBox::critical(this, "Load Error", "Invalid chart file format: " + filePaths[1]);
            return;
        }
    } else {
        QMessageBox::critical(this, "Load Error", "Could not open chart file " + filePaths[1]);
        return;
    }

    // Extract chart data
    QJsonObject chartData1 = saveData1["chartData"].toObject();
    QJsonObject chartData2 = saveData2["chartData"].toObject();

    // Extract birth info
    QJsonObject birthInfo1 = saveData1["birthInfo"].toObject();
    QJsonObject birthInfo2 = saveData2["birthInfo"].toObject();

    // Get names for display
    QString name1 = birthInfo1["firstName"].toString();
    QString name2 = birthInfo2["firstName"].toString();
    QString surname1 = birthInfo1["lastName"].toString();
    QString surname2 = birthInfo2["lastName"].toString();

    // Create composite chart data
    QJsonObject compositeChartData;

    // Calculate midpoint planets
    QJsonArray planets1 = chartData1["planets"].toArray();
    QJsonArray planets2 = chartData2["planets"].toArray();
    QJsonArray compositePlanets;

    // Create a map for quick lookup of planets in chart2
    QMap<QString, QJsonObject> planetMap2;
    for (const QJsonValue &planetValue : planets2) {
        QJsonObject planet = planetValue.toObject();
        planetMap2[planet["id"].toString()] = planet;
    }

    // Calculate midpoints for planets
    for (const QJsonValue &planetValue1 : planets1) {
        QJsonObject planet1 = planetValue1.toObject();
        QString planetId = planet1["id"].toString();
        // Find matching planet in chart2
        if (planetMap2.contains(planetId)) {
            QJsonObject planet2 = planetMap2[planetId];
            // Create composite planet
            QJsonObject compositePlanet;
            compositePlanet["id"] = planetId;
            // Calculate midpoint longitude
            double long1 = planet1["longitude"].toDouble();
            double long2 = planet2["longitude"].toDouble();
            // Handle the case where angles cross 0°/360° boundary
            double diff = fmod(long2 - long1 + 540.0, 360.0) - 180.0;
            double midpoint = fmod(long1 + diff/2.0 + 360.0, 360.0);
            compositePlanet["longitude"] = midpoint;
            // Determine the sign for the midpoint
            int signIndex = static_cast<int>(midpoint) / 30;
            QStringList signs = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
                                 "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
            compositePlanet["sign"] = signs[signIndex];
            // Copy other properties from first planet
            if (planet1.contains("retrograde"))
                compositePlanet["retrograde"] = planet1["retrograde"];
            if (planet1.contains("house"))
                compositePlanet["house"] = planet1["house"];
            compositePlanets.append(compositePlanet);
        }
    }

    // Calculate midpoints for angles
    QJsonArray angles1 = chartData1["angles"].toArray();
    QJsonArray angles2 = chartData2["angles"].toArray();
    QJsonArray compositeAngles;

    // Create maps for quick lookup
    QMap<QString, QJsonObject> angleMap1;
    QMap<QString, QJsonObject> angleMap2;
    for (const QJsonValue &angleValue : angles1) {
        QJsonObject angle = angleValue.toObject();
        if (angle.contains("id")) {
            angleMap1[angle["id"].toString()] = angle;
        }
    }
    for (const QJsonValue &angleValue : angles2) {
        QJsonObject angle = angleValue.toObject();
        if (angle.contains("id")) {
            angleMap2[angle["id"].toString()] = angle;
        }
    }

    // Calculate midpoints for common angles
    QStringList angleIds = {"Asc", "MC", "Desc", "IC"};
    for (const QString &id : angleIds) {
        if (angleMap1.contains(id) && angleMap2.contains(id)) {
            QJsonObject angle1 = angleMap1[id];
            QJsonObject angle2 = angleMap2[id];
            double longitude1 = angle1["longitude"].toDouble();
            double longitude2 = angle2["longitude"].toDouble();
            // Handle the case where angles cross 0°/360° boundary
            double diff = fmod(longitude2 - longitude1 + 540.0, 360.0) - 180.0;
            double midpoint = fmod(longitude1 + diff/2.0 + 360.0, 360.0);
            // Determine the sign for the midpoint
            int signIndex = static_cast<int>(midpoint) / 30;
            QStringList signs = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
                                 "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
            QString sign = signs[signIndex];
            // Create a new angle object for the composite chart
            QJsonObject compositeAngle;
            compositeAngle["id"] = id;
            compositeAngle["longitude"] = midpoint;
            compositeAngle["sign"] = sign;
            compositeAngles.append(compositeAngle);
        }
    }

    QJsonArray houses1 = chartData1["houses"].toArray();
    QJsonArray houses2 = chartData2["houses"].toArray();

    // Create maps for quick lookup
    QMap<int, QJsonObject> houseMap1;
    QMap<int, QJsonObject> houseMap2;
    for (const QJsonValue &houseValue : houses1) {
        QJsonObject house = houseValue.toObject();
        QString id = house["id"].toString();
        // Extract house number from id (e.g., "House1" -> 1)
        int houseNumber = id.mid(5).toInt();
        if (houseNumber > 0 && houseNumber <= 12) {
            houseMap1[houseNumber] = house;
        }
    }
    for (const QJsonValue &houseValue : houses2) {
        QJsonObject house = houseValue.toObject();
        QString id = house["id"].toString();
        // Extract house number from id (e.g., "House1" -> 1)
        int houseNumber = id.mid(5).toInt();
        if (houseNumber > 0 && houseNumber <= 12) {
            houseMap2[houseNumber] = house;
        }
    }

    // Calculate the composite Ascendant (House 1)
    QJsonObject house1_1 = houseMap1[1];
    QJsonObject house1_2 = houseMap2[1];
    double longitude1_1 = house1_1["longitude"].toDouble();
    double longitude1_2 = house1_2["longitude"].toDouble();
    double diff1 = fmod(longitude1_2 - longitude1_1 + 540.0, 360.0) - 180.0;
    double compositeAsc = fmod(longitude1_1 + diff1/2.0 + 360.0, 360.0);

    // Now calculate equal houses from the composite Ascendant
    QJsonArray compositeHouses;
    for (int i = 1; i <= 12; i++) {
        // Each house is 30 degrees from the previous one
        double houseLongitude = fmod(compositeAsc + (i-1) * 30.0, 360.0);

        // Determine the sign
        int signIndex = static_cast<int>(houseLongitude) / 30;
        QStringList signs = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
                             "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
        QString sign = signs[signIndex];

        // Create house object
        QJsonObject compositeHouse;
        compositeHouse["id"] = QString("House%1").arg(i);
        compositeHouse["longitude"] = houseLongitude;
        compositeHouse["sign"] = sign;
        compositeHouses.append(compositeHouse);
    }

    // Debug: Print the final composite houses
    for (int i = 0; i < compositeHouses.size(); i++) {
        QJsonObject house = compositeHouses[i].toObject();
    }


    // Assemble the composite chart data
    compositeChartData["planets"] = compositePlanets;
    compositeChartData["angles"] = compositeAngles;
    compositeChartData["houses"] = compositeHouses;

    // Copy aspects from first chart (this is a simplification)
    // In a real implementation, you would recalculate aspects between composite planets
    QJsonArray compositeAspects;
    // Define aspect types and their orbs
    struct AspectType {
        QString name;
        double angle;
        double orb;
    };
    double orbMax = getOrbMax();
    QVector<AspectType> aspectTypes = {
        {"CON", 0.0, orbMax},      // Conjunction
        {"OPP", 180.0, orbMax},    // Opposition
        {"TRI", 120.0, orbMax},    // Trine
        {"SQR", 90.0, orbMax},     // Square
        {"SEX", 60.0, orbMax},     // Sextile
        {"QUI", 150.0, orbMax * 0.75},  // Quintile
        {"SSQ", 45.0, orbMax * 0.75},  // Semi-square
        {"SSX", 30.0, orbMax * 0.75}   // Semi-sextile
    };

    // Check for aspects between each pair of planets
    for (int i = 0; i < compositePlanets.size(); i++) {
        QJsonObject planet1 = compositePlanets[i].toObject();
        for (int j = i + 1; j < compositePlanets.size(); j++) {
            QJsonObject planet2 = compositePlanets[j].toObject();
            double long1 = planet1["longitude"].toDouble();
            double long2 = planet2["longitude"].toDouble();
            // Calculate the angular distance between planets
            double distance = fabs(long1 - long2);
            if (distance > 180.0) distance = 360.0 - distance;
            // Check if this distance matches any aspect type
            for (const AspectType &aspectType : aspectTypes) {
                double orb = fabs(distance - aspectType.angle);
                if (orb <= aspectType.orb) {
                    // Create aspect object
                    QJsonObject aspect;
                    aspect["planet1"] = planet1["id"].toString();
                    aspect["planet2"] = planet2["id"].toString();
                    aspect["aspectType"] = aspectType.name;  // Use "aspectType" not "type"
                    aspect["orb"] = orb;
                    compositeAspects.append(aspect);
                    break; // Only record the closest matching aspect
                }
            }
        }
    }

    // Update the composite chart data with the calculated aspects
    compositeChartData["aspects"] = compositeAspects;

    // Create the full save data structure
    QJsonObject compositeSaveData;
    compositeSaveData["chartData"] = compositeChartData;

    // Add relationship info
    QJsonObject relationshipInfo;
    QString compositeFirstName = name1 + " " + surname1;
    QString compositeLastName = name2 + " " + surname2;
    relationshipInfo["type"] = "Composite";
    relationshipInfo["person1"] = name1 + " " + birthInfo1["lastName"].toString();
    relationshipInfo["person2"] = name2 + " " + birthInfo2["lastName"].toString();
    relationshipInfo["displayName"] = "Composite Chart: " + compositeFirstName + " & " + compositeLastName;
    m_currentRelationshipInfo = relationshipInfo;

    // Calculate midpoint date and time using QDateTime
    QDateTime dateTime1, dateTime2;
    QString dateStr1 = birthInfo1["date"].toString();
    QString timeStr1 = birthInfo1["time"].toString();
    QString dateStr2 = birthInfo2["date"].toString();
    QString timeStr2 = birthInfo2["time"].toString();

    // Handle different date formats
    if (dateStr1.contains("/")) {
        // Format is dd/MM/yyyy
        QDate date1 = QDate::fromString(dateStr1, "dd/MM/yyyy");
        QTime time1 = QTime::fromString(timeStr1, "HH:mm");
        dateTime1 = QDateTime(date1, time1);
    } else {
        // Format is yyyy-MM-dd
        dateTime1 = QDateTime::fromString(dateStr1 + " " + timeStr1, "yyyy-MM-dd HH:mm");
    }
    if (dateStr2.contains("/")) {
        // Format is dd/MM/yyyy
        QDate date2 = QDate::fromString(dateStr2, "dd/MM/yyyy");
        QTime time2 = QTime::fromString(timeStr2, "HH:mm");
        dateTime2 = QDateTime(date2, time2);
    } else {
        // Format is yyyy-MM-dd
        dateTime2 = QDateTime::fromString(dateStr2 + " " + timeStr2, "yyyy-MM-dd HH:mm");
    }

    // Calculate midpoint timestamp (average of Unix timestamps)
    qint64 timestamp1 = dateTime1.toSecsSinceEpoch();
    qint64 timestamp2 = dateTime2.toSecsSinceEpoch();
    qint64 midpointTimestamp = (timestamp1 + timestamp2) / 2;

    // Convert back to QDateTime
    QDateTime midpointDateTime = QDateTime::fromSecsSinceEpoch(midpointTimestamp);

    // Format midpoint date and time
    QString compositeDateStr = midpointDateTime.toString("dd/MM/yyyy");
    QString compositeTimeStr = midpointDateTime.toString("HH:mm");

    // Add to relationshipInfo
    relationshipInfo["date"] = compositeDateStr;
    relationshipInfo["time"] = compositeTimeStr;
    compositeSaveData["relationshipInfo"] = relationshipInfo;

    // Calculate midpoint location
    double lat1 = birthInfo1["latitude"].toString().toDouble();
    double lon1 = birthInfo1["longitude"].toString().toDouble();
    double lat2 = birthInfo2["latitude"].toString().toDouble();
    double lon2 = birthInfo2["longitude"].toString().toDouble();
    double midLat = (lat1 + lat2) / 2.0;
    double midLon = (lon1 + lon2) / 2.0;
    QString compositeLatStr = QString::number(midLat, 'f', 6);
    QString compositeLonStr = QString::number(midLon, 'f', 6);

    // Format Google coordinates string in the proper format
    QString latDirection = midLat >= 0 ? "N" : "S";
    QString lonDirection = midLon >= 0 ? "E" : "W";
    QString compositeGoogleCoords = QString("%1° %2, %3° %4")
                                        .arg(fabs(midLat), 0, 'f', 4)
                                        .arg(latDirection)
                                        .arg(fabs(midLon), 0, 'f', 4)
                                        .arg(lonDirection);

    // Create the composite birth info object
    QJsonObject compositeBirthInfo;
    compositeBirthInfo["firstName"] = compositeFirstName;
    compositeBirthInfo["lastName"] = compositeLastName;
    compositeBirthInfo["date"] = compositeDateStr;  // Midpoint date
    compositeBirthInfo["time"] = compositeTimeStr;  // Midpoint time
    compositeBirthInfo["latitude"] = compositeLatStr;  // Midpoint latitude
    compositeBirthInfo["longitude"] = compositeLonStr;  // Midpoint longitude
    compositeBirthInfo["googleCoords"] = compositeGoogleCoords;  // Formatted Google coordinates

    //utc median
    QString compositeUtcOffsetStr = "+0:00";
    // Set in birth info
    compositeBirthInfo["utcOffset"] = compositeUtcOffsetStr;
    // Set in UI combobox - find the item with +00:00
    int index = m_utcOffsetCombo->findText(compositeUtcOffsetStr);
    if (index >= 0) {
        m_utcOffsetCombo->setCurrentIndex(index);
    } else {
        // If not found, try to find one with "UTC+0" or similar
        index = m_utcOffsetCombo->findText("UTC+0", Qt::MatchContains);
        if (index >= 0) {
            m_utcOffsetCombo->setCurrentIndex(index);
        }
    }

    compositeBirthInfo["houseSystem"] = birthInfo1["houseSystem"].toString();  // Keep first person's house system
    compositeSaveData["birthInfo"] = compositeBirthInfo;

    // Update UI fields for saving
    first_name->setText(compositeFirstName);
    last_name->setText(compositeLastName);
    m_birthDateEdit->setText(compositeDateStr);
    m_birthTimeEdit->setText(compositeTimeStr);
    m_googleCoordsEdit->setText(compositeGoogleCoords);  // Use the formatted Google coordinates

    // Display the chart
    m_currentChartData = compositeChartData;
    displayChart(compositeChartData);
    m_chartCalculated = true;
    populateInfoOverlay();

    // Save the composite chart
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmm");
    QString outputFileName = QString("Composite_%1_%2_%3.astr")
                                 .arg(name1 + surname1)
                                 .arg(name2 + surname2)
                                 .arg(timestamp);
    QDir relationshipDir(appDir + "/RelationshipCharts");
    if (!relationshipDir.exists()) {
        relationshipDir.mkpath(".");
    }
    QString outputFilePath = appDir + "/RelationshipCharts/" + outputFileName;
    QFile outputFile(outputFilePath);
    if (outputFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(compositeSaveData);
        outputFile.write(doc.toJson(QJsonDocument::Indented));
        outputFile.close();
        QMessageBox::information(this, "Chart Saved", "Composite chart saved to:\n" + outputFilePath);
    } else {
        QMessageBox::warning(this, "Save Failed", "Could not save Composite chart to:\n" + outputFilePath);
    }

    // Update window title
    setWindowTitle("Asteria - Astrological Chart Analysis - " + relationshipInfo["displayName"].toString());
}




void MainWindow::createDavisonChart() {
    QMessageBox::information(this, "Select Charts",
                             "Please select two natal charts to create a Davison chart.");
    QString appName = QApplication::applicationName();
    QString appDir;
#ifdef FLATHUB_BUILD
    appDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + appName;
#else
    appDir = QDir::homePath() + "/" + appName;
#endif
    QDir dir;
    if (!dir.exists(appDir))
        dir.mkpath(appDir);

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this, "Select Two Charts", appDir, "Astrological Chart (*.astr)");

    if (filePaths.size() != 2) {
        QMessageBox::warning(this, "Invalid Selection",
                             "You must select exactly two charts.");
        return;
    }

    QJsonObject saveData1, saveData2;
    QFile file1(filePaths[0]);
    if (file1.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file1.readAll());
        file1.close();
        if (doc.isObject()) {
            saveData1 = doc.object();
        } else {
            QMessageBox::critical(this, "Load Error", "Invalid chart file format: " + filePaths[0]);
            return;
        }
    } else {
        QMessageBox::critical(this, "Load Error", "Could not open chart file " + filePaths[0]);
        return;
    }

    QFile file2(filePaths[1]);
    if (file2.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file2.readAll());
        file2.close();
        if (doc.isObject()) {
            saveData2 = doc.object();
        } else {
            QMessageBox::critical(this, "Load Error", "Invalid chart file format: " + filePaths[1]);
            return;
        }
    } else {
        QMessageBox::critical(this, "Load Error", "Could not open chart file " + filePaths[1]);
        return;
    }

    QJsonObject birthInfo1 = saveData1["birthInfo"].toObject();
    QJsonObject birthInfo2 = saveData2["birthInfo"].toObject();
    QString name1 = birthInfo1["firstName"].toString();
    QString surname1 = birthInfo1["lastName"].toString();
    QString name2 = birthInfo2["firstName"].toString();
    QString surname2 = birthInfo2["lastName"].toString();

    // Fixed date/time parsing
    QString dateStr1 = birthInfo1["date"].toString();
    QString timeStr1 = birthInfo1["time"].toString();
    QString dateStr2 = birthInfo2["date"].toString();
    QString timeStr2 = birthInfo2["time"].toString();

    // Try to parse with seconds first
    QDateTime dateTime1 = QDateTime::fromString(dateStr1 + " " + timeStr1, "dd/MM/yyyy HH:mm:ss");
    QDateTime dateTime2 = QDateTime::fromString(dateStr2 + " " + timeStr2, "dd/MM/yyyy HH:mm:ss");

    // If parsing failed, try without seconds
    if (!dateTime1.isValid()) {
        dateTime1 = QDateTime::fromString(dateStr1 + " " + timeStr1, "dd/MM/yyyy HH:mm");
    }
    if (!dateTime2.isValid()) {
        dateTime2 = QDateTime::fromString(dateStr2 + " " + timeStr2, "dd/MM/yyyy HH:mm");
    }

    // Calculate midpoint date
    qint64 midpointTimestamp = (dateTime1.toSecsSinceEpoch() + dateTime2.toSecsSinceEpoch()) / 2;
    QDateTime midpointDateTime = QDateTime::fromSecsSinceEpoch(midpointTimestamp);

    // Calculate the average time separately
    int hour1 = dateTime1.time().hour();
    int minute1 = dateTime1.time().minute();
    int hour2 = dateTime2.time().hour();
    int minute2 = dateTime2.time().minute();
    int totalMinutes1 = hour1 * 60 + minute1;
    int totalMinutes2 = hour2 * 60 + minute2;
    int midpointTotalMinutes = (totalMinutes1 + totalMinutes2) / 2;
    int midpointHour = midpointTotalMinutes / 60;
    int midpointMinute = midpointTotalMinutes % 60;

    // Set the correct time on the midpoint date
    midpointDateTime.setTime(QTime(midpointHour, midpointMinute));

    // Format the date and time for display
    QString midpointDate = midpointDateTime.toString("dd/MM/yyyy");
    QString midpointTime = midpointDateTime.toString("HH:mm");

    double lat1 = birthInfo1["latitude"].toString().toDouble();
    double lon1 = birthInfo1["longitude"].toString().toDouble();
    double lat2 = birthInfo2["latitude"].toString().toDouble();
    double lon2 = birthInfo2["longitude"].toString().toDouble();
    double midpointLat = (lat1 + lat2) / 2.0;
    double lonDiff = fmod(lon2 - lon1 + 540.0, 360.0) - 180.0;
    double midpointLon = fmod(lon1 + lonDiff / 2.0 + 360.0, 360.0);
    if (midpointLon > 180.0) midpointLon -= 360.0;
    QString midpointLatStr = QString::number(midpointLat, 'f', 6);
    QString midpointLonStr = QString::number(midpointLon, 'f', 6);

    QString utcOffsetStr1 = birthInfo1["utcOffset"].toString();
    QString utcOffsetStr2 = birthInfo2["utcOffset"].toString();
    bool neg1 = utcOffsetStr1.startsWith("-");
    bool neg2 = utcOffsetStr2.startsWith("-");
    QStringList parts1 = utcOffsetStr1.mid(1).split(":");
    QStringList parts2 = utcOffsetStr2.mid(1).split(":");
    double hours1 = parts1[0].toDouble() + parts1[1].toDouble() / 60.0;
    double hours2 = parts2[0].toDouble() + parts2[1].toDouble() / 60.0;
    if (neg1) hours1 = -hours1;
    if (neg2) hours2 = -hours2;
    double midpointHours = (hours1 + hours2) / 2.0;
    bool neg = midpointHours < 0;
    double absHours = std::abs(midpointHours);
    int h = static_cast<int>(absHours);
    int m = static_cast<int>((absHours - h) * 60);

    // With this corrected version:
    QString midpointUtcOffsetStr = QString("%1%2:%3")
                                       .arg(neg ? "-" : "+")
                                       .arg(h, 1, 10, QChar('0'))  // Use width 1 to avoid unnecessary padding
                                       .arg(m, 2, 10, QChar('0'));

    QString houseSystem = birthInfo1["houseSystem"].toString();
    QString davisonFirstName = name1 + " " + surname1;
    QString davisonLastName = name2 + " " + surname2;

    first_name->setText(davisonFirstName);
    last_name->setText(davisonLastName);
    m_birthDateEdit->setText(midpointDate);
    m_birthTimeEdit->setText(midpointTime);

    QString latDirection = (midpointLat >= 0) ? "N" : "S";
    QString lonDirection = (midpointLon >= 0) ? "E" : "W";
    QString googleCoords = QString("%1° %2, %3° %4")
                               .arg(qAbs(midpointLat), 0, 'f', 4).arg(latDirection)
                               .arg(qAbs(midpointLon), 0, 'f', 4).arg(lonDirection);
    m_googleCoordsEdit->setText(googleCoords);

    int utcIndex = -1;
    // First try exact match
    utcIndex = m_utcOffsetCombo->findText(midpointUtcOffsetStr);
    // If not found, try with "UTC" prefix
    if (utcIndex < 0) {
        utcIndex = m_utcOffsetCombo->findText("UTC" + midpointUtcOffsetStr);
    }
    // If still not found, try partial match
    if (utcIndex < 0) {
        // Extract the numeric part (e.g., "+1:30" -> "1:30")
        QString numericPart = midpointUtcOffsetStr.mid(1);
        for (int i = 0; i < m_utcOffsetCombo->count(); i++) {
            QString itemText = m_utcOffsetCombo->itemText(i);
            if (itemText.contains(numericPart)) {
                utcIndex = i;
                break;
            }
        }
    }
    // If still not found, try matching just the hour part
    if (utcIndex < 0) {
        QString hourPart = QString::number(h);
        for (int i = 0; i < m_utcOffsetCombo->count(); i++) {
            QString itemText = m_utcOffsetCombo->itemText(i);
            if ((itemText.contains("+" + hourPart) || itemText.contains("-" + hourPart)) &&
                ((neg && itemText.contains("-")) || (!neg && !itemText.contains("-")))) {
                utcIndex = i;
                break;
            }
        }
    }
    // If we found a match, set it
    if (utcIndex >= 0) {
        m_utcOffsetCombo->setCurrentIndex(utcIndex);
    } else {
        // If all else fails, add the calculated offset to the combobox
        m_utcOffsetCombo->addItem("UTC" + midpointUtcOffsetStr);
        m_utcOffsetCombo->setCurrentIndex(m_utcOffsetCombo->count() - 1);
    }

    int houseSystemIndex = m_houseSystemCombo->findText(houseSystem);
    if (houseSystemIndex >= 0)
        m_houseSystemCombo->setCurrentIndex(houseSystemIndex);

    calculateChart();

    QJsonObject relationshipInfo;
    relationshipInfo["type"] = "Davison";
    relationshipInfo["person1"] = name1 + " " + surname1;
    relationshipInfo["person2"] = name2 + " " + surname2;
    relationshipInfo["displayName"] = "Davison Chart: " + davisonFirstName + " & " + davisonLastName;
    m_currentRelationshipInfo = relationshipInfo;

    QJsonObject davisonBirthInfo;
    davisonBirthInfo["firstName"] = davisonFirstName;
    davisonBirthInfo["lastName"] = davisonLastName;
    davisonBirthInfo["date"] = midpointDate;
    davisonBirthInfo["time"] = midpointTime;
    davisonBirthInfo["latitude"] = midpointLatStr;
    davisonBirthInfo["longitude"] = midpointLonStr;
    davisonBirthInfo["utcOffset"] = midpointUtcOffsetStr;
    davisonBirthInfo["houseSystem"] = houseSystem;
    davisonBirthInfo["googleCoords"] = googleCoords;
    //m_googleCoordsEdit->setText(googleCoords);
    QJsonObject saveData;
    saveData["chartData"] = m_currentChartData;
    saveData["birthInfo"] = davisonBirthInfo;
    saveData["relationshipInfo"] = relationshipInfo;  // ← THIS is the only required addition

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmm");
    QString outputFileName = QString("Davison_%1_%2_%3.astr")
                                 .arg(name1 + surname1)
                                 .arg(name2 + surname2)
                                 .arg(timestamp);
    QDir relationshipDir(appDir + "/RelationshipCharts");
    if (!relationshipDir.exists()) {
        relationshipDir.mkpath(".");
    }
    QString outputFilePath = appDir + "/RelationshipCharts/" + outputFileName;

    QFile outputFile(outputFilePath);
    if (outputFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(saveData);
        outputFile.write(doc.toJson(QJsonDocument::Indented));
        outputFile.close();
        QMessageBox::information(this, "Chart Saved", "Davison chart saved to:\n" + outputFilePath);
    } else {
        QMessageBox::warning(this, "Save Failed", "Could not save Davison chart to:\n" + outputFilePath);
    }

    displayChart(m_currentChartData);
    m_chartCalculated = true;
    populateInfoOverlay();
    setWindowTitle("Asteria - Astrological Chart Analysis - " + relationshipInfo["displayName"].toString());
}




void MainWindow::createSynastryChart()
{

}

void MainWindow::showRelationshipChartsDialog()
{
    // Create the dialog only if it doesn't exist yet
    if (!m_relationshipChartsDialog) {
        m_relationshipChartsDialog = new QDialog(this);
        m_relationshipChartsDialog->setWindowTitle("About Relationship Charts");
        m_relationshipChartsDialog->setMinimumSize(500, 400);

        // Create layout
        QVBoxLayout *layout = new QVBoxLayout(m_relationshipChartsDialog);

        // Create a text browser for rich text display
        QTextBrowser *textBrowser = new QTextBrowser(m_relationshipChartsDialog);
        textBrowser->setOpenExternalLinks(true);

        // Set the help content
        QString helpText = R"(
        <h2>Understanding Relationship Charts</h2>

        <h3>Composite Charts</h3>
        <p>A Composite Chart represents the midpoints between two people's natal charts and shows the energy of the relationship itself as a separate entity.</p>

        <p><b>How it's calculated:</b> For each planet and point, the Composite Chart takes the midpoint between the same planets in both individuals' charts. For example, if Person A's Sun is at 15° Aries and Person B's Sun is at 15° Libra, the Composite Sun would be at 15° Cancer (the midpoint).</p>

        <p><b>Important Note:</b> When viewing a Composite Chart in Asteria, the birth information fields are populated with placeholder values only. These values are not used in the actual calculation of the chart. <span style="color:red;font-weight:bold;">Do not attempt to recalculate the chart</span> using these placeholder values, as this will produce an incorrect chart.</p>

        <p><b>Purpose:</b> The Composite Chart reveals the purpose and potential of the relationship. It shows how two people function as a unit and the shared destiny or path of the relationship.</p>

        <p><b>Insights offered:</b></p>
        <ul>
            <li>The relationship's inherent strengths and challenges</li>
            <li>The purpose or mission of the relationship</li>
            <li>How others perceive you as a couple</li>
            <li>The natural dynamics that emerge when you're together</li>
        </ul>

        <h3>Davison Relationship Charts</h3>
        <p>A Davison Chart creates a hypothetical birth chart for the relationship by finding the midpoint between two people's birth times and locations.</p>

        <p><b>How it's calculated:</b> The Davison Chart uses the average of:</p>
        <ul>
            <li>The two birth dates (midpoint in time)</li>
            <li>The two birth times (midpoint in time)</li>
            <li>The two birth locations (midpoint in space - latitude and longitude)</li>
        </ul>

        <p><b>Purpose:</b> The Davison Chart treats the relationship as if it were a person with its own birth chart. It shows the relationship's inherent nature and potential evolution over time.</p>

        <p><b>Insights offered:</b></p>
        <ul>
            <li>The relationship's innate character and development potential</li>
            <li>How the relationship responds to transits and progressions</li>
            <li>The relationship's timing and life cycles</li>
            <li>A more dynamic view of the relationship as an evolving entity</li>
        </ul>

        <h3>Which Chart to Use?</h3>
        <p>Both charts offer valuable insights:</p>
        <ul>
            <li><b>Composite:</b> Better for understanding the relationship's purpose and inherent dynamics</li>
            <li><b>Davison:</b> Better for timing events in the relationship and understanding its evolution</li>
        </ul>

        <p>For a complete relationship analysis, it's beneficial to examine both charts alongside the synastry (planet-to-planet aspects) between the individual natal charts.</p>
        )";

        textBrowser->setHtml(helpText);
        layout->addWidget(textBrowser);

        // Add a close button at the bottom
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_relationshipChartsDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // Connect the close button
        connect(closeButton, &QPushButton::clicked, m_relationshipChartsDialog, &QDialog::close);

        // Connect the dialog's finished signal to handle cleanup
        connect(m_relationshipChartsDialog, &QDialog::finished, this, [this]() {
            m_relationshipChartsDialog->deleteLater();
            m_relationshipChartsDialog = nullptr;
        });
    }

    // Show and raise the dialog to bring it to the front
    m_relationshipChartsDialog->show();
    m_relationshipChartsDialog->raise();
    m_relationshipChartsDialog->activateWindow();

}

QJsonObject MainWindow::loadChartForRelationships(const QString &filePath) {
    QJsonObject chartData;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject saveData = doc.object();
            // Load chart data
            if (saveData.contains("chartData") && saveData["chartData"].isObject()) {
                chartData = saveData["chartData"].toObject();
            }
            // Load birth information
            if (saveData.contains("birthInfo") && saveData["birthInfo"].isObject()) {
                chartData["birthInfo"] = saveData["birthInfo"].toObject();
            }
        }
    }

    return chartData;
}

