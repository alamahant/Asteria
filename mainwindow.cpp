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
#include <QRegularExpression>
#include<QClipboard>
#include<QDrag>
#include<QDragEnterEvent>
#include<QDragLeaveEvent>
#include<QDragMoveEvent>
#include<QDropEvent>
#include<QMimeData>
#include<QProcess>

extern QString g_astroFontFamily;

const QRegularExpression MainWindow::dateRegex(
        R"(^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(000[1-9]|00[1-9][0-9]|0[1-9][0-9]{2}|[12][0-9]{3}|3000)$)"
);



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chartCalculated(false)
    , m_transitDialog(nullptr)
    , m_dragStartPosition(0, 0) // Initialize drag start position
{
    setAcceptDrops(true);
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
    //this->setMinimumSize(1200, 800);
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
    if (m_transitSearchDialog) {
        delete m_transitSearchDialog;
        m_transitSearchDialog = nullptr;
    }
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

    // drag drop
    m_chartView->setAcceptDrops(true);
    //m_chartView->viewport()->setMouseTracking(true);
    //m_chartView->installEventFilter(this);
    m_chartView->viewport()->installEventFilter(this);
    //

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

    // Angles table
    QTableWidget *anglesTable = new QTableWidget(0, 3, detailsTabs);
    anglesTable->setObjectName("Angles");
    anglesTable->setHorizontalHeaderLabels({"Angle", "Sign", "Raw Degrees in Dec"});
    anglesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    // Houses table
    QTableWidget *housesTable = new QTableWidget(0, 3, detailsTabs);
    housesTable->setObjectName("Houses");
    housesTable->setHorizontalHeaderLabels({"House", "Sign", "Raw Degrees in Dec"});
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


    // Eclipse Data table
    QTableWidget *eclipseTable = new QTableWidget(0, 6, detailsTabs);
    eclipseTable->setObjectName("Eclipses");
    eclipseTable->setHorizontalHeaderLabels({"Date", "Time", "Type", "Magnitude", "Latitude", "Longitude"});
    eclipseTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // make all tables copiable
    QList<QTableWidget*> tables = {planetsTable, anglesTable, housesTable, aspectsTable, rawTransitTable, eclipseTable};

    for (QTableWidget *table : tables) {
        table->setSelectionBehavior(QAbstractItemView::SelectItems);
        table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setContextMenuPolicy(Qt::ActionsContextMenu);

        QAction *copyAction = new QAction("Copy", table);
        copyAction->setShortcut(QKeySequence::Copy);
        table->addAction(copyAction);

        connect(copyAction, &QAction::triggered, [table]() {
            QItemSelectionModel *selection = table->selectionModel();
            QModelIndexList indexes = selection->selectedIndexes();

            if (indexes.isEmpty())
                return;

            // Sort indexes by row and column
            std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
                if (a.row() == b.row())
                    return a.column() < b.column();
                return a.row() < b.row();
            });

            QString copiedText;
            int prevRow = indexes.first().row();
            for (int i = 0; i < indexes.size(); ++i) {
                const QModelIndex &index = indexes.at(i);
                if (i > 0) {
                    if (index.row() != prevRow) {
                        copiedText += '\n';
                        prevRow = index.row();
                    } else {
                        copiedText += '\t';
                    }
                }
                QTableWidgetItem *item = table->item(index.row(), index.column());
                copiedText += item ? item->text() : "";
            }

            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(copiedText);
        });
    }

    // Add tables to tabs
    detailsTabs->addTab(planetsTable, "Planets");
    detailsTabs->addTab(anglesTable, "Angles");
    detailsTabs->addTab(housesTable, "Houses");
    detailsTabs->addTab(aspectsTable, "Aspects");
    detailsTabs->addTab(rawTransitTable, "Raw Transit Data");
    detailsTabs->addTab(eclipseTable, "Eclipses");

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
    m_inputDock->setWindowTitle("Birth Chart Input");
    m_inputDock->setTitleBarWidget(titleLabel);
    m_inputDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_inputDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    //m_inputDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QWidget *inputWidget = new QWidget(m_inputDock);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputWidget);

    // Birth information group
    QGroupBox *birthGroup = new QGroupBox("Birth Details", inputWidget);

    QFormLayout *birthLayout = new QFormLayout(birthGroup);

    // Date input as QLineEdit with regex validation
    m_birthDateEdit = new QLineEdit(birthGroup);
    m_birthDateEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format.\n"
                                "Allowed range 0001-3000. Allowed format 'dd/MM/yyyy' Year MUST be in four digit yyyy format.");

    m_birthDateEdit->setPlaceholderText("DD/MM/YYYY");
    // Create a validator for the date format. dateRegex is defined at the top of the class
    QValidator *dateValidator = new QRegularExpressionValidator(dateRegex, this);
    m_birthDateEdit->setValidator(dateValidator);

    // Set current date as default
    QDate currentDate = QDate::currentDate();
    m_birthDateEdit->setText(currentDate.toString("dd/MM/yyyy"));

    // Time input as QLineEdit with regex validation
    m_birthTimeEdit = new QLineEdit(birthGroup);
    m_birthTimeEdit->setToolTip("To set new time, highlight and delete the existing time and set desired with proper format.\n"
                                "Allowed format 'HH:mm'");

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
    connect(m_additionalBodiesCB, &QCheckBox::toggled, this, [this](bool checked) {

        GlobalFlags::additionalBodiesEnabled = checked;

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
    /*
    getPredictionButton = new QPushButton("Get AI Prediction", predictiveGroup);
    getPredictionButton->setToolTip("This operation generates a huge ammount of data that is sent to AI for interpretation.\n"
                                    "Therefore it may be costly tokenwise.\n To mitigate this please reduce the number of days and/or use smaller orb.");
    getPredictionButton->setEnabled(false);
    getPredictionButton->setIcon(QIcon::fromTheme("view-refresh"));
    getPredictionButton->setStatusTip("The AI prediction will be appended at the end of any existing text. Scroll down and be patient!");
    predictiveLayout->addRow(getPredictionButton);
    */
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // Prediction Button
    getPredictionButton = new QPushButton("Get AI Prediction", predictiveGroup);
    getPredictionButton->setToolTip("This operation generates a huge ammount of data that is sent to AI for interpretation.\n"
                                    "Therefore it may be costly tokenwise.\n To mitigate this please reduce the number of days and/or use smaller orb.");
    getPredictionButton->setEnabled(false);
    getPredictionButton->setIcon(QIcon::fromTheme("view-refresh"));
    getPredictionButton->setStatusTip("The AI prediction will be appended at the end of any existing text. Scroll down and be patient!");

    // Transit Chart Button
    getTransitsButton = new QPushButton("Calculate Transits", predictiveGroup);
    getTransitsButton->setToolTip("Calculate transits for the selected period");
    getTransitsButton->setEnabled(false);
    getTransitsButton->setIcon(QIcon::fromTheme("view-chart"));
    getTransitsButton->setStatusTip("Calculate transits for the selected period");

    // Add buttons to horizontal layout
    buttonLayout->addWidget(getPredictionButton);
    buttonLayout->addWidget(getTransitsButton);

    // Add the button layout to the form
    predictiveLayout->addRow(buttonLayout);

    // Connect the new button
    connect(getTransitsButton, &QPushButton::clicked, this, &MainWindow::CalculateTransits);




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
    m_getInterpretationButton = new QPushButton("Get Chart Interpretation From AI", interpretationWidget);
    m_getInterpretationButton->setIcon(QIcon::fromTheme("system-search"));
    m_getInterpretationButton->setEnabled(false);

    // Interpretation text area
    m_interpretationtextEdit = new QTextEdit(interpretationWidget);
    m_interpretationtextEdit->setAcceptRichText(true);
    m_interpretationtextEdit->setReadOnly(true);
    m_interpretationtextEdit->setPlaceholderText("AI interpretation will appear here after you click the 'Get Chart Interpretation From AI' button.");

    // Add Language Button
    QHBoxLayout* languageLayout = new QHBoxLayout();
    //QLabel* languageLabel = new QLabel("Language:", interpretationWidget);
    languageComboBox = new QComboBox(interpretationWidget);
    languageComboBox->setToolTip("Select AI Response Language");
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
    languageComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    //add clear button
    QPushButton *clearTextButton = new QPushButton("ClearText", this);
    clearTextButton->setToolTip("Clear AI Interpretation Text Area");
    clearTextButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    // Connect using a lambda
    connect(clearTextButton, &QPushButton::clicked, this, [this]() {
        m_currentInterpretation.clear();              // Clear your stored interpretations
        m_interpretationtextEdit->clear();           // Clear the QTextEdit content
    });

    //languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(languageComboBox);

    // Add widgets to layout
    interpretationLayout->addWidget(m_getInterpretationButton);
    interpretationLayout->addWidget(m_interpretationtextEdit);
    interpretationLayout->addLayout(languageLayout);
    interpretationLayout->addWidget(clearTextButton);

    // Set widget as dock content
    m_interpretationDock->setWidget(interpretationWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_interpretationDock);
}



void MainWindow::setupMenus()
{

    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    // Open new app window

    // New Window action
    QAction *newWindowAction = fileMenu->addAction("New &Window in New Process");
    newWindowAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    newWindowAction->setIcon(QIcon::fromTheme("window-new"));
    newWindowAction->setStatusTip("Open a new application window");

    // Connect using lambda
    connect(newWindowAction, &QAction::triggered, this, [this]() {
        // Create and show a new MainWindow instance
        QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
    });

    fileMenu->addSeparator();


    // open window in current process
    QAction *newSameProcessAction = fileMenu->addAction("New &Window");
    newSameProcessAction->setShortcut(QKeySequence("Ctrl+Shift+W"));
    newSameProcessAction->setIcon(QIcon::fromTheme("window-new"));
    newSameProcessAction->setStatusTip("Open a new window within current application");

    connect(newSameProcessAction, &QAction::triggered, this, [this]() {
        MainWindow *newWindow = new MainWindow();
        newWindow->show();
    });

    fileMenu->addSeparator();

    //open existing chart in new window- same as drag-drop
    QAction *openChartInNewWindowAction = fileMenu->addAction("Open Chart in New &Window");
    openChartInNewWindowAction->setShortcut(QKeySequence("Ctrl+Alt+O"));
    openChartInNewWindowAction->setIcon(QIcon::fromTheme("window-duplicate"));
    openChartInNewWindowAction->setStatusTip("Open current chart with all data in a new window");

    connect(openChartInNewWindowAction, &QAction::triggered, this, [this]() {
        if (!m_chartCalculated) {
            QMessageBox::information(this, "No Chart", "Please calculate a chart first.");
            return;
        }

        // Create the same data structure as drag operation
        QJsonObject chartData;
        chartData["birthDate"] = m_birthDateEdit->text();
        chartData["birthTime"] = m_birthTimeEdit->text();
        chartData["utcOffset"] = m_utcOffsetCombo->currentText();
        chartData["latitude"] = m_latitudeEdit->text();
        chartData["longitude"] = m_longitudeEdit->text();
        chartData["houseSystem"] = m_houseSystemCombo->currentText();
        chartData["useJulian"] = useJulianForPre1582Action->isChecked();
        chartData["chartType"] = GlobalFlags::lastGeneratedChartType;

        // Include interpretation text if available
        if (m_interpretationtextEdit && !m_interpretationtextEdit->toPlainText().isEmpty()) {
            chartData["interpretationText"] = m_interpretationtextEdit->toPlainText();
        }

        chartData["chartData"] = m_currentChartData;

        // Create new window and import the data
        MainWindow *newWindow = new MainWindow();
        newWindow->importChartInputData(chartData);
        newWindow->show();

        statusBar()->showMessage("Chart opened in new window", 3000);
    });
    fileMenu->addSeparator();

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

    QAction *exportDataAction = fileMenu->addAction("Export Chart &Data as Text...", this, &MainWindow::exportChartData);
    exportDataAction->setIcon(QIcon::fromTheme("text-x-generic"));

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

    // Add "Chart Only" toggle action
    m_chartOnlyAction = new QAction("&View Chart Only", this);
    m_chartOnlyAction->setCheckable(true);
    m_chartOnlyAction->setChecked(false); // Default to not checked
    connect(m_chartOnlyAction, &QAction::toggled, this, &MainWindow::toggleChartOnlyView);
    // show infooverlay action
    viewMenu->addAction(m_chartOnlyAction);

    showOverlayAction = new QAction("Show &Info Overlay", this);
    showOverlayAction->setCheckable(true);
    showOverlayAction->setChecked(false); // Default to not checked (hidden)

    // Connect with lambda that handles both UI and settings
    connect(showOverlayAction, &QAction::toggled, this, [this](bool checked) {
        m_showInfoOverlay = checked;
        if(chartInfoOverlay && m_chartCalculated)
            chartInfoOverlay->setVisible(checked);
    });

    viewMenu->addAction(showOverlayAction);



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

    // Add the Julian/Gregorian checkbox
    useJulianForPre1582Action = new QAction(tr("Use Julian calendar for dates before 15 October 1582"), this);
    useJulianForPre1582Action->setCheckable(true);
    QSettings settings;
    if (settings.contains("useJulianForPre1582")) {
        useJulianForPre1582Action->setChecked(settings.value("useJulianForPre1582").toBool());
    } else {
        useJulianForPre1582Action->setChecked(true); // Default: checked (use Julian for pre-1582)
    }
    settingsMenu->addAction(useJulianForPre1582Action);

    // Persist state changes to QSettings
    connect(useJulianForPre1582Action, &QAction::toggled, this, [this](bool checked) {
        qDebug() << "checked";
        QSettings settings;
        settings.setValue("useJulianForPre1582", checked);
    });
    // Create Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("Tools");

    // Create Relationship Charts submenu
    QMenu *relationshipMenu = toolsMenu->addMenu("Relationship Charts");

    // Create actions for relationship chart types
    QAction *compositeAction = new QAction("Composite Chart (exp)", this);
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

    QAction *changelogAction = helpMenu->addAction(tr("Changelog"));
    connect(changelogAction, &QAction::triggered, this, &MainWindow::showChangelog);


    QAction *newFeaturesAction = helpMenu->addAction(tr("What's New!"));
    connect(newFeaturesAction, &QAction::triggered, this, &MainWindow::showNewFeaturesDialog);

    QAction *transitFilterAction = new QAction("Transit Filter", this);
    transitFilterAction->setToolTip("Filter transit data by date, planets and aspects");
    transitFilterAction->setStatusTip("Open transit filter dialog");
    transitFilterAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(transitFilterAction, &QAction::triggered, this, &MainWindow::openTransitFilter);
    toolsMenu->addAction(transitFilterAction);

    QAction *eclipseCalcAction = new QAction("Calculate Eclipses", this);
    eclipseCalcAction->setToolTip("Calculate solar and lunar eclipses in the selected date range");
    eclipseCalcAction->setStatusTip("Calculate eclipses for the current chart and date range");
    eclipseCalcAction->setShortcut(QKeySequence("Ctrl+Shift+E"));

    connect(eclipseCalcAction, &QAction::triggered, this, &MainWindow::CalculateEclipses);

    toolsMenu->addAction(eclipseCalcAction);

    // Add Return Charts submenu
    QMenu *returnChartsMenu = toolsMenu->addMenu(tr("Return Charts"));

    QAction *solarReturnCalcAction = new QAction("Calculate Solar Return", this);
    solarReturnCalcAction->setToolTip("Calculate the solar return chart for a selected year");
    solarReturnCalcAction->setStatusTip("Calculate the solar return chart for the current birth data and chosen year");
    solarReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+H"));

    connect(solarReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateSolarReturn);

    returnChartsMenu->addAction(solarReturnCalcAction);

    QAction *lunarReturnCalcAction = new QAction("Calculate Lunar Return", this);
    lunarReturnCalcAction->setToolTip("Calculate the lunar return chart for a selected month and year");
    lunarReturnCalcAction->setStatusTip("Calculate the lunar return chart for the current birth data and chosen month/year");
    lunarReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+L"));

    connect(lunarReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateLunarReturn);

    returnChartsMenu->addAction(lunarReturnCalcAction);

    QAction *saturnReturnCalcAction = new QAction("Calculate Saturn Return", this);
    saturnReturnCalcAction->setToolTip("Calculate the Saturn return chart for a selected return number");
    saturnReturnCalcAction->setStatusTip("Calculate the Saturn return chart for the current birth data and chosen return number");
    saturnReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+S")); // Choose a shortcut that doesn't conflict
    connect(saturnReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateSaturnReturn);

    returnChartsMenu->addAction(saturnReturnCalcAction);

    QAction *jupiterReturnCalcAction = new QAction("Calculate Jupiter Return", this);
    jupiterReturnCalcAction->setToolTip("Calculate the Jupiter return chart for a selected return number");
    jupiterReturnCalcAction->setStatusTip("Calculate the Jupiter return chart for the current birth data and chosen return number");
    jupiterReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+J"));
    connect(jupiterReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateJupiterReturn);
    returnChartsMenu->addAction(jupiterReturnCalcAction);

    // Venus Return
    QAction *venusReturnCalcAction = new QAction("Calculate Venus Return", this);
    venusReturnCalcAction->setToolTip("Calculate the Venus return chart for a selected return number");
    venusReturnCalcAction->setStatusTip("Calculate the Venus return chart for the current birth data and chosen return number");
    venusReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+V"));
    connect(venusReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateVenusReturn);
    returnChartsMenu->addAction(venusReturnCalcAction);

    // Mars Return
    QAction *marsReturnCalcAction = new QAction("Calculate Mars Return", this);
    marsReturnCalcAction->setToolTip("Calculate the Mars return chart for a selected return number");
    marsReturnCalcAction->setStatusTip("Calculate the Mars return chart for the current birth data and chosen return number");
    marsReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+R"));
    connect(marsReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateMarsReturn);
    returnChartsMenu->addAction(marsReturnCalcAction);

    // Mercury Return
    QAction *mercuryReturnCalcAction = new QAction("Calculate Mercury Return", this);
    mercuryReturnCalcAction->setToolTip("Calculate the Mercury return chart for a selected return number");
    mercuryReturnCalcAction->setStatusTip("Calculate the Mercury return chart for the current birth data and chosen return number");
    mercuryReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+M"));
    connect(mercuryReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateMercuryReturn);
    returnChartsMenu->addAction(mercuryReturnCalcAction);

    QAction *uranusReturnCalcAction = new QAction("Calculate Uranus Return", this);
    uranusReturnCalcAction->setToolTip("Calculate the Uranus return chart for a selected return number");
    uranusReturnCalcAction->setStatusTip("Calculate the Uranus return chart for the current birth data and chosen return number");
    uranusReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+U"));
    connect(uranusReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateUranusReturn);
    returnChartsMenu->addAction(uranusReturnCalcAction);

    QAction *neptuneReturnCalcAction = new QAction("Calculate Neptune Return", this);
    neptuneReturnCalcAction->setToolTip("Calculate the Neptune return chart for a selected return number");
    neptuneReturnCalcAction->setStatusTip("Calculate the Neptune return chart for the current birth data and chosen return number");
    neptuneReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+N"));
    connect(neptuneReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateNeptuneReturn);
    returnChartsMenu->addAction(neptuneReturnCalcAction);

    QAction *plutoReturnCalcAction = new QAction("Calculate Pluto Return", this);
    plutoReturnCalcAction->setToolTip("Calculate the Pluto return chart for a selected return number");
    plutoReturnCalcAction->setStatusTip("Calculate the Pluto return chart for the current birth data and chosen return number");
    plutoReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+P"));
    connect(plutoReturnCalcAction, &QAction::triggered, this, &MainWindow::calculatePlutoReturn);
    returnChartsMenu->addAction(plutoReturnCalcAction);

    //Secondary Progression Chart
    QAction *secondaryProgressionAction = new QAction("Calculate Secondary Progression Chart", this);
    secondaryProgressionAction->setToolTip("Calculate a secondary progression chart for a selected year of life");
    secondaryProgressionAction->setStatusTip("Calculate the secondary progression chart for the current birth data and chosen progression year");
    secondaryProgressionAction->setShortcut(QKeySequence("Ctrl+G")); // Choose a shortcut that doesn't conflict
    connect(secondaryProgressionAction, &QAction::triggered, this, &MainWindow::calculateSecondaryProgression);
    toolsMenu->insertAction(nullptr, secondaryProgressionAction); // Add at the top of Tools menu

    // Current Chart
    QAction *zodiacChartAction = new QAction("Calculate Zodiac Chart", this);
    zodiacChartAction->setToolTip("Calculate a chart for all Zodiac Signs");
    //zodiacChartAction->setStatusTip("Calculate the current chart using the current date/time and entered location");
    zodiacChartAction->setShortcut(QKeySequence("Ctrl+H")); // Choose a shortcut that doesn't conflict
    connect(zodiacChartAction, &QAction::triggered, this, &MainWindow::calculateZodiacSignsChart);
    toolsMenu->insertAction(nullptr, zodiacChartAction); // Add at the top of Tools menu

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
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }
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
    birthDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {

        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        // Set chart type for interpretation

        GlobalFlags::lastGeneratedChartType = "Natal Birth";

        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);
        // Clear previous interpretation
        //m_currentInterpretation.clear();
        //m_interpretationtextEdit->clear();
        //m_interpretationtextEdit->setPlaceholderText("Click 'Get AI Interpretation' to analyze this chart.");
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
    chartInfoOverlay->setVisible(m_showInfoOverlay);
    populateInfoOverlay();

    // Switch to chart tab
    m_centralTabWidget->setCurrentIndex(0);
    // this->setWindowTitle("Asteria - Astrological Chart Analysis - Birth Chart");
}


void MainWindow::updateChartDetailsTables(const QJsonObject &chartData)
{
    // Get table widgets
    QTableWidget *planetsTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Planets");
    QTableWidget *anglesTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Angles");
    QTableWidget *housesTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Houses");
    QTableWidget *aspectsTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Aspects");

    if (!planetsTable || !housesTable || !aspectsTable || ! anglesTable) {
        return;
    }

    // Clear tables
    planetsTable->setRowCount(0);
    anglesTable->setRowCount(0);
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

            // Split the sign string to get just the sign name and degrees
            QString fullSign = planet["sign"].toString();
            QString signName = fullSign.split(' ').first();
            QStringList parts = fullSign.split(' ');
            QString degreesPart = parts.size() > 1 ? parts.mid(1).join(' ') : "";

            QTableWidgetItem *nameItem = new QTableWidgetItem(planetName);
            QTableWidgetItem *signItem = new QTableWidgetItem(signName);
            QTableWidgetItem *degreeItem = new QTableWidgetItem(degreesPart);
            QTableWidgetItem *houseItem = new QTableWidgetItem(planet["house"].toString());



            planetsTable->setItem(i, 0, nameItem);
            planetsTable->setItem(i, 1, signItem);
            planetsTable->setItem(i, 2, degreeItem);
            planetsTable->setItem(i, 3, houseItem);
        }
    }

    // Mapping from angle IDs to long names
    QMap<QString, QString> angleLongNames = {
        {"Asc", "Ascendant"},
        {"Desc", "Descendant"},
        {"MC", "Medium Coeli (Midheaven)"},
        {"IC", "Imum Coeli (Nadir)"}
    };
    // Fill angles table
    if (chartData.contains("angles") && chartData["angles"].isArray()) {
        QJsonArray angles = chartData["angles"].toArray();
        anglesTable->setRowCount(angles.size());
        for (int i = 0; i < angles.size(); ++i) {
            QJsonObject angle = angles[i].toObject();
            QString angleName = angle["id"].toString(); // e.g., ASC, DESC, IC, MC
            QString longName = angleLongNames.value(angleName, angleName); // fallback to id if not found

            QString signName = angle["sign"].toString();
            QString degreeStr = QString::number(angle["longitude"].toDouble(), 'f', 2) + "°";
            //QTableWidgetItem *nameItem = new QTableWidgetItem(angleName);
            QTableWidgetItem *nameItem = new QTableWidgetItem(longName);


            QTableWidgetItem *signItem = new QTableWidgetItem(signName);
            QTableWidgetItem *degreeItem = new QTableWidgetItem(degreeStr);
            anglesTable->setItem(i, 0, nameItem);
            anglesTable->setItem(i, 1, signItem);
            anglesTable->setItem(i, 2, degreeItem);
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

/*
void MainWindow::displayInterpretation(const QString &interpretation)
{
    m_currentInterpretation += interpretation;
    m_interpretationtextEdit->append(
        "\nChart reading for " + first_name->text() + " " + last_name->text() +
        " born on " + m_birthDateEdit->text() +
        " at " + m_birthTimeEdit->text() +
        " in location " + m_googleCoordsEdit->text() + "\n"
        );
    //m_interpretationtextEdit->append("\n" + interpretation);
    m_interpretationtextEdit->append("\n" + interpretation);

    m_getInterpretationButton->setEnabled(true);

    m_interpretationtextEdit->append("\nReceived interpretation from AI...");
        statusBar()->showMessage("Interpretation received", 3000);
}
*/


void MainWindow::displayInterpretation(const QString &interpretation)
{
    m_currentInterpretation += interpretation;

    // Convert the AI response from Markdown to HTML
    QString htmlInterpretation = markdownToHtml(interpretation);

    // Get the existing text and convert it to HTML
    //QString existingText = m_interpretationtextEdit->toPlainText();
    //QString existingHtml = plainTextToHtml(existingText);
    QString existingHtml = m_interpretationtextEdit->toHtml();

    // Build the new header as HTML
    QString header = QString(
                "<p><b>Chart reading for %1 %2</b> born on %3 at %4 in location %5</p>"
                ).arg(
                first_name->text(),
                last_name->text(),
                m_birthDateEdit->text(),
                m_birthTimeEdit->text(),
                m_googleCoordsEdit->text()
                );

    // Combine everything into full HTML
    QString fullHtml = existingHtml + "\n" + header + "\n" + htmlInterpretation + "\n" +
            "<p><i>Received interpretation from AI...</i></p>";

    // Set the complete HTML content
    m_interpretationtextEdit->setAcceptRichText(true);
    m_interpretationtextEdit->setHtml(fullHtml);

    m_getInterpretationButton->setEnabled(true);
    statusBar()->showMessage("Interpretation received", 3000);
}

// Helper function to convert plain text to basic HTML
QString MainWindow::plainTextToHtml(const QString &plainText)
{
    if (plainText.isEmpty()) return "";

    QString html = plainText;
    // Convert line breaks to HTML paragraphs
    html.replace("\n\n", "</p><p>");
    html.replace("\n", "<br>");
    return "<p>" + html + "</p>";
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

            // Load interpretation
            if (saveData.contains("interpretation") && saveData["interpretation"].isString()) {
                m_currentInterpretation = saveData["interpretation"].toString();
                //m_interpretationtextEdit->setPlainText(m_currentInterpretation);
                //QString htmlInterpretation = markdownToHtml(m_currentInterpretation);
                //m_interpretationtextEdit->setHtml(htmlInterpretation);
                displayInterpretation(m_currentInterpretation);
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

/*
void MainWindow::showAboutDialog()
{
    QString version = QCoreApplication::applicationVersion();
    QMessageBox::about(this, "About Asteria",
                       "Asteria - Astrological Chart Analysis\n\n"
                       "Version 2.1.1\n\n"
                       "A tool for calculating and interpreting astrological charts "
                       "with AI-powered analysis.\n\n"
                       "© 2025 Alamahant");
}
*/void MainWindow::showAboutDialog()
{
    QString version = QCoreApplication::applicationVersion();
    QMessageBox::about(
        this,
        "About Asteria",
        QString("Asteria - Astrological Chart Analysis\n\n"
                "Version %1\n\n"
                "A tool for calculating and interpreting astrological charts "
                "with AI-powered analysis.\n\n"
                "© 2025 Alamahant")
            .arg(version)
    );
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

    // Save info overlay visibility setting
    settings.setValue("view/showInfoOverlay", m_showInfoOverlay);

    AspectSettings::instance().saveToSettings(settings);

}

void MainWindow::loadSettings()
{
    QSettings settings;

    // Restore window state

    /*
    if (settings.contains("mainWindow/geometry")) {
        restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    }

    if (settings.contains("mainWindow/windowState")) {
        restoreState(settings.value("mainWindow/windowState").toByteArray());
    }
    */

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

    //restore infooverlay visibility
    // Load info overlay visibility setting
    if (settings.contains("view/showInfoOverlay")) {
        m_showInfoOverlay = settings.value("view/showInfoOverlay").toBool();
    } else {
        m_showInfoOverlay = false; // Default to false if setting doesn't exist
    }

    // Update the action checkbox state
    if (showOverlayAction) {
        showOverlayAction->setChecked(m_showInfoOverlay);
    }

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

/*
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
*/

void MainWindow::displayTransitInterpretation(const QString &interpretation) {
    m_currentInterpretation += interpretation;

    // Convert the transit interpretation from Markdown to HTML
    QString htmlInterpretation = markdownToHtml(interpretation);

    // Get the existing content as HTML directly
    QString existingHtml = m_interpretationtextEdit->toHtml();

    // Build the header as HTML
    QString header = QString(
                "<p><b>Astrological Prediction reading for %1 %2</b> born on %3 at %4 "
                "in location %5 for the period from %6 to %7</p>"
                ).arg(
                first_name->text(),
                last_name->text(),
                m_birthDateEdit->text(),
                m_birthTimeEdit->text(),
                m_googleCoordsEdit->text(),
                m_predictiveFromEdit->text(),
                m_predictiveToEdit->text()
                );

    // Combine everything into a single HTML string
    QString fullHtml = existingHtml + "\n" + header + "\n" + htmlInterpretation + "\n" +
            "<p><i>Transit interpretation received</i></p>";

    m_interpretationtextEdit->setAcceptRichText(true);
    m_interpretationtextEdit->setHtml(fullHtml);

    statusBar()->showMessage("Transit interpretation complete", 3000);
    getPredictionButton->setEnabled(true);
}

void MainWindow::populateInfoOverlay() {
    chartInfoOverlay->setVisible(m_showInfoOverlay);
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
                    //m_sunSignLabel->setText(QString("Sun: %1 %2°").arg(sunSign).arg(sunDegree, 0, 'f', 1));
                    m_sunSignLabel->setText(QString("Sun: %1").arg(sunSign));

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
                    //m_ascendantLabel->setText(QString("Asc: %1 %2°").arg(ascSign).arg(ascDegree, 0, 'f', 1));
                    m_ascendantLabel->setText(QString("Asc: %1").arg(ascSign));

                    break;
                }
            }
        }
    }
    m_housesystemLabel->setText(m_houseSystemCombo->currentText());
}

void MainWindow::displayRawTransitData(const QJsonObject &transitData) {
    QString rawData = transitData["rawTransitData"].toString();
    rawTransitTable->setRowCount(0);

    QStringList lines = rawData.split('\n', Qt::SkipEmptyParts);
    bool inTransitsSection = false;
    QDate currentDate;

    for (const QString &line : lines) {
        if (line.startsWith("---TRANSITS---")) {
            inTransitsSection = true;
            continue;
        }
        if (!inTransitsSection) {
            continue;
        }

        if (line.contains(QRegularExpression("^\\d{4}/\\d{2}/\\d{2}:"))) {
            QRegularExpression dateRe("^(\\d{4}/\\d{2}/\\d{2}):");
            QRegularExpressionMatch dateMatch = dateRe.match(line);
            if (dateMatch.hasMatch()) {
                currentDate = QDate::fromString(dateMatch.captured(1), "yyyy/MM/dd");
            }

            int startPos = line.indexOf(':') + 1;
            QString aspectsStr = line.mid(startPos).trimmed();
            QStringList aspects = aspectsStr.split(',', Qt::SkipEmptyParts);

            for (QString aspect : aspects) {
                aspect = aspect.trimmed();
                //QRegularExpression aspectRe("((?:North|South) Node|Pars Fortuna|Part of Spirit|East Point|\\w+)(?:\\s+\\(R\\))? (\\w+) ((?:North|South) Node|Pars Fortuna|Part of Spirit|East Point|\\w+)\\( ([\\d.]+)°\\)");

                QRegularExpression aspectRe("((?:North|South) Node|Pars Fortuna|Part of Spirit|East Point|\\w+)(?:\\s+\\(R\\))? (\\w+) ((?:North|South) Node|Pars Fortuna|Part of Spirit|East Point|\\w+(?:\\s+\\(R\\))?)\\( ([\\d.]+)°\\)");

                QRegularExpressionMatch aspectMatch = aspectRe.match(aspect);

                if (aspectMatch.hasMatch()) {
                    int row = rawTransitTable->rowCount();
                    rawTransitTable->insertRow(row);

                    QString transitPlanet = aspectMatch.captured(1);
                    //bool isRetrograde = aspect.contains("(R)");
                    bool isRetrograde = aspect.contains("(R)") &&
                            !transitPlanet.contains("Node");
                    QString natalPlanet = aspectMatch.captured(3);

                    if (transitPlanet == "North Node") transitPlanet = "NNode";
                    if (transitPlanet == "South Node") transitPlanet = "SNode";
                    if (transitPlanet == "Pars Fortuna") transitPlanet = "PFortuna";
                    if (transitPlanet == "Part of Spirit") transitPlanet = "PSpirit";
                    if (transitPlanet == "East Point") transitPlanet = "EPoint";

                    if (natalPlanet == "North Node") natalPlanet = "NNode";
                    if (natalPlanet == "South Node") natalPlanet = "SNode";
                    if (natalPlanet == "Pars Fortuna") natalPlanet = "PFortuna";
                    if (natalPlanet == "Part of Spirit") natalPlanet = "PSpirit";
                    if (natalPlanet == "East Point") natalPlanet = "EPoint";

                    rawTransitTable->setItem(row, 0, new QTableWidgetItem(currentDate.toString("yyyy-MM-dd")));
                    rawTransitTable->setItem(row, 1, new QTableWidgetItem(transitPlanet + (isRetrograde ? " (R)" : "")));
                    rawTransitTable->setItem(row, 2, new QTableWidgetItem(aspectMatch.captured(2)));
                    rawTransitTable->setItem(row, 3, new QTableWidgetItem(natalPlanet + " (" + aspectMatch.captured(4) + "°)"));
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
    QString chartTypeSanitized = GlobalFlags::lastGeneratedChartType.replace(" ", "-");
    QString baseName = QString("%1-%2-%3-%4-%5-chart").arg(chartTypeSanitized, name, surname, currentDate, currentTime);
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
        <h3>Getting AI Interpretations</h3>
        <p>Asteria uses the Mistral AI API to provide personalized astrological interpretations. To use this feature:</p>
        <ol>
            <li><b>Create a Mistral AI account:</b>
                <ul>
                    <li>Navigate to <a href="https://mistral.ai/">https://mistral.ai/</a></li>
                    <li>Click "Try the API" button</li>
                    <li>Sign up for an account</li>
                </ul>
            </li>
            <li><b>Get your API key:</b>
                <ul>
                    <li>Once logged in, go to the "API Keys" section</li>
                    <li>Create a new API key (keep this secure as it's linked to your account)</li>
                </ul>
            </li>
            <li><b>Configure Asteria to use your API key:</b>
                <ul>
                    <li>In Asteria, go to "Settings" in the menu bar</li>
                    <li>Select "Configure API Key"</li>
                    <li>Enter your Mistral AI API key in the provided field</li>
                    <li>Click "Save"</li>
                </ul>
            </li>
            <li><b>Using AI features:</b>
                <ul>
                    <li>After calculating a birth chart, click "Get Birth Chart From AI" to receive a detailed interpretation</li>
                    <li>For future predictions, set your desired date range and click "Get AI Prediction"</li>
                    <li>The AI will analyze the astrological data and provide personalized insights</li>
                </ul>
            </li>
        </ol>
        <p><b>Note:</b> The AI interpretation feature requires an internet connection. Mistral AI offers free tokens for API usage, and with moderate usage you will be fine within the free tier. Usage beyond their free tier may incur charges to your Mistral AI account.</p>
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
        {"SQQ", 135.0, orbMax * 0.75}, // Sesquiquadrate


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
    GlobalFlags::lastGeneratedChartType = "Composite Relationship";
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
    GlobalFlags::lastGeneratedChartType = "Davison Relationship";

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


void MainWindow::showChangelog(){

    // Create the dialog only if it doesn't exist yet
    if (!m_showChangelogDialog) {
        m_showChangelogDialog = new QDialog(this);
        m_showChangelogDialog->setWindowTitle("Changelog");
        m_showChangelogDialog->setMinimumSize(600, 500);

        // Create layout
        QVBoxLayout *layout = new QVBoxLayout(m_showChangelogDialog);

        // Create a text browser for rich text display
        QTextBrowser *textBrowser = new QTextBrowser(m_showChangelogDialog);
        textBrowser->setOpenExternalLinks(true);

        // Set the changelog content
        QString changelogText = R"(

<h1>Changelog</h1>

<h2>Version 2.1.1 (2025-09-23) <span style='color:#27ae60;'>&mdash; Major Update</span></h2>
<ul>
  <li><b>Stunning AI Interpretations:</b> Markdown AI responses are now transformed into rich HTML, producing beautifully formatted, clear, and engaging interpretations—like reading a professional digital publication.</li>
  <li><b>Smarter Chart Tagging:</b> All chart types (Natal, Relationship, Progression, Return, Zodiac Signs) now carry unique tags, so AI provides context-specific interpretations. <b>Exception:</b> Composite charts are excluded for accuracy.</li>
  <li><b>New Zodiac Sign Chart:</b> Generates magazine-style horoscopes for all 12 signs for any chosen date/time.</li>
  <li><b>Multi-Window Support:</b> Open multiple Asteria windows via File → New Window, either in the same instance or as new instances. Charts can also be opened in new windows for comparison.</li>
  <li><b>Drag-and-Drop Between Windows:</b> Use Ctrl+Left-Click to drag charts into another Asteria window (input or interpretation docks) to load all chart data instantly.</li>
  <li><b>Enhanced Viewing Options:</b> View menu checkboxes to toggle Chart-Only view or Info Overlay visibility (hidden by default).</li>
  <li><b>Clear Interpretation Button:</b> Clears both displayed interpretation and stored previous readings in one click.</li>
  <li><b>Chart Zoom in/out Functionality:</b> Use CTRL+mouse wheel to zoom in and out of generated chart.</li>
</ul>

<h2>Version 2.1.0 (2025-07-14) <span style='color:#27ae60;'>&mdash; Major Release</span></h2>
<ul>
  <li><b>Rendering System Overhaul:</b> Redesigned the chart rendering engine—now all chart elements are drawn in an <b>anticlockwise</b> direction for improved astrological convention and clarity.</li>
  <li><b>Return Charts for All Major Planets:</b> Added support for calculating and displaying return charts for all basic planets (Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto).</li>
  <li><b>Secondary Progression Charts:</b> Introduced secondary progression chart calculation and display.</li>
  <li><b>Eclipse Calculations:</b> Added functionality to calculate both lunar and solar eclipses.</li>
  <li><b>Transits Calculation Enhancements:</b>
    <ul>
      <li>Added a new method and UI button to calculate planetary transits for any chart over a period of up to one year.</li>
      <li>Introduced an advanced transit search dialog, allowing users to filter transits by date, transiting planet, aspect, natal planet, orb, and to apply exclusion filters for precise results.</li>
    </ul>
  </li>
  <li><b>Julian/Gregorian Calendar Toggle:</b> Implemented a settings menu checkbox to toggle between Julian and Gregorian calendars for dates prior to 1582, ensuring historical accuracy.</li>
  <li><b>Chart Type Flag for Transits:</b> Each calculated transit now includes a chart type flag, which is also passed to the AI interpretation prompt for more targeted and context-aware readings.</li>
  <li><b>Date Range Expansion:</b> Extended the allowed date span for chart and transit calculations to 0001–3000 CE (excluding BCE years due to QDate limitations).</li>
  <li><b>Export Chart Data:</b> Added an option to export all chart data as text, saving the contents of every table in the "Chart Details" tab for easy sharing and analysis.</li>
  <li><b>Angles Table:</b> Added a new Angles table displaying the values of ASC (Ascendant), DESC (Descendant), IC (Imum Coeli), and MC (Medium Coeli).</li>
  <li><b>SQQ Aspect:</b> Added the SQQ (Sesquiquadrate, 135°) aspect to aspect calculations and filtering.</li>
  <li><b>General Improvements & Bug Fixes:</b> Various minor UI polishes, bug fixes, and code optimizations for a smoother user experience.</li>
</ul>

<h2>Version 2.0.1 (2025-06-06)</h2>
<h3>Improvements</h3>
<ul>
<li>Changed degree display format from 0-360° decimal system to traditional in-sign degrees with minutes for improved astrological readability (e.g., 285.5° → 15°30' Capricorn)</li>
<li>House display now shows both formats for reference</li>
</ul>
<p><b style="color: #d35400;">Important Note:</b> Charts calculated and saved with the old degree format will not display correctly with this update. To fix this, please reload your saved charts, press the "Calculate Chart" button again, and save them with the new format.</p>

<h2>Version 2.0.0 (2025-05-15)</h2>
<h3>Major Improvements</h3>
<ul>
<li>Completely rebuilt the calculation engine: removed Python dependency, virtual environment, flatlib, and all related scripts</li>
<li>Now using direct Swiss Ephemeris integration through native Qt/C++ code for significantly improved performance and reduced complexity</li>
</ul>
<h3>New Features</h3>
<ul>
<li>Added an orb slider to set how many aspects are calculated and displayed</li>
<li>Added additional celestial bodies to be calculated and displayed:
  <ul>
    <li>Black Moon Lilith</li>
    <li>Ceres</li>
    <li>Pallas</li>
    <li>Juno</li>
    <li>Vesta</li>
    <li>Vertex</li>
    <li>East Point</li>
    <li>Part of Spirit</li>
  </ul>
</li>
<li>Added a checkbox to toggle displaying additional bodies in the chart</li>
<li>Added Aspect Display Settings Dialog where users can customize the thickness and style of major and minor aspects</li>
<li>Added relationship charts functionality with Composite and Davison charts (synastry to be implemented in a future release)</li>
</ul>
<h3>Performance Improvements</h3>
<ul>
<li>Preloaded OpenStreetMap QML at application startup to eliminate pause when opening the map dialog</li>
</ul>
<h3>Changes</h3>
<ul>
<li>Updated license to AGPL-3 to conform with Swiss Ephemeris requirements (see credits.txt for full attribution)</li>
</ul>

<h2>Version 1.1.0 (2025-04-30)</h2>
<h3>New Features</h3>
<ul>
<li>Added <a href="https://www.openstreetmap.org/">OpenStreetMap</a> dialog to assist users in easily searching and setting their birth location</li>
<li>Expanded retrograde planet depiction beyond the planets drawn red on the chart itself and the tooltips, to also include the planet widgets, planet view, and aspects view</li>
</ul>
<h3>Improvements</h3>
<ul>
<li>Made minor UI improvements for better usability</li>
<li>Added multiple screenshots to better showcase the application's features</li>
<li>Edited application summary to conform with Flathub quality guidelines</li>
</ul>

<h2>Version 1.0.0 (2025-04-22)</h2>
<h3>Initial Release</h3>
<ul>
<li>Natal chart calculation and display</li>
<li>Aspect analysis</li>
<li>AI-powered interpretations</li>
<li>Transit calculations</li>
</ul>
)";

        textBrowser->setHtml(changelogText);

        layout->addWidget(textBrowser);

        // Add a close button at the bottom
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_showChangelogDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // Connect the close button
        connect(closeButton, &QPushButton::clicked, m_showChangelogDialog, &QDialog::close);

        // Connect the dialog's finished signal to handle cleanup
        connect(m_showChangelogDialog, &QDialog::finished, this, [this]() {
            m_showChangelogDialog->deleteLater();
            m_showChangelogDialog = nullptr;
        });
    }

    // Show and raise the dialog to bring it to the front
    m_showChangelogDialog->show();
    m_showChangelogDialog->raise();
    m_showChangelogDialog->activateWindow();
}



void MainWindow::CalculateTransits() {

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


    // Gregorian calendar reform date
    QDate gregorianStart(1582, 10, 15);

    // Validate all dates are post-Gregorian
    if (birthDate < gregorianStart || fromDate < gregorianStart || toDate < gregorianStart) {
        QMessageBox::warning(
                    this,
                    tr("Unsupported Date"),
                    tr("All dates (birth, start, and end) must be after 15 October 1582 (Gregorian calendar reform) for transit calculations.\n"
                       "Please enter valid post-Gregorian dates.")
                    );
        return;
    }

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

    if (transitDays <= 0 || transitDays > 370) {
        QMessageBox::warning(this, "Input Error", "Prediction period must be between 1 and 370 days.");
        return;
    }

    if (transitDays > 60) {
        QMessageBox::information(
                    this,
                    "Please Be Patient",
                    "This operation may take some time.\n"

                    " When finished you will be notified."
                    );
    }


    // Update status
    statusBar()->showMessage(QString("Calculating transits for %1 to %2...")
                             .arg(fromDate.toString("yyyy-MM-dd"))
                             .arg(toDate.toString("yyyy-MM-dd")));

    // Calculate transits
    this->setEnabled(false); // Disable all widgets in the main window

    QJsonObject transitData = m_chartDataManager.calculateTransitsAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, fromDate, transitDays);

    this->setEnabled(true); // Enable all widgets in the main window


    if (m_chartDataManager.getLastError().isEmpty()) {
        //populate tab
        displayRawTransitData(transitData);
        QMessageBox::information(this, "Transit Data", "Transit data has been generated successfully.\n"
                                                       "Please Navigate to the 'Raw Transit Data Table' to view the data.");


    } else {
        handleError("Transit calculation error: " + m_chartDataManager.getLastError());

    }

}


void MainWindow::applyTransitFilter(const QString &datePattern,
                                    const QString &transitPattern,
                                    const QString &aspectPattern,
                                    const QString &natalPattern,
                                    const QString &maxOrbPattern,

                                    const QString &excludePattern)
{
    // Show "Applying filter..." before starting
    if (m_transitSearchDialog && m_transitSearchDialog->statusLabel) {
        m_transitSearchDialog->statusLabel->setText("Please wait...");
        qApp->processEvents();
    }
    // Save current state
    m_savedScrollPosition = rawTransitTable->verticalScrollBar()->value();
    m_savedSelection = rawTransitTable->selectionModel()->selection();

    int matchCount = 0;

    for(int row = 0; row < rawTransitTable->rowCount(); ++row) {
        bool match = true;

        // Apply include filters
        if(!datePattern.isEmpty()) {
            match &= rawTransitTable->item(row, 0)->text().contains(QRegularExpression(datePattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !transitPattern.isEmpty()) {
            match &= rawTransitTable->item(row, 1)->text().contains(QRegularExpression(transitPattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !aspectPattern.isEmpty()) {
            match &= rawTransitTable->item(row, 2)->text().contains(QRegularExpression(aspectPattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !natalPattern.isEmpty()) {
            match &= rawTransitTable->item(row, 3)->text().contains(QRegularExpression(natalPattern, QRegularExpression::CaseInsensitiveOption));
        }
        // Orb Filter
        if (match && !maxOrbPattern.isEmpty()) {
            bool ok = false;
            double maxOrb = maxOrbPattern.toDouble(&ok);
            if (ok) {
                // Extract orb from natal planet column (column 3)
                QString natalText = rawTransitTable->item(row, 3)->text();
                QRegularExpression orbRegex("\\((\\d+(?:\\.\\d+)?)°\\)");
                QRegularExpressionMatch orbMatch = orbRegex.match(natalText);
                if (orbMatch.hasMatch()) {
                    double orbValue = orbMatch.captured(1).toDouble();
                    if (orbValue > maxOrb) {
                        match = false;
                    }
                }
                // If no orb found, you may want to hide or show by default:
                // else { match = false; } // Uncomment to hide rows without orb info
            }
        }


        //
        // Apply exclude filter
        if(match && !excludePattern.isEmpty()) {
            QStringList excludeTerms = excludePattern.split(',', Qt::SkipEmptyParts);
            for(const QString &term : excludeTerms) {
                QString trimmedTerm = term.trimmed();
                if(!trimmedTerm.isEmpty()) {
                    // Check if any column contains the exclude term
                    bool containsExcludeTerm =
                            rawTransitTable->item(row, 0)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            rawTransitTable->item(row, 1)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            rawTransitTable->item(row, 2)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            rawTransitTable->item(row, 3)->text().contains(trimmedTerm, Qt::CaseInsensitive);

                    if(containsExcludeTerm) {
                        match = false;
                        break; // No need to check other exclude terms
                    }
                }
            }
        }

        rawTransitTable->setRowHidden(row, !match);
        if(match) matchCount++;
    }
    // Show "Filter applied" after finishing
    if (m_transitSearchDialog && m_transitSearchDialog->statusLabel)
        m_transitSearchDialog->statusLabel->setText("Filter applied");
}




void MainWindow::openTransitFilter() {
    if (!m_transitSearchDialog) {
        m_transitSearchDialog = new TransitSearchDialog(this);
        connect(m_transitSearchDialog, &TransitSearchDialog::filterChanged,
                this, &MainWindow::applyTransitFilter);
    }
    m_transitSearchDialog->show();
}

void MainWindow::exportChartData(){
    // Check if we have chart data by looking for the details tab widget
    if (!m_chartDetailsWidget) {
        QMessageBox::warning(this, "No Chart Data", "Please generate a chart first.");
        return;
    }

    // Find the details tabs widget
    QTabWidget *detailsTabs = m_chartDetailsWidget->findChild<QTabWidget*>();
    if (!detailsTabs) {
        QMessageBox::warning(this, "No Chart Data", "Chart details not available.");
        return;
    }

    QString filePath = getFilepath("txt");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        // Write header
        stream << "ASTROLOGICAL CHART DATA\n";
        stream << "=======================\n\n";

        // Iterate through each tab
        for (int tabIndex = 0; tabIndex < detailsTabs->count(); ++tabIndex) {
            QWidget *tabWidget = detailsTabs->widget(tabIndex);
            QString tabName = detailsTabs->tabText(tabIndex).toUpper();
            QTableWidget *table = qobject_cast<QTableWidget*>(tabWidget);

            if (!table || table->rowCount() == 0) {
                continue; // Skip empty tables
            }

            // Write tab section header
            stream << tabName << "\n";
            stream << QString("-").repeated(tabName.length()) << "\n";

            // Write column headers
            QStringList headers;
            for (int col = 0; col < table->columnCount(); ++col) {
                headers << table->horizontalHeaderItem(col)->text();
            }

            // Calculate column widths for alignment
            QList<int> columnWidths;
            for (int col = 0; col < table->columnCount(); ++col) {
                int maxWidth = headers[col].length();
                for (int row = 0; row < table->rowCount(); ++row) {
                    QTableWidgetItem *item = table->item(row, col);
                    if (item) {
                        maxWidth = qMax(maxWidth, item->text().length());
                    }
                }
                columnWidths << qMax(maxWidth + 2, 8); // Minimum width of 8
            }

            // Write headers with proper spacing
            for (int col = 0; col < headers.size(); ++col) {
                stream << headers[col].leftJustified(columnWidths[col]);
            }
            stream << "\n";

            // Write header underlines
            for (int col = 0; col < headers.size(); ++col) {
                stream << QString("-").repeated(headers[col].length()).leftJustified(columnWidths[col]);
            }
            stream << "\n";

            // Write table data
            for (int row = 0; row < table->rowCount(); ++row) {
                for (int col = 0; col < table->columnCount(); ++col) {
                    QTableWidgetItem *item = table->item(row, col);
                    QString cellText = item ? item->text() : "";
                    stream << cellText.leftJustified(columnWidths[col]);
                }
                stream << "\n";
            }

            stream << "\n"; // Add spacing between sections
        }

        file.close();
        statusBar()->showMessage("Chart data exported to " + filePath, 3000);
    } else {
        QMessageBox::critical(this, "Export Error", "Could not save chart data to " + filePath);
    }
}

void MainWindow::CalculateEclipses()
{
    // Optional: Only proceed if a chart is calculated
    //if (!m_chartCalculated) {
    //  QMessageBox::warning(this, "No Chart", "Please calculate a birth chart first.");
    //return;
    //}

    // Use the same date fields as for transits
    QDate fromDate = QDate::fromString(m_predictiveFromEdit->text(), "dd/MM/yyyy");
    QDate toDate = QDate::fromString(m_predictiveToEdit->text(), "dd/MM/yyyy");

    // Validate dates
    if (!fromDate.isValid() || !toDate.isValid()) {
        QMessageBox::warning(this, "Input Error", "Please enter valid dates for eclipse search range.");
        return;
    }
    if (fromDate > toDate) {
        QMessageBox::warning(this, "Input Error", "From date must be before To date.");
        return;
    }

    int days = fromDate.daysTo(toDate) + 1;
    if (days <= 0 || days > 365 * 100) {
        QMessageBox::warning(this, "Input Error", "Eclipse search period must be between 1 and 36500 days.");
        return;
    }

    // Example: Assume you have checkboxes for eclipse types
    //bool solarEclipses = true;
    //bool lunarEclipses = true;

    if (!solarEclipses && !lunarEclipses) {
        QMessageBox::warning(this, "Input Error", "Please select at least one eclipse type (solar or lunar).");
        return;
    }

    statusBar()->showMessage(QString("Calculating eclipses for %1 to %2...")
                             .arg(fromDate.toString("yyyy-MM-dd"))
                             .arg(toDate.toString("yyyy-MM-dd")));

    QJsonArray eclipseData = m_chartDataManager.calculateEclipsesAsJson(
                fromDate, toDate, solarEclipses, lunarEclipses);

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayRawEclipseData(eclipseData);
        QMessageBox::information(this, "Eclipse Data", "Eclipse data has been generated successfully.\n"
                                                       "Please navigate to 'Chart Details->Eclipses' tab tp view the data.");
    } else {
        handleError("Eclipse calculation error: " + m_chartDataManager.getLastError());
    }
}

void MainWindow::displayRawEclipseData(const QJsonArray &eclipseData)
{
    // Find the eclipse table by object name (set when creating the table)
    QTableWidget *eclipseTable = findChild<QTableWidget*>("Eclipses");
    if (!eclipseTable) return;

    eclipseTable->setRowCount(0); // Clear previous data

    for (const QJsonValue &val : eclipseData) {
        QJsonObject obj = val.toObject();
        int row = eclipseTable->rowCount();
        eclipseTable->insertRow(row);

        // Extract and set each field
        QString date = obj.value("date").toString();
        QString time = obj.value("time").toString();
        QString type = obj.value("type").toString();
        double magnitude = obj.value("magnitude").toDouble();
        double latitude = obj.value("latitude").toDouble();
        double longitude = obj.value("longitude").toDouble();

        eclipseTable->setItem(row, 0, new QTableWidgetItem(date));
        eclipseTable->setItem(row, 1, new QTableWidgetItem(time));
        eclipseTable->setItem(row, 2, new QTableWidgetItem(type));
        eclipseTable->setItem(row, 3, new QTableWidgetItem(QString::number(magnitude, 'f', 2)));
        eclipseTable->setItem(row, 4, new QTableWidgetItem(QString::number(latitude, 'f', 2)));
        eclipseTable->setItem(row, 5, new QTableWidgetItem(QString::number(longitude, 'f', 2)));
    }

    // Optional: sort by date column
    eclipseTable->sortItems(0);
}


void MainWindow::calculateSolarReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Prompt user for the solar return year
    bool ok = false;
    int year = QInputDialog::getInt(
                this,
                tr("Solar Return Year"),
                tr("Enter the year for the solar return:"),
                QDate::currentDate().year(), // default value
                1900, 2999, 1, &ok
                );
    if (!ok) {
        // User cancelled the dialog
        return;
    }

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doSolarReturnCalculation(birthDate, birthTime, year);
    });

    dialog->show();
}

void MainWindow::doSolarReturnCalculation(const QDate& birthDate, const QTime& birthTime, int year)
{

    // Get the (possibly updated) input values
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

    // Calculate solar return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateSolarReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, year
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;

        GlobalFlags::lastGeneratedChartType = "Solar Return";


        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        // Clear previous interpretation
        //m_currentInterpretation.clear();
        //m_interpretationtextEdit->clear();
        //m_interpretationtextEdit->setPlaceholderText("Click 'Get AI Interpretation' to analyze this chart.");

        statusBar()->showMessage("Solar return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Solar Return Chart");


        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr   = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Solar Return Year: %1\n"
                    "Solar Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(QString::number(year), dateStr, timeStr, jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Solar return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Solar Return Moment", msg);

    } else {
        handleError("Solar return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        // Clear any partial chart data after error
        m_chartRenderer->scene()->clear();
    }
}

void MainWindow::calculateLunarReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Prompt user for the lunar return year
    bool ok = false;
    int year = QInputDialog::getInt(
                this,
                tr("Lunar Return Year"),
                tr("Enter the year for the lunar return:"),
                QDate::currentDate().year(),
                1900, 2100, 1, &ok
                );
    if (!ok) {
        // User cancelled the dialog
        return;
    }

    // Prompt user for the lunar return month
    int month = QInputDialog::getInt(
                this,
                tr("Lunar Return Month"),
                tr("Enter the month for the lunar return (1-12):"),
                QDate::currentDate().month(),
                1, 12, 1, &ok
                );
    if (!ok) {
        // User cancelled the dialog
        return;
    }

    // Use the first day of the selected month as the target date
    QDate targetDate(year, month, 1);

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doLunarReturnCalculation(birthDate, birthTime, targetDate);
    });

    dialog->show();
}

void MainWindow::doLunarReturnCalculation(const QDate& birthDate, const QTime& birthTime, const QDate& targetDate)
{
    // Get the (possibly updated) input values
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

    // Calculate lunar return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateLunarReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, targetDate
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        // Set chart type for interpretation
        GlobalFlags::lastGeneratedChartType = "Lunar Return";

        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        // Clear previous interpretation
        //m_currentInterpretation.clear();
        //m_interpretationtextEdit->clear();
        //m_interpretationtextEdit->setPlaceholderText("Click 'Get AI Interpretation' to analyze this chart.");

        statusBar()->showMessage("Lunar return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Lunar Return Chart");



        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr   = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Lunar Return Moment\n"
                    "Date: %1\n"
                    "Time: %2\n"
                    "Julian Day: %3\n\n"
                    ).arg(dateStr, timeStr, jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Lunar return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Lunar Return Moment", msg);

    } else {
        handleError("Lunar return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        // Clear any partial chart data after error
        m_chartRenderer->scene()->clear();
    }
}


void MainWindow::calculateSaturnReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Calculate approximate current Saturn return
    double saturnPeriod = 29.4571; // Saturn orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / saturnPeriod) + 1;

    // Prompt user for which Saturn return
    bool ok = false;
    QString prompt = tr("Enter which Saturn return (1 = first, 2 = second, etc.):\n"
                        "Saturn orbital period: %1 years\n"
                        "You are currently in your approx. %2 Saturn return.")

            .arg(QString::number(saturnPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Saturn Return Number"),
                prompt,
                approxReturn, 1, 200, 1, &ok // Allow up to 5 Saturn returns
                );

    if (!ok) return;

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doSaturnReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}



void MainWindow::doSaturnReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    // Get the (possibly updated) input values
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
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();



    // Calculate Saturn return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());


    m_currentChartData = m_chartDataManager.calculateSaturnReturnAsJson(
                chartDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Saturn Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        // Clear previous interpretation
        // m_currentInterpretation.clear();
        // m_interpretationtextEdit->clear();
        // m_interpretationtextEdit->setPlaceholderText("Click 'Get AI Interpretation' to analyze this chart.");

        statusBar()->showMessage("Saturn return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Saturn Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Saturn Return %1\n"
                    "Saturn Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Saturn return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Saturn Return Moment", msg);

    } else {
        handleError("Saturn return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

void MainWindow::calculateJupiterReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Calculate approximate current Jupiter return
    double jupiterPeriod = 11.862; // Jupiter orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / jupiterPeriod) + 1;

    // Prompt user for which Jupiter return
    bool ok = false;
    QString prompt = tr("Enter which Jupiter return (1 = first, 2 = second, etc.):\n"
                        "Jupiter orbital period: %1 years\n"
                        "You are currently in your approx. %2 Jupiter return.")
            .arg(QString::number(jupiterPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Jupiter Return Number"),
                prompt,
                approxReturn, 1, 300, 1, &ok // Allow up to 20 returns for flexibility
                );

    if (!ok) return;

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doJupiterReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}




void MainWindow::doJupiterReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    // Get the (possibly updated) input values
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
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    // Calculate Jupiter return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateJupiterReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Jupiter Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Jupiter return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Jupiter Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Jupiter Return %1\n"
                    "Jupiter Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"

                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);
        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Jupiter return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Jupiter Return Moment", msg);

    } else {
        handleError("Jupiter return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

// more planet returns

void MainWindow::calculateVenusReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Calculate approximate current Venus return
    double venusPeriod = 0.61519726; // Venus orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / venusPeriod) + 1;

    // Prompt user for which Venus return
    bool ok = false;
    QString prompt = tr("Enter which Venus return (1 = first, 2 = second, etc.):\n"
                        "Venus orbital period: %1 years\n"
                        "You are currently in your approx. %2 Venus return.")
            .arg(QString::number(venusPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Venus Return Number"),
                prompt,
                approxReturn, 1, 1000, 1, &ok
                );
    if (!ok) return;

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doVenusReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}

void MainWindow::doVenusReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    // Get the (possibly updated) input values
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
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    // Calculate Venus return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateVenusReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Venus Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);
        statusBar()->showMessage("Venus return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Venus Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Venus Return %1\n"
                    "Venus Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Venus return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Venus Return Moment", msg);

    } else {
        handleError("Venus return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

/* --- MARS RETURN --- */
void MainWindow::calculateMarsReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Calculate approximate current Mars return
    double marsPeriod = 1.8808476; // Mars orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / marsPeriod) + 1;

    // Prompt user for which Mars return
    bool ok = false;
    QString prompt = tr("Enter which Mars return (1 = first, 2 = second, etc.):\n"
                        "Mars orbital period: %1 years\n"
                        "You are currently in your approx. %2 Mars return.")
            .arg(QString::number(marsPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Mars Return Number"),
                prompt,
                approxReturn, 1, 1000, 1, &ok
                );
    if (!ok) return;

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doMarsReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}

void MainWindow::doMarsReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    // Get the (possibly updated) input values
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
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    // Calculate Mars return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateMarsReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Mars Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);
        statusBar()->showMessage("Mars return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Mars Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Mars Return %1\n"
                    "Mars Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Mars return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Mars Return Moment", msg);

    } else {
        handleError("Mars return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

/* --- MERCURY RETURN --- */
void MainWindow::calculateMercuryReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    // Get input values
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    // Calculate approximate current Mercury return
    double mercuryPeriod = 0.2408467; // Mercury orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / mercuryPeriod) + 1;

    // Prompt user for which Mercury return
    bool ok = false;
    QString prompt = tr("Enter which Mercury return (1 = first, 2 = second, etc.):\n"
                        "Mercury orbital period: %1 years\n"
                        "You are currently in your approx. %2 Mercury return.")
            .arg(QString::number(mercuryPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Mercury Return Number"),
                prompt,
                approxReturn, 1, 1000, 1, &ok
                );
    if (!ok) return;

    // Create a non-modal dialog with a Continue button
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    // Connect the continue button
    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doMercuryReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}

void MainWindow::doMercuryReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    // Get the (possibly updated) input values
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
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    // Calculate Mercury return chart
    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateMercuryReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Mercury Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);
        statusBar()->showMessage("Mercury return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Mercury Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Mercury Return %1\n"
                    "Mercury Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Mercury return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Mercury Return Moment", msg);

    } else {
        handleError("Mercury return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}


bool MainWindow::validateDateFormat(const QString& dateText, QWidget* parentWidget)
{
    QRegularExpressionMatch match = dateRegex.match(dateText);
    if (!match.hasMatch()) {
        QMessageBox::warning(parentWidget ? parentWidget : this, tr("Input Error"),
                             tr("Please enter the date in DD/MM/YYYY format with a four-digit year (0001–3000)."));
        return false;
    }

    // Now, check if the date is a real calendar date
    QDate date = QDate::fromString(dateText, "dd/MM/yyyy");
    if (!date.isValid()) {
        QMessageBox::warning(parentWidget ? parentWidget : this, tr("Input Error"),
                             tr("The date you entered does not exist in the calendar.\n\n"
                                "Examples of invalid dates:\n"
                                "  - 30/02/2023 (February never has 30 days)\n"
                                "  - 31/04/2022 (April has only 30 days)\n"
                                "  - 29/02/2023 (2023 is not a leap year)\n"
                                "  - 32/01/2020 (No month has more than 31 days)\n"
                                "  - 15/13/2020 (There is no 13th month)\n\n"
                                "Please check your entry and try again."));
        return false;
    }
    return true;
}

QDate MainWindow::julianToGregorian(int year, int month, int day) const
{
    // Calculate Julian Day Number for Julian calendar date
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    int julianDay = day + ((153 * m + 2) / 5) + 365 * y + y / 4 - 32083;

    // Now convert that JDN to Gregorian date using QDate
    QDate gregorianDate = QDate::fromJulianDay(julianDay);
    return gregorianDate;
}

QDate MainWindow::checkAndConvertJulian(const QDate& date, bool useJulian) const
{
    QDate gregorianStart(1582, 10, 15);
    if (useJulian && date.isValid() && date < gregorianStart) {
        QDate converted = julianToGregorian(date.year(), date.month(), date.day());
        qDebug() << "Julian input:" << date.toString(Qt::ISODate)
                 << "-> Gregorian:" << converted.toString(Qt::ISODate);
        return converted;
    }
    return date;
}

// Uranus Neptune Pluto Returns
void MainWindow::calculateUranusReturn()
{
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double uranusPeriod = 84.016846; // Uranus orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / uranusPeriod) + 1;

    bool ok = false;
    QString prompt = tr("Enter which Uranus return (1 = first, 2 = second, etc.):\n"
                        "Uranus orbital period: %1 years\n"
                        "You are currently in your approx. %2 Uranus return.")
            .arg(QString::number(uranusPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Uranus Return Number"),
                prompt,
                approxReturn, 1, 200, 1, &ok
                );
    if (!ok) return;

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doUranusReturnCalculation(birthDate, birthTime, returnNumber);
    });
    dialog->show();
}

void MainWindow::doUranusReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();
    QString houseSystem = m_houseSystemCombo->currentText();

    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());
    m_currentChartData = m_chartDataManager.calculateUranusReturnAsJson(
                chartDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Uranus Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Uranus return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Uranus Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Uranus Return %1\n"
                    "Uranus Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Uranus return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Uranus Return Moment", msg);

    } else {
        handleError("Uranus return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

// Repeat for Neptune

void MainWindow::calculateNeptuneReturn()
{
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double neptunePeriod = 164.79132; // Neptune orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / neptunePeriod) + 1;

    bool ok = false;
    QString prompt = tr("Enter which Neptune return (1 = first, 2 = second, etc.):\n"
                        "Neptune orbital period: %1 years\n"
                        "You are currently in your approx. %2 Neptune return.")
            .arg(QString::number(neptunePeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Neptune Return Number"),
                prompt,
                approxReturn, 1, 200, 1, &ok
                );
    if (!ok) return;

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doNeptuneReturnCalculation(birthDate, birthTime, returnNumber);
    });
    dialog->show();
}

void MainWindow::doNeptuneReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();
    QString houseSystem = m_houseSystemCombo->currentText();

    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());
    m_currentChartData = m_chartDataManager.calculateNeptuneReturnAsJson(
                chartDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Neptune Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Neptune return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Neptune Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Neptune Return %1\n"
                    "Neptune Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Neptune return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Neptune Return Moment", msg);

    } else {
        handleError("Neptune return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

// Repeat for Pluto

void MainWindow::calculatePlutoReturn()
{
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double plutoPeriod = 248.00; // Pluto orbital period in years (approximate)
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / plutoPeriod) + 1;

    bool ok = false;
    QString prompt = tr("Enter which Pluto return (1 = first, 2 = second, etc.):\n"
                        "Pluto orbital period: %1 years\n"
                        "You are currently in your approx. %2 Pluto return.")
            .arg(QString::number(plutoPeriod, 'f', 3))
            .arg(approxReturn);

    int returnNumber = QInputDialog::getInt(
                this,
                tr("Pluto Return Number"),
                prompt,
                approxReturn, 1, 200, 1, &ok
                );
    if (!ok) return;

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doPlutoReturnCalculation(birthDate, birthTime, returnNumber);
    });
    dialog->show();
}

void MainWindow::doPlutoReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
{
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();
    QString houseSystem = m_houseSystemCombo->currentText();

    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());
    m_currentChartData = m_chartDataManager.calculatePlutoReturnAsJson(
                chartDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Pluto Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Pluto return chart calculated successfully", 3000);
        setWindowTitle("Asteria - Pluto Return Chart");

        QString dateStr = m_currentChartData.value("returnDate").toString();
        QString timeStr = m_currentChartData.value("returnTime").toString();
        QString jdStr = m_currentChartData.value("returnJulianDay").toString();

        QString infoText = QString(
                    "Pluto Return %1\n"
                    "Pluto Return Moment\n"
                    "Date: %2\n"
                    "Time: %3\n"
                    "Julian Day: %4\n\n"
                    ).arg(returnNumber).arg(dateStr).arg(timeStr).arg(jdStr);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

        QString msg = QString("Pluto return occurs on %1 at %2 (Julian Day: %3)")
                .arg(dateStr)
                .arg(timeStr)
                .arg(jdStr);

        QMessageBox::information(this, "Pluto Return Moment", msg);

    } else {
        handleError("Pluto return calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

// Secondary Progression Chart

void MainWindow::calculateSecondaryProgression()
{
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }

    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    int yearsSinceBirth = static_cast<int>(daysSinceBirth / 365.25);

    bool ok = false;
    QString prompt = tr("Enter the progression year (1 = first year after birth, etc.):\n"
                        "You are currently approximately %1 years old.")
            .arg(yearsSinceBirth);

    int progressionYear = QInputDialog::getInt(
                this,
                tr("Secondary Progression Year"),
                prompt,
                qMax(1, yearsSinceBirth), 1, 120, 1, &ok
                );
    if (!ok) return;

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Location and UTC Offset"));
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* label = new QLabel(tr(
                                   "If you want to set a different location, set it now with the proper UTC offset; "
                                   "otherwise, the birth location will be used.\n\n"
                                   "Click 'Continue' when ready."
                                   ), dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    QPushButton* continueBtn = new QPushButton(tr("Continue"), dialog);
    layout->addWidget(continueBtn);

    connect(continueBtn, &QPushButton::clicked, this, [=]() {
        dialog->close();
        doSecondaryProgressionCalculation(progressionYear);
    });
    dialog->show();
}

// Helper: Calculate and display the secondary progression chart
void MainWindow::doSecondaryProgressionCalculation(int progressionYear)
{
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }

    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();
    QString houseSystem = m_houseSystemCombo->currentText();

    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    // Calculate the progressed date: birth date + progressionYear years
    QDate progressedDate = birthDate.addYears(progressionYear);

    // Reset chart state before new calculation
    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject();
    m_chartRenderer->scene()->clear();

    progressedDate = checkAndConvertJulian(progressedDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                progressedDate, birthTime, utcOffset, latitude, longitude, houseSystem
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Secondary Progression";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        m_currentInterpretation.clear();
        m_interpretationtextEdit->clear();
        m_interpretationtextEdit->setPlaceholderText("Click 'Get AI Interpretation' to analyze this chart.");

        statusBar()->showMessage("Secondary progression chart calculated successfully", 3000);

        QString dateStr = m_currentChartData.value("date").toString();
        QString timeStr = m_currentChartData.value("time").toString();
        QString infoText = QString(
                    "Secondary Progression Chart\n"
                    "Progressed Date: %1\n"
                    "Time: %2\n"
                    "Progression Year: %3\n\n"
                    ).arg(dateStr, timeStr).arg(progressionYear);

        m_interpretationtextEdit->append(infoText);
        m_currentInterpretation.append(infoText);

    } else {
        handleError("Secondary progression calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}

void MainWindow::showNewFeaturesDialog() {
    // Create the dialog only if it doesn't exist yet
    if (!m_showNewFeaturesDialog) {
        m_showNewFeaturesDialog = new QDialog(this);
        m_showNewFeaturesDialog->setWindowTitle("What's New!");
        m_showNewFeaturesDialog->setMinimumSize(650, 600);

        // Create layout
        QVBoxLayout *layout = new QVBoxLayout(m_showNewFeaturesDialog);

        // Create a text browser for rich text display
        QTextBrowser *textBrowser = new QTextBrowser(m_showNewFeaturesDialog);
        textBrowser->setOpenExternalLinks(true);

        // Set the new features content
        QString featuresText = R"(

<h1 style="color:#27ae60;">What’s New in Version 2.1.1</h1>
<p>
Welcome to the 2.1.1 update! This release refines AI interpretations, adds new chart tagging, and introduces the Zodiac Sign Chart, making your astrological analysis more powerful and visually appealing than ever.
</p>

<h2>Enhanced AI Interpretations</h2>
<p>
All AI-generated markdown responses are now transformed into rich HTML, producing beautifully formatted, clear, and engaging interpretations—like reading a professional digital publication.
</p>

<h2>Chart Type Tagging</h2>
<p>
All charts (Natal, Relationship, Progression, Return, Zodiac Signs) now carry unique tags, allowing AI to interpret them accurately. <b>Exception:</b> Composite charts are excluded for correctness.
</p>

<h2>Zodiac Sign Chart</h2>
<p>
Generates magazine-style horoscopes for all 12 signs for any chosen date/time, providing concise but detailed narratives with practical advice.
</p>

<h2>Multi-Window & Drag-and-Drop</h2>
<ul>
  <li>Open multiple Asteria windows from File → New Window, either in the same instance or as a new instance.</li>
  <li>Drag charts between windows using Ctrl + Left Mouse Button to load all data into the target window’s input or interpretation docks.</li>
</ul>

<h2>View & Dock Enhancements</h2>
<ul>
  <li>View menu checkboxes: toggle Chart-Only view or Info Overlay visibility (hidden by default).</li>
  <li>Clear Interpretation button: clears both displayed text and stored previous interpretations.</li>
  <li>Chart Zoom in/out functionality via CTRL+mouse wheel.</li>
</ul>

<h1 style="color:#27ae60;">What’s New in Version 2.1.0</h1>
<p>
Welcome to the major <b>2.1.0</b> release of our astrology application! This update brings a wealth of new features, enhancements, and refinements designed to make your astrological work more powerful, flexible, and insightful than ever before.
</p>

<h2>Chart Rendering Overhaul</h2>
<p>
One of the most significant changes is a complete overhaul of the chart rendering system. All chart elements are now drawn in an <b>anticlockwise</b> direction, aligning with traditional astrological conventions and providing a more intuitive visual experience. We’d love to hear your feedback on this new approach!
</p>

<h2>Expanded Chart Types</h2>
<ul>
  <li><b>Natal Charts:</b> Your foundational birth chart, calculated for your exact birth details.</li>
  <li><b>(NEW)Return Charts for All Major Planets:</b> Generate return charts for the Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, and Pluto—offering deeper insights into planetary cycles.</li>
  <li><b>(NEW)Secondary Progression Charts:</b> Explore the symbolic evolution of your natal chart over time with secondary progression charts.</li>
  <li><b>Composite &amp; Davison Charts:</b> Analyze relationship dynamics from multiple perspectives with these advanced chart types.</li>
</ul>

<h2>Predictive Astrology Enhancements</h2>
<ul>
  <li><b>Eclipse Calculations:</b> Easily compute both lunar and solar eclipses. Set your desired period in the <i>Predictive Astrology</i> group box and navigate to the <b>Details</b> tab’s <b>Eclipses</b> section to view upcoming eclipse events relevant to your chart.</li>
  <li><b>Transits Calculation:</b> Calculate planetary transits for any chart over a period of up to one year using a dedicated UI button. With all planets and a wide orb, the system can calculate over 100,000 transits in a single run!</li>
  <li><b>Advanced Transit Search Dialog:</b> Efficiently filter and analyze transits by date, transiting planet, aspect, natal planet, orb, and exclusion filters. Access this powerful tool from the <b>Tools</b> menu.</li>
</ul>

<h2>Calendar &amp; Date Range Features</h2>
<ul>
  <li><b>Julian/Gregorian Calendar Toggle:</b> For dates prior to October 15, 1582, switch between calendar systems using a simple checkbox in the <b>Settings</b> menu for historical accuracy.</li>
  <li><b>Date Range Expansion:</b> Chart and transit calculations now support years from 0001 to 3000 CE (excluding BCE years due to technical limitations).</li>
</ul>

<h2>Data &amp; Table Improvements</h2>
<ul>
  <li><b>Export Chart Data:</b> Export all chart data as text, saving the contents of every table in the <b>Chart Details</b> tab for easy sharing and analysis.</li>
  <li><b>Angles Table:</b> View the values of the four primary chart angles: <b>Ascendant (ASC)</b>, <b>Descendant (DESC)</b>, <b>Imum Coeli (IC)</b>, and <b>Medium Coeli (MC)</b>—always at your fingertips.</li>
</ul>

<h2>Aspect Calculations</h2>
<p>
Our aspect calculations now include the following, each with its astrological significance:
</p>
<table border="1" cellpadding="4" cellspacing="0" style="border-collapse:collapse;">
  <tr><th>Code</th><th>Name</th><th>Angle</th><th>Significance</th></tr>
  <tr><td>CON</td><td>Conjunction</td><td>0°</td><td>Fusion, new beginnings, powerful blending of energies</td></tr>
  <tr><td>OPP</td><td>Opposition</td><td>180°</td><td>Tension, awareness, polarity, need for balance</td></tr>
  <tr><td>SQR</td><td>Square</td><td>90°</td><td>Challenge, action, friction, growth through conflict</td></tr>
  <tr><td>TRI</td><td>Trine</td><td>120°</td><td>Harmony, ease, natural talent, supportive flow</td></tr>
  <tr><td>SEX</td><td>Sextile</td><td>60°</td><td>Opportunity, cooperation, creative potential</td></tr>
  <tr><td>QUI</td><td>Quincunx</td><td>150°</td><td>Adjustment, awkwardness, need for adaptation</td></tr>
  <tr><td>SSQ</td><td>Semi-square</td><td>45°</td><td>Minor friction, irritation, motivation to resolve small issues</td></tr>
  <tr><td>SSX</td><td>Semi-sextile</td><td>30°</td><td>Subtle opportunity, minor growth, gentle nudges</td></tr>
  <tr><td>SQQ</td><td>Sesquiquadrate</td><td>135°</td><td>Dynamic tension, stress, need for adjustment <b>(new in 2.1.0)</b></td></tr>
</table>

<h2>Other Improvements</h2>
<ul>
  <li><b>Chart Type Flag for Transits:</b> Each calculated transit now includes a chart type flag, also passed to the AI interpretation prompt for more targeted and context-aware readings.</li>
  <li><b>General Improvements &amp; Bug Fixes:</b> Numerous minor UI polishes, bug fixes, and code optimizations for a smoother user experience.</li>
</ul>

<p>
We hope you enjoy these new features and improvements. As always, your feedback is invaluable—let us know how these changes enhance your astrological journey!
</p>
        )";

        textBrowser->setHtml(featuresText);

        layout->addWidget(textBrowser);

        // Add a close button at the bottom
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_showNewFeaturesDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        // Connect the close button
        connect(closeButton, &QPushButton::clicked, m_showNewFeaturesDialog, &QDialog::close);

        // Connect the dialog's finished signal to handle cleanup
        connect(m_showNewFeaturesDialog, &QDialog::finished, this, [this]() {
            m_showNewFeaturesDialog->deleteLater();
            m_showNewFeaturesDialog = nullptr;
        });
    }

    // Show and raise the dialog to bring it to the front
    m_showNewFeaturesDialog->show();
    m_showNewFeaturesDialog->raise();
    m_showNewFeaturesDialog->activateWindow();
}

void MainWindow::toggleChartOnlyView(bool chartOnly)
{
    if (chartOnly) {
        // Hide docks, but respect the overlay setting
        m_inputDock->hide();
        m_interpretationDock->hide();
        if (chartInfoOverlay) {
            chartInfoOverlay->setVisible(false);
        }
    } else {
        // Show docks, and restore overlay based on its setting
        m_inputDock->show();
        m_interpretationDock->show();
        if (chartInfoOverlay && m_chartCalculated) {
            chartInfoOverlay->setVisible(m_showInfoOverlay);
        }
    }
}

//event filter for drad-drop
/*
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Only handle events from the chart view's viewport
    if (obj == m_chartView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if ((mouseEvent->buttons() & Qt::LeftButton) &&
                (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
                // Store the starting position for drag operation
                m_dragStartPosition = mouseEvent->pos();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if ((mouseEvent->buttons() & Qt::LeftButton) &&
                (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
                // Check if we've moved enough to start a drag (minimum drag distance)
                if ((mouseEvent->pos() - m_dragStartPosition).manhattanLength()
                    >= QApplication::startDragDistance()) {
                    // Start the drag operation
                    startChartDrag();
                    return true;
                }
            }
        }
    }
    // Let other events be handled normally
    return QMainWindow::eventFilter(obj, event);
}
*/

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_chartView->viewport()) {
        bool ctrlPressed = (QApplication::keyboardModifiers() & Qt::ControlModifier);

        // Handle Ctrl+Mouse Wheel for zooming
        if (event->type() == QEvent::Wheel && ctrlPressed) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            QPoint numDegrees = wheelEvent->angleDelta() / 8;

            if (!numDegrees.isNull()) {
                // Get current scale to apply limits
                qreal currentScale = m_chartView->transform().m11();
                qreal zoomFactor = numDegrees.y() > 0 ? 1.2 : 0.8;
                qreal newScale = currentScale * zoomFactor;

                // Apply zoom limits (0.1x to 10x)
                if (newScale >= 0.1 && newScale <= 10.0) {
                    m_chartView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
                    m_chartView->scale(zoomFactor, zoomFactor);
                }
                return true;
            }
        }
        // Handle Ctrl+Left Drag for chart transfer
        else if (ctrlPressed) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if ((mouseEvent->buttons() & Qt::LeftButton)) {
                    m_dragStartPosition = mouseEvent->pos();
                    return true;
                }
            }
            else if (event->type() == QEvent::MouseMove) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if ((mouseEvent->buttons() & Qt::LeftButton)) {
                    if ((mouseEvent->pos() - m_dragStartPosition).manhattanLength() >=
                            QApplication::startDragDistance()) {
                        startChartDrag();
                        return true;
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::startChartDrag()
{
    if (!m_chartCalculated) return;

    // Create a JSON representation of the input data only
    QJsonObject inputData;
    inputData["birthDate"] = m_birthDateEdit->text();
    inputData["birthTime"] = m_birthTimeEdit->text();
    inputData["utcOffset"] = m_utcOffsetCombo->currentText();
    inputData["latitude"] = m_latitudeEdit->text();
    inputData["longitude"] = m_longitudeEdit->text();
    inputData["houseSystem"] = m_houseSystemCombo->currentText();
    inputData["useJulian"] = useJulianForPre1582Action->isChecked();

    //include chartType
    // Include the chart type
    inputData["chartType"] = GlobalFlags::lastGeneratedChartType;

    // Include interpretation text if available
    if (m_interpretationtextEdit && !m_interpretationtextEdit->toPlainText().isEmpty()) {

        inputData["interpretationText"] = m_interpretationtextEdit->toPlainText();
    }

    inputData["chartData"] = m_currentChartData;

    // Create MIME data for drag
    QMimeData *mimeData = new QMimeData();
    QByteArray jsonData = QJsonDocument(inputData).toJson();
    mimeData->setData("application/x-astrological-input", jsonData);
    mimeData->setText("Astrological Chart Input Data");

    // Create drag object
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Use a simple icon instead of chart screenshot
    drag->setPixmap(QIcon::fromTheme("x-office-calendar").pixmap(32, 32));

    // Start drag
    drag->exec(Qt::CopyAction);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-astrological-input")) {
        event->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-astrological-input")) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-astrological-input")) {
        QByteArray jsonData = event->mimeData()->data("application/x-astrological-input");
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        QJsonObject inputData = doc.object();

        importChartInputData(inputData);
        event->acceptProposedAction();
    }
}

void MainWindow::importChartInputData(const QJsonObject &inputData)
{
    // Populate input fields from dragged data (optional, for UI consistency)
    m_birthDateEdit->setText(inputData["birthDate"].toString());
    m_birthTimeEdit->setText(inputData["birthTime"].toString());
    m_utcOffsetCombo->setCurrentText(inputData["utcOffset"].toString());
    m_latitudeEdit->setText(inputData["latitude"].toString());
    m_longitudeEdit->setText(inputData["longitude"].toString());
    m_houseSystemCombo->setCurrentText(inputData["houseSystem"].toString());
    useJulianForPre1582Action->setChecked(inputData["useJulian"].toBool());

    // Set the chart type from the dragged data
    if (inputData.contains("chartType")) {
        GlobalFlags::lastGeneratedChartType = inputData["chartType"].toString();
    }

    // Extract and set interpretation text if available
    if (inputData.contains("interpretationText") && m_interpretationtextEdit) {
        QString interpretation = inputData["interpretationText"].toString();
        m_interpretationtextEdit->setPlainText(interpretation);
    }

    // Always use the pre-calculated chart data - call displayChart directly!
    QJsonObject chartData = inputData["chartData"].toObject();
    displayChart(chartData);

    m_chartCalculated = true;
    m_currentChartData = chartData;
    statusBar()->showMessage("Chart imported via drag & drop", 3000);
    setWindowTitle("Asteria - " + GlobalFlags::lastGeneratedChartType + " Chart");

}

// tranform markdown to html for ai response
QString MainWindow::markdownToHtml(const QString &markdown)
{
    QString html = markdown;

    // Convert headers
    html.replace(QRegularExpression("^###### (.*)$", QRegularExpression::MultilineOption), "<h6>\\1</h6>");
    html.replace(QRegularExpression("^##### (.*)$", QRegularExpression::MultilineOption), "<h5>\\1</h5>");
    html.replace(QRegularExpression("^#### (.*)$", QRegularExpression::MultilineOption), "<h4>\\1</h4>"); // ← THIS ONE!
    html.replace(QRegularExpression("^### (.*)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.*)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.*)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");

    // Convert bold (**text**)
    html.replace(QRegularExpression("\\*\\*(.*?)\\*\\*"), "<b>\\1</b>");

    // Convert italic (*text*)
    html.replace(QRegularExpression("\\*(.*?)\\*"), "<i>\\1</i>");

    // Convert bullet points
    //html.replace(QRegularExpression("^\\- (.*)$", QRegularExpression::MultilineOption), "• \\1<br>");
    //html.replace(QRegularExpression("^\\- (.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    // Handle • bullets (with optional whitespace)
    html.replace(QRegularExpression("^[\\s]*\\•[\\s]+(.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    // Handle - bullets (with optional whitespace)
    html.replace(QRegularExpression("^[\\s]*\\-[\\s]+(.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    // Handle * bullets (with optional whitespace)
    html.replace(QRegularExpression("^[\\s]*\\*[\\s]+(.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    // Convert horizontal rules (---, ***, ___) with optional spaces
    html.replace(QRegularExpression("^\\s*(---|\\*\\*\\*|___)\\s*$", QRegularExpression::MultilineOption), "<hr>");

    html.replace("\n", "<br>");

    //return "<html><body>" + html + "</body></html>";
    return html;
}

/*
void MainWindow::calculateCurrentChart()
{
    // Use current date and time
    QDate currentDate = QDate::currentDate();
    QTime currentTime = QTime::currentTime();

    // Format as in original fields
    QString dateText = currentDate.toString("dd/MM/yyyy");
    QString timeText = currentTime.toString("HH:mm");

    // Get longitude and latitude from the fields
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();

    // Validate inputs
    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    QString utcOffset = m_utcOffsetCombo->currentText();
    QString houseSystem = m_houseSystemCombo->currentText();

    // Reset chart state before new calculation
    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info
    m_chartRenderer->scene()->clear();

    // Convert formatted date/time back to QDate/QTime for calculation
    QDate birthDate = QDate::fromString(dateText, "dd/MM/yyyy");
    QTime birthTime = QTime::fromString(timeText, "HH:mm");

    birthDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {

        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;

        // Fill name fields with no/ name
        first_name->setText("no");
        last_name->setText("name");
        m_birthDateEdit->setText(currentDate.toString("dd/MM/yyyy"));
        m_birthTimeEdit->setText(currentTime.toString("HH:mm"));
        // Store in global flags
        GlobalFlags::lastGeneratedChartType = "Current Zodiac";

        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Current chart calculated successfully", 3000);
    } else {
        handleError("Chart calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}
*/
/*
void MainWindow::calculateCurrentChart()
{
    // Use current date and time
    QDate currentDate = QDate::currentDate();
    QTime currentTime = QTime::currentTime();

    // Get longitude and latitude from the fields
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();

    // Validate inputs
    if (latitude.isEmpty() || longitude.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter latitude and longitude.");
        return;
    }

    QString utcOffset = m_utcOffsetCombo->currentText();
    QString houseSystem = m_houseSystemCombo->currentText();

    // Reset chart state before new calculation
    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info
    m_chartRenderer->scene()->clear();

    // Convert Julian date if needed
    QDate birthDate = checkAndConvertJulian(currentDate, useJulianForPre1582Action->isChecked());
    QTime birthTime = currentTime;

    // Calculate chart
    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {

        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;

        // Fill name and date/time fields
        first_name->setText("no");
        last_name->setText("name");
        m_birthDateEdit->setText(currentDate.toString("dd/MM/yyyy"));
        m_birthTimeEdit->setText(currentTime.toString("HH:mm"));

        // Set chart type for interpretation
        GlobalFlags::lastGeneratedChartType = "Zodiac Signs";

        // Enable interpretation buttons
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Current chart calculated successfully", 3000);
    } else {
        handleError("Chart calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}
*/


void MainWindow::calculateZodiacSignsChart()
{
    // Show info dialog before proceeding
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "Zodiac Signs Chart",
        "The Zodiac Signs Chart looks like a Birth chart but provides general astrological interpretations "
        "for all 12 zodiac signs based on planetary positions at a chosen date, time, and location.\n\n"
        "This chart's AI interpretations are not personal birth readings but give magazine-style forecasts. "
        "Please make sure to populate the relevant date, time, and location fields before proceeding.\n\n"
        "Do you want to continue?",
        QMessageBox::Ok | QMessageBox::Cancel
    );

    if (reply == QMessageBox::Cancel) {
        return;
    }

    // Proceed like in calculateChart()
    QString dateText = m_birthDateEdit->text();
    if (!validateDateFormat(dateText, this)) {
        return;
    }

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
    birthDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {
        // Display chart
        displayChart(m_currentChartData);
        m_chartCalculated = true;

        // Set default name fields
        first_name->setText("no");
        last_name->setText("name");

        // Set chart type for interpretation
        GlobalFlags::lastGeneratedChartType = "Zodiac Signs";

        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);

        statusBar()->showMessage("Zodiac Signs chart calculated successfully", 3000);
    } else {
        handleError("Chart calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);

        // Clear any partial chart data after error
        m_chartRenderer->scene()->clear();
    }
}
