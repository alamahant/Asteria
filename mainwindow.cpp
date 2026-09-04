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

#if defined(FLATHUB_BUILD) || defined(GENTOO_BUILD)
#else
#include <QPdfWriter>
#include <QPrinter>
#include <QPrintDialog>
#endif

#include <QTextDocument>
#include<QScrollBar>
#include"Globals.h"
#include"aspectsettingsdialog.h"
#include <QCheckBox>
#include <QRegularExpression>
#include<QClipboard>
#include<QDrag>
#include<QDragEnterEvent>
#include<QDragLeaveEvent>
#include<QDragMoveEvent>
#include<QDropEvent>
#include<QMimeData>
#include<QProcess>
#include"socialsharedialog.h"
#include<QCoreApplication>
#include<QDesktopServices>
#include<QApplication>
#include<QProcess>
#include"osmmapdialog.h"
#include"donationdialog.h"
#include "modelselectordialog.h"

extern QString g_astroFontFamily;

const QRegularExpression MainWindow::dateRegex(
        R"(^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(000[1-9]|00[1-9][0-9]|0[1-9][0-9]{2}|[12][0-9]{3}|3000)$)"
);



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chartCalculated(false)
    , m_transitDialog(nullptr)
    , m_dragStartPosition(0, 0) // Initialize drag start position
    , m_socialShare(new SocialShare(this))
    , rssDialog(new RssNotificationDialog(this))
    , m_aspectSearchDialog(nullptr)
    , m_synastrySearchDialog(nullptr)
{
    setAcceptDrops(true);
    preloadMapResources();
    setWindowTitle("Asteria - Astrological Chart Analysis");
    setWindowIcon(QIcon(":/icons/asteria-icon-512.png"));
    setupUi();

    loadSettings();
    if (!m_chartDataManager.isCalculatorAvailable()) {
    }

    connect(languageComboBox, &QComboBox::currentTextChanged,
            &m_mistralApi, &MistralAPI::setLanguage);

    m_symbolsDialog = nullptr;
    m_howToUseDialog = nullptr;
    chartInfoOverlay->setVisible(false);


    QTimer::singleShot(0, this, [this]() {
        this->resize(1200, 800);
    });

    setupShareButton();

    connect(rssDialog, &RssNotificationDialog::newContentAvailable,
            this, [this](bool hasNew){

        if (!rssAction) return;

        if (hasNew) {
            rssAction->setIcon(QIcon(":/icons/rss-green.svg"));
        } else {
            rssAction->setIcon(QIcon(":/icons/rss.svg"));

        }


    });
}

MainWindow::~MainWindow()
{
    saveSettings();

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
    setupCentralWidget();
    setupInputDock();
    setupInterpretationDock();
    setupMenus();
    setupConnections();
    resizeDocks({m_inputDock, m_interpretationDock}, {250, 350}, Qt::Horizontal);

    this->setStyleSheet("QScrollBar:horizontal { height: 0px; background: transparent; }");
}

void MainWindow::setupCentralWidget() {
    m_centralTabWidget = new QTabWidget(this);

    chartContainer = new QWidget(this);
    chartLayout = new QHBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);  // Remove margins for better splitter experience

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, chartContainer);
    mainSplitter->setObjectName("mainSplitter");
    mainSplitter->setHandleWidth(10);
    m_chartView = new QGraphicsView(mainSplitter);

    m_chartView->setAcceptDrops(true);
    m_chartView->viewport()->installEventFilter(this);

    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_chartView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    m_chartView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_chartView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    m_chartRenderer = new ChartRenderer(this);
    m_chartView->setScene(m_chartRenderer->scene());


    chartInfoOverlay = new QWidget(m_chartView);
    chartInfoOverlay->setGeometry(10, 10, 250, 160); // Adjusted position and height
    chartInfoOverlay->setStyleSheet("background-color: rgba(235, 225, 200, 0);"); // Completely transparent background
    QVBoxLayout *infoLayout = new QVBoxLayout(chartInfoOverlay);
    infoLayout->setContentsMargins(0, 2, 0, 2); // Set both left and right margins to 0
    infoLayout->setSpacing(4); // Keep original spacing


    m_nameLabel = new QLabel("Name",chartInfoOverlay);
    m_surnameLabel = new QLabel("Surname",chartInfoOverlay);
    m_birthDateLabel = new QLabel("Birth Date",chartInfoOverlay);
    m_birthTimeLabel = new QLabel("Birth Time",chartInfoOverlay);
    m_locationLabel = new QLabel("Birth Place",chartInfoOverlay);
    m_sunSignLabel = new QLabel(chartInfoOverlay);
    m_ascendantLabel = new QLabel(chartInfoOverlay);
    m_housesystemLabel = new QLabel(chartInfoOverlay);
    infoLayout->addWidget(m_nameLabel);
    infoLayout->addWidget(m_surnameLabel);
    infoLayout->addWidget(m_birthDateLabel);
    infoLayout->addWidget(m_birthTimeLabel);
    infoLayout->addWidget(m_locationLabel);
    infoLayout->addWidget(m_sunSignLabel);
    infoLayout->addWidget(m_ascendantLabel);
    infoLayout->addWidget(m_housesystemLabel);

    mainSplitter->addWidget(m_chartView);

    QSplitter *sidebarSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    sidebarSplitter->setObjectName("sidebarSplitter");
    sidebarSplitter->setHandleWidth(10);
    sidebarSplitter->setStyleSheet( "QSplitter#sidebarSplitter::handle:vertical {" " background: rgba(120,120,120,0.5);" " margin: 0;" "}" "QSplitter#sidebarSplitter::handle:vertical:hover {" " background: rgba(90,90,90,0.8);" "}" "QSplitter#sidebarSplitter::handle:vertical:pressed {" " background: rgba(70,70,70,0.9);" "}" "QSplitter#sidebarSplitter::handle:vertical > * {" " background: transparent;" "}" );

    m_planetListWidget = new PlanetListWidget(sidebarSplitter);
    m_planetListWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    sidebarSplitter->addWidget(m_planetListWidget);

    m_aspectarianWidget = new AspectarianWidget(sidebarSplitter);
    m_aspectarianWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    sidebarSplitter->addWidget(m_aspectarianWidget);

    m_modalityElementWidget = new ElementModalityWidget(sidebarSplitter);
    m_modalityElementWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);


    sidebarSplitter->addWidget(m_modalityElementWidget);

    QList<int> sidebarSizes;
    sidebarSizes << 200 << 300 << 150;  // PlanetList: 200px, Aspectarian: 300px, ModalityElement: 150px
    sidebarSplitter->setSizes(sidebarSizes);


    mainSplitter->addWidget(sidebarSplitter);

    QList<int> mainSizes;
    mainSizes << 75 << 25; // 75 << 25
    mainSplitter->setSizes(mainSizes);


    chartLayout->addWidget(mainSplitter);

    m_chartDetailsWidget = new QWidget(this);
    QVBoxLayout *detailsLayout = new QVBoxLayout(m_chartDetailsWidget);

    detailsTabs = new QTabWidget(m_chartDetailsWidget);

    QTableWidget *planetsTable = new QTableWidget(0, 4, detailsTabs);
    planetsTable->setObjectName("Planets");
    planetsTable->setHorizontalHeaderLabels({"Planet", "Sign", "Degree", "House"});
    planetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QTableWidget *anglesTable = new QTableWidget(0, 3, detailsTabs);
    anglesTable->setObjectName("Angles");
    anglesTable->setHorizontalHeaderLabels({"Angle", "Sign", "Raw Degrees in Dec"});
    anglesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    QTableWidget *housesTable = new QTableWidget(0, 3, detailsTabs);
    housesTable->setObjectName("Houses");
    housesTable->setHorizontalHeaderLabels({"House", "Sign", "Raw Degrees in Dec"});
    housesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    aspectsTable = new QTableWidget(0, 4, detailsTabs);
    aspectsTable->setObjectName("Aspects");
    aspectsTable->setHorizontalHeaderLabels({"Planet 1", "Aspect", "Planet 2", "Orb"});
    aspectsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    rawTransitTable = new QTableWidget(0, 4, detailsTabs);
    rawTransitTable->setObjectName("RawTransits");
    rawTransitTable->setHorizontalHeaderLabels({"Date", "Transit Planet", "Aspect", "Natal Planet (Orb)"});
    rawTransitTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    QTableWidget *eclipseTable = new QTableWidget(0, 6, detailsTabs);
    eclipseTable->setObjectName("Eclipses");
    eclipseTable->setHorizontalHeaderLabels({"Date", "Time", "Type", "Magnitude", "Latitude", "Longitude"});
    eclipseTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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

    //synastry
    m_synastryTable = new QTableWidget(0, 4, this);
    m_synastryTable->setObjectName("Synastry");
    m_synastryTable->setHorizontalHeaderLabels({"Person A Planet", "Aspect", "Person B Planet", "Orb"});
    m_synastryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_synastryTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_synastryTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_synastryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_synastryTable->setContextMenuPolicy(Qt::ActionsContextMenu);
    // Copy action for synastry table
    QAction *copySynastryAction = new QAction("Copy", m_synastryTable);
    copySynastryAction->setShortcut(QKeySequence::Copy);
    m_synastryTable->addAction(copySynastryAction);
    connect(copySynastryAction, &QAction::triggered, [this]() {
        QItemSelectionModel *selection = m_synastryTable->selectionModel();
        QModelIndexList indexes = selection->selectedIndexes();
        if (indexes.isEmpty()) return;

        std::sort(indexes.begin(), indexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
            if (a.row() == b.row()) return a.column() < b.column();
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
            QTableWidgetItem *item = m_synastryTable->item(index.row(), index.column());
            copiedText += item ? item->text() : "";
        }
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(copiedText);
    });
    //

    detailsTabs->addTab(planetsTable, "Planets");
    detailsTabs->addTab(anglesTable, "Angles");
    detailsTabs->addTab(housesTable, "Houses");
    detailsTabs->addTab(aspectsTable, "Aspects");
    detailsTabs->addTab(rawTransitTable, "Raw Transit Data");
    detailsTabs->addTab(eclipseTable, "Eclipses");
    detailsTabs->addTab(m_synastryTable, "Synastry");

    detailsLayout->addWidget(detailsTabs);

    m_centralTabWidget->addTab(chartContainer, "Chart Wheel");
    m_centralTabWidget->addTab(m_chartDetailsWidget, "Chart Details");

    setCentralWidget(m_centralTabWidget);
}

void MainWindow::setupInputDock() {
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


    QWidget *inputWidget = new QWidget(m_inputDock);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputWidget);

    QGroupBox *birthGroup = new QGroupBox("Birth Details", inputWidget);

    QFormLayout *birthLayout = new QFormLayout(birthGroup);

    m_birthDateEdit = new QLineEdit(birthGroup);
    m_birthDateEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format.\n"
                                "Allowed range 0001-3000. Allowed format 'dd/MM/yyyy' Year MUST be in four digit yyyy format.");

    m_birthDateEdit->setPlaceholderText("DD/MM/YYYY");
    QValidator *dateValidator = new QRegularExpressionValidator(dateRegex, this);
    m_birthDateEdit->setValidator(dateValidator);

    QDate currentDate = QDate::currentDate();
    m_birthDateEdit->setText(currentDate.toString("dd/MM/yyyy"));

    m_birthTimeEdit = new QLineEdit(birthGroup);
    m_birthTimeEdit->setToolTip("To set new time, highlight and delete the existing time and set desired with proper format.\n"
                                "Allowed format 'HH:mm'");

    m_birthTimeEdit->setPlaceholderText("HH:MM (24-hour format)");
    QRegularExpression timeRegex("^([01]\\d|2[0-3]):([0-5]\\d)$");
    QValidator *timeValidator = new QRegularExpressionValidator(timeRegex, this);
    m_birthTimeEdit->setValidator(timeValidator);
    QTime currentTime = QTime::currentTime();
    m_birthTimeEdit->setText(currentTime.toString("HH:mm"));

    m_latitudeEdit = new QLineEdit(birthGroup);
    m_latitudeEdit->setReadOnly(true);
    m_latitudeEdit->setToolTip("Please prefer the 'From Google' field or the 'Select on Map' button.");


    m_longitudeEdit = new QLineEdit(birthGroup);
    m_longitudeEdit->setReadOnly(true);
    m_longitudeEdit->setToolTip("Please prefer the 'From Google' field or the 'Select on Map' button.");


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
        if (text.contains(',') &&
                (text.contains('N') || text.contains('n') || text.contains('S') || text.contains('s')) &&
                (text.contains('E') || text.contains('e') || text.contains('W') || text.contains('w'))) {

            QString input = text;
            input.remove(' ');

            int commaPos = input.indexOf(',');
            QString latPart = input.left(commaPos);
            QString longPart = input.mid(commaPos + 1);

            int latDirPos = latPart.indexOf('N');
            if (latDirPos == -1) latDirPos = latPart.indexOf('n');
            if (latDirPos == -1) latDirPos = latPart.indexOf('S');
            if (latDirPos == -1) latDirPos = latPart.indexOf('s');

            int longDirPos = longPart.indexOf('E');
            if (longDirPos == -1) longDirPos = longPart.indexOf('e');
            if (longDirPos == -1) longDirPos = longPart.indexOf('W');
            if (longDirPos == -1) longDirPos = longPart.indexOf('w');

            if (latDirPos != -1 && longDirPos != -1) {
                QString latNumStr = latPart.left(latDirPos).remove(QString::fromUtf8("°"));
                QString latDir = latPart.mid(latDirPos, 1).toUpper();
                QString longNumStr = longPart.left(longDirPos).remove(QString::fromUtf8("°"));
                QString longDir = longPart.mid(longDirPos, 1).toUpper();

                bool latOk, longOk;
                double latDegrees = latNumStr.toDouble(&latOk);
                double longDegrees = longNumStr.toDouble(&longOk);

                if (latOk && longOk) {
                    double latDecimal = latDegrees;
                    if (latDir == "S") latDecimal = -latDecimal;

                    double longDecimal = longDegrees;
                    if (longDir == "W") longDecimal = -longDecimal;

                    m_latitudeEdit->setText(QString::number(latDecimal, 'f', 6));
                    m_longitudeEdit->setText(QString::number(longDecimal, 'f', 6));

                    statusBar()->showMessage("Coordinates converted successfully", 3000);
                }
            }
        }
    });

    locationSearchEdit = new QLineEdit(this);
    locationSearchEdit->setPlaceholderText("Enter location and press Enter to search coordinates");
    locationSearchEdit->setToolTip("Enter location, for example 'Athens Greece', and press Enter to search coordinates");

    connect(locationSearchEdit, &QLineEdit::returnPressed, this, [this]() {
        searchLocationCoordinates(locationSearchEdit->text());
    });



    /*
    m_utcOffsetCombo = new QComboBox(birthGroup);
    for (int i = -12; i <= 14; i++) {
        QString offset = (i >= 0) ? QString("+%1:00").arg(i) : QString("%1:00").arg(i);
        m_utcOffsetCombo->addItem(offset, offset);
    }
    m_utcOffsetCombo->setCurrentText("+00:00");
    */
    m_utcOffsetCombo = new QComboBox(birthGroup);

    QList<QPair<QString, double>> offsetsWithValues;

    for (int i = -12; i <= 14; i++) {
        QString offset = (i >= 0) ? QString("+%1:00").arg(i) : QString("%1:00").arg(i);
        double value = i;
        offsetsWithValues.append(qMakePair(offset, value));
    }

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

    for (auto it = specialOffsets.begin(); it != specialOffsets.end(); ++it) {
        offsetsWithValues.append(qMakePair(it.key(), it.value()));
    }

    std::sort(offsetsWithValues.begin(), offsetsWithValues.end(),
              [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
        return a.second < b.second;
    });

    for (const auto& pair : offsetsWithValues) {
        m_utcOffsetCombo->addItem(pair.first);
    }

    m_utcOffsetCombo->setCurrentText("+00:00");
    m_utcOffsetCombo->setToolTip("Select the UTC offset for the birth location.\n"
                                 "Remember to account for Daylight Saving Time if applicable.\n"
                                 "For accurate charts, you need to determine if DST was in effect\n"
                                 "at the time and location of birth.");



    m_houseSystemCombo = new QComboBox(birthGroup);
    m_houseSystemCombo->addItems({"Placidus", "Koch", "Porphyrius", "Regiomontanus", "Campanus", "Equal", "Whole Sign"});

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





    m_selectLocationButton = new QPushButton("Select on Map", birthGroup);
    m_selectLocationButton->setIcon(QIcon::fromTheme("view-refresh"));
    connect(m_selectLocationButton, &QPushButton::clicked, this, &MainWindow::onOpenMapClicked);

    m_selectLocationButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_selectLocationButton->setMinimumWidth(200);  // Set a reasonable minimum width

    birthLayout->addRow("Location:", m_selectLocationButton);

    m_selectLocationButton->setMinimumWidth(locationSearchEdit->width());


    birthLayout->addRow("UTC Offset:", m_utcOffsetCombo);
    birthLayout->addRow("House System:", m_houseSystemCombo);
    QWidget *orbContainer = new QWidget(inputWidget);
    QVBoxLayout *orbLayout = new QVBoxLayout(orbContainer);
    orbLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *sliderLayout = new QHBoxLayout();

    QSlider *orbSlider = new QSlider(Qt::Horizontal, orbContainer);
    orbSlider->setRange(0, 24);  // 0 to 12 in 0.5° increments (multiply by 2)
    orbSlider->setValue(static_cast<int>(getOrbMax() * 2)); // Convert current value to slider range
    orbSlider->setTickInterval(4); // Tick marks every 2° (multiply by 2)
    orbSlider->setTickPosition(QSlider::TicksBelow);
    orbSlider->setMinimumWidth(150);

    QLabel *orbValueLabel = new QLabel(QString::number(getOrbMax(), 'f', 1) + "°", orbContainer);
    orbValueLabel->setMinimumWidth(40);
    orbValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sliderLayout->addWidget(orbSlider);
    sliderLayout->addWidget(orbValueLabel);

    QLabel *orbDescriptionLabel = new QLabel(getOrbDescription(getOrbMax()), orbContainer);
    orbDescriptionLabel->setAlignment(Qt::AlignCenter);

    orbLayout->addLayout(sliderLayout);
    orbLayout->addWidget(orbDescriptionLabel);

    connect(orbSlider, &QSlider::valueChanged, [=](int value) {
        double orb = value / 2.0;
        orbValueLabel->setText(QString::number(orb, 'f', 1) + "°");
        orbDescriptionLabel->setText(getOrbDescription(orb));
        setOrbMax(orb); // Update the global setting
    });

    birthLayout->addRow("Aspect Orbs:", orbContainer);


    m_additionalBodiesCB = new QCheckBox("Include Additional Bodies", this);
    m_additionalBodiesCB->setToolTip("Include Lilith, Ceres, Pallas, Juno, Vesta, Vertex, East Point and Part of Spirit");
    connect(m_additionalBodiesCB, &QCheckBox::toggled, this, [this](bool checked) {

        GlobalFlags::additionalBodiesEnabled = checked;

        if (m_chartCalculated) {
            displayChart(m_currentChartData);
        }

    });
    birthLayout->addRow(m_additionalBodiesCB);

    m_calculateButton = new QPushButton("Calculate Chart", inputWidget);
    m_calculateButton->setIcon(QIcon::fromTheme("view-refresh"));


    QGroupBox *predictiveGroup = new QGroupBox("Predictive Astrology", inputWidget);
    QFormLayout *predictiveLayout = new QFormLayout(predictiveGroup);

    m_predictiveFromEdit = new QLineEdit(predictiveGroup);
    m_predictiveFromEdit->setPlaceholderText("DD/MM/YYYY");
    m_predictiveFromEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format");

    m_predictiveFromEdit->setValidator(dateValidator); // Reuse the same validator
    m_predictiveFromEdit->setText(currentDate.toString("dd/MM/yyyy"));

    m_predictiveToEdit = new QLineEdit(predictiveGroup);
    m_predictiveToEdit->setPlaceholderText("DD/MM/YYYY");
    m_predictiveToEdit->setToolTip("To set new date, highlight and delete the existing date and set desired with proper format");

    m_predictiveToEdit->setValidator(dateValidator); // Reuse the same validator
    QDate defaultFutureDate = currentDate.addDays(1); // Just a default starting point
    m_predictiveFromEdit->setText(currentDate.toString("dd/MM/yyyy"));
    m_predictiveToEdit->setText(defaultFutureDate.toString("dd/MM/yyyy"));

    predictiveLayout->addRow("From:", m_predictiveFromEdit);
    predictiveLayout->addRow("Up to:", m_predictiveToEdit);
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

    getPredictionButton = new QPushButton("Get AI Prediction", predictiveGroup);
    getPredictionButton->setToolTip("This operation generates a huge ammount of data that is sent to AI for interpretation.\n"
                                    "Therefore it may be costly tokenwise.\n To mitigate this please reduce the number of days and/or use smaller orb.");
    getPredictionButton->setEnabled(false);
    getPredictionButton->setIcon(QIcon::fromTheme("view-refresh"));
    getPredictionButton->setStatusTip("The AI prediction will be appended at the end of any existing text. Scroll down and be patient!");

    getTransitsButton = new QPushButton("Calculate Transits", predictiveGroup);
    getTransitsButton->setToolTip("Calculate transits for the selected period");
    getTransitsButton->setEnabled(false);
    getTransitsButton->setIcon(QIcon::fromTheme("view-chart"));
    getTransitsButton->setStatusTip("Calculate transits for the selected period");

    buttonLayout->addWidget(getPredictionButton);
    buttonLayout->addWidget(getTransitsButton);

    predictiveLayout->addRow(buttonLayout);

    connect(getTransitsButton, &QPushButton::clicked, this, &MainWindow::CalculateTransits);




    inputLayout->addWidget(birthGroup);
    inputLayout->addWidget(m_calculateButton);

    inputLayout->addWidget(predictiveGroup);
    inputLayout->addStretch();

    m_inputDock->setWidget(inputWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_inputDock);
}


void MainWindow::setupInterpretationDock() {
    m_interpretationDock = new QDockWidget("Chart Interpretation", this);
    m_interpretationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_interpretationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    QWidget *interpretationWidget = new QWidget(m_interpretationDock);
    QVBoxLayout *interpretationLayout = new QVBoxLayout(interpretationWidget);

    m_getInterpretationButton = new QPushButton("Get Chart Interpretation From AI", interpretationWidget);
    m_getInterpretationButton->setIcon(QIcon::fromTheme("system-search"));
    m_getInterpretationButton->setEnabled(false);

    m_interpretationtextEdit = new QTextEdit(interpretationWidget);
    m_interpretationtextEdit->setAcceptRichText(true);
    m_interpretationtextEdit->setReadOnly(true);
    m_interpretationtextEdit->setPlaceholderText("AI interpretation will appear here after you click the 'Get Chart Interpretation From AI' button.");

    QHBoxLayout* languageLayout = new QHBoxLayout();
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
    languageComboBox->addItem("Modern Standard Arabic");

    languageComboBox->setCurrentIndex(0);
    languageComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    QPushButton *clearTextButton = new QPushButton("ClearText", this);
    clearTextButton->setToolTip("Clear AI Interpretation Text Area");
    clearTextButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    connect(clearTextButton, &QPushButton::clicked, this, [this]() {
        m_currentInterpretation.clear();              // Clear stored interpretations
        m_interpretationtextEdit->clear();           // Clear the QTextEdit content
    });

    languageLayout->addWidget(languageComboBox);

    interpretationLayout->addWidget(m_getInterpretationButton);
    interpretationLayout->addWidget(m_interpretationtextEdit);
    interpretationLayout->addLayout(languageLayout);
    interpretationLayout->addWidget(clearTextButton);

    m_interpretationDock->setWidget(interpretationWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_interpretationDock);
}



void MainWindow::setupMenus()
{

    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *newWindowAction = fileMenu->addAction("New &Window in New Process");
    newWindowAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    newWindowAction->setIcon(QIcon::fromTheme("window-new"));
    newWindowAction->setStatusTip("Open a new application window");

    connect(newWindowAction, &QAction::triggered, this, [this]() {
        QProcess::startDetached(QApplication::applicationFilePath(), QStringList());
    });

    fileMenu->addSeparator();


    QAction *newSameProcessAction = fileMenu->addAction("New &Window");
    newSameProcessAction->setShortcut(QKeySequence("Ctrl+Shift+W"));
    newSameProcessAction->setIcon(QIcon::fromTheme("window-new"));
    newSameProcessAction->setStatusTip("Open a new window within current application");

    connect(newSameProcessAction, &QAction::triggered, this, [this]() {
        MainWindow *newWindow = new MainWindow();
        newWindow->show();
    });

    fileMenu->addSeparator();

    QAction *openChartInNewWindowAction = fileMenu->addAction("Open Chart in New &Window");
    openChartInNewWindowAction->setShortcut(QKeySequence("Ctrl+Alt+O"));
    openChartInNewWindowAction->setIcon(QIcon::fromTheme("window-duplicate"));
    openChartInNewWindowAction->setStatusTip("Open current chart with all data in a new window");

    connect(openChartInNewWindowAction, &QAction::triggered, this, [this]() {
        if (!m_chartCalculated) {
            QMessageBox::information(this, "No Chart", "Please calculate a chart first.");
            return;
        }

        QJsonObject chartData;
        chartData["birthDate"] = m_birthDateEdit->text();
        chartData["birthTime"] = m_birthTimeEdit->text();
        chartData["utcOffset"] = m_utcOffsetCombo->currentText();
        chartData["latitude"] = m_latitudeEdit->text();
        chartData["longitude"] = m_longitudeEdit->text();
        chartData["houseSystem"] = m_houseSystemCombo->currentText();
        chartData["useJulian"] = useJulianForPre1582Action->isChecked();
        chartData["chartType"] = GlobalFlags::lastGeneratedChartType;

        if (m_interpretationtextEdit && !m_interpretationtextEdit->toPlainText().isEmpty()) {
            chartData["interpretationText"] = m_interpretationtextEdit->toPlainText();
        }

        chartData["chartData"] = m_currentChartData;

        MainWindow *newWindow = new MainWindow();
        newWindow->importChartInputData(chartData);
        newWindow->show();

        statusBar()->showMessage("Chart opened in new window", 3000);
    });
    fileMenu->addSeparator();

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
    QAction *saveSynastryAction = fileMenu->addAction("Save Synastry Data");
    saveSynastryAction->setShortcut(QKeySequence("Ctrl+Alt+S"));
    saveSynastryAction->setIcon(QIcon(":/icons/link.svg"));
    saveSynastryAction->setStatusTip("Save current synastry data");
    connect(saveSynastryAction, &QAction::triggered, this, &MainWindow::saveSynastry);

    QAction *loadSynastryAction = fileMenu->addAction("Load Synastry Data");
    loadSynastryAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    loadSynastryAction->setIcon(QIcon(":/icons/link-2.svg"));

    loadSynastryAction->setStatusTip("Load previously saved synastry data");
    connect(loadSynastryAction, &QAction::triggered, this, &MainWindow::loadSynastry);

    fileMenu->addSeparator();
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

    QAction *getDataDirAction = fileMenu->addAction("Copy Save Location...", this, &MainWindow::copySavePath);
    getDataDirAction->setShortcut(QKeySequence("Ctrl+C"));
    getDataDirAction->setIcon(QIcon::fromTheme("edit-copy"));

    fileMenu->addSeparator();


    QAction *printAction = fileMenu->addAction("&Print...", this, &MainWindow::printChart);
    printAction->setShortcut(QKeySequence::Print);
    printAction->setIcon(QIcon::fromTheme("document-print"));

    fileMenu->addSeparator();


    QAction *openFolderAction = fileMenu->addAction("&Open Data Directory");
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);
    fileMenu->addSeparator();

    fileMenu->addSeparator();
    QAction *createSymlinkAction = fileMenu->addAction(QString("Create Shortcut to %1 Data").arg(QApplication::applicationName()));
    connect(createSymlinkAction, &QAction::triggered, this, &MainWindow::createSymlink);
    fileMenu->addSeparator();

    rssAction = fileMenu->addAction("&RSS Notifications");
    rssAction->setShortcut(QKeySequence("Ctrl+R"));
    rssAction->setIcon(QIcon(":/icons/rss.svg"));
    connect(rssAction, &QAction::triggered, this, [this]{
        if(rssDialog) rssDialog->show();
    });
    fileMenu->addAction(rssAction);
    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("E&xit", this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setIcon(QIcon::fromTheme("application-exit"));

    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_inputDock->toggleViewAction());
    viewMenu->addAction(m_interpretationDock->toggleViewAction());

    m_chartOnlyAction = new QAction("&View Chart Only", this);
    m_chartOnlyAction->setCheckable(true);
    m_chartOnlyAction->setChecked(false); // Default to not checked
    connect(m_chartOnlyAction, &QAction::toggled, this, &MainWindow::toggleChartOnlyView);
    viewMenu->addAction(m_chartOnlyAction);

    showOverlayAction = new QAction("Show &Info Overlay", this);
    showOverlayAction->setCheckable(true);
    showOverlayAction->setChecked(false); // Default to not checked (hidden)

    connect(showOverlayAction, &QAction::toggled, this, [this](bool checked) {
        m_showInfoOverlay = checked;
        if(chartInfoOverlay && m_chartCalculated)
            chartInfoOverlay->setVisible(checked);
    });

    viewMenu->addAction(showOverlayAction);



    QMenu *settingsMenu = menuBar()->addMenu("&Settings");


    QAction *aiModelsAction = settingsMenu->addAction("Configure AI &Models...", this, &MainWindow::configureAIModels);



    QAction *checkModelAction = settingsMenu->addAction("Check AI Model &Status", this, [this]() {
        if (!GlobalFlags::activeModelLoaded) {
            m_mistralApi.loadActiveModel();
            if (!GlobalFlags::activeModelLoaded) {

                QMessageBox msgBox(this);
                msgBox.setWindowTitle("AI Model Not Configured");
                msgBox.setText("No active AI model found. You need to configure a model to get chart interpretations.");
                msgBox.setInformativeText("Would you like to configure one now?\n\n"
                                          "Note: If you've been using Mistral, you can add it as a provider with your API key.");

                QPushButton *configureButton = msgBox.addButton("Configure Models", QMessageBox::ActionRole);
                QPushButton *closeButton = msgBox.addButton(QMessageBox::Close);

                msgBox.exec();

                if (msgBox.clickedButton() == configureButton) {
                    configureAIModels();
                }
            }
        } else {
            QSettings settings;
            settings.beginGroup("Models");
            QString activeModelName = settings.value("ActiveModel").toString();
            settings.beginGroup(activeModelName);

            QString provider = settings.value("provider").toString();
            QString endpoint = settings.value("endpoint").toString();
            QString modelName = settings.value("modelName").toString();
            QString apiKey = settings.value("apiKey").toString();
            double temperature = settings.value("temperature", 0.7).toDouble();
            int maxTokens = settings.value("maxTokens", 8192).toInt();

            settings.endGroup();
            settings.endGroup();

            QString statusMessage = QString(
                        "<b>Active Model:</b> %1<br><br>"
                        "<b>Provider:</b> %2<br>"
                        "<b>Model:</b> %3<br>"
                        "<b>Endpoint:</b> %4<br>"
                        "<b>Temperature:</b> %5<br>"
                        "<b>Max Tokens:</b> %6<br>"
                        "<b>API Key:</b> %7"
                        ).arg(activeModelName)
                    .arg(provider)
                    .arg(modelName)
                    .arg(endpoint)
                    .arg(temperature)
                    .arg(maxTokens)
                    .arg(apiKey.isEmpty() ? "<font color='red'><b>MISSING</b></font>" : "<font color='green'><b>Configured</b></font>");

            if (apiKey.isEmpty() && !endpoint.contains("localhost") && !endpoint.contains("127.0.0.1")) {
                statusMessage += "<br><br><font color='red'><b>WARNING:</b> This appears to be a cloud provider but no API key is set. Interpretations will fail.</font>";
            }

            QMessageBox::information(this, "AI Model Status", statusMessage);
        }
    });
    checkModelAction->setIcon(QIcon::fromTheme("dialog-information"));

    QAction* aiModelInfoAction = new QAction("AI Model Info Guide", this);
    connect(aiModelInfoAction, &QAction::triggered, this, &MainWindow::showAIConfigGuide);
    settingsMenu->addAction(aiModelInfoAction);

    settingsMenu->addSeparator();
    QAction *aspectSettingsAction = new QAction("&Aspect Display Settings...", this);
    connect(aspectSettingsAction, &QAction::triggered, this, &MainWindow::showAspectSettings);
    settingsMenu->addAction(aspectSettingsAction);

    settingsMenu->addSeparator();
    useJulianForPre1582Action = new QAction(tr("Use Julian calendar for dates before 15 October 1582"), this);
    useJulianForPre1582Action->setCheckable(true);
    QSettings settings;
    if (settings.contains("useJulianForPre1582")) {
        useJulianForPre1582Action->setChecked(settings.value("useJulianForPre1582").toBool());
    } else {
        useJulianForPre1582Action->setChecked(true); // Default: checked (use Julian for pre-1582)
    }
    settingsMenu->addAction(useJulianForPre1582Action);

    connect(useJulianForPre1582Action, &QAction::toggled, this, [this](bool checked) {
        qDebug() << "checked";
        QSettings settings;
        settings.setValue("useJulianForPre1582", checked);
    });

    settingsMenu->addSeparator();

    QAction *resetSettingsAction = settingsMenu->addAction("Reset Settings");
    connect(resetSettingsAction, &QAction::triggered, this, &MainWindow::onResetSettings);

    QMenu *toolsMenu = menuBar()->addMenu("Tools");

    QMenu *relationshipMenu = toolsMenu->addMenu("Relationship Charts");

    QAction *compositeAction = new QAction("Composite Chart (exp)", this);
    QAction *davisonAction = new QAction("Davison Relationship Chart", this);
    QAction *synastryAction = new QAction("Synastry Chart", this);
    synastryAction->setIcon(QIcon(":/icons/link.svg"));
    connect(synastryAction, &QAction::triggered, this, &MainWindow::loadSynastryCharts);

    relationshipMenu->addAction(compositeAction);
    relationshipMenu->addSeparator();
    relationshipMenu->addAction(davisonAction);
    relationshipMenu->addSeparator();
    relationshipMenu->addAction(synastryAction);

    //synastryAction->setEnabled(false);

    connect(compositeAction, &QAction::triggered, this, &MainWindow::createCompositeChart);
    connect(davisonAction, &QAction::triggered, this, &MainWindow::createDavisonChart);
    connect(synastryAction, &QAction::triggered, this, &MainWindow::createSynastryChart);


    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About...", this, &MainWindow::showAboutDialog);
    aboutAction->setIcon(QIcon::fromTheme("help-about"));
    helpMenu->addSeparator();
    QAction *symbolsAction = helpMenu->addAction(tr("Astrological &Symbols"));
    connect(symbolsAction, &QAction::triggered, this, &MainWindow::showSymbolsDialog);
    helpMenu->addSeparator();
    QAction *howToUseAction = helpMenu->addAction(tr("&How to Use"));
    connect(howToUseAction, &QAction::triggered, this, &MainWindow::showHowToUseDialog);
    helpMenu->addSeparator();
    QAction *relationshipChartsAction = helpMenu->addAction(tr("About &Relationship Charts"));
    connect(relationshipChartsAction, &QAction::triggered, this, &MainWindow::showRelationshipChartsDialog);
    helpMenu->addSeparator();
    QAction *changelogAction = helpMenu->addAction(tr("Changelog"));
    connect(changelogAction, &QAction::triggered, this, &MainWindow::showChangelog);
    helpMenu->addSeparator();

    QAction *newFeaturesAction = helpMenu->addAction(tr("What's New!"));
    connect(newFeaturesAction, &QAction::triggered, this, &MainWindow::showNewFeaturesDialog);
    newFeaturesAction->setVisible(false);
    QAction *supportAction = helpMenu->addAction(tr("Support Us"));
    connect(supportAction, &QAction::triggered, this, [this]() {
        DonationDialog dialog(this);
        dialog.exec();
    });

    toolsMenu->addSeparator();
    QAction *transitFilterAction = new QAction("Transit Filter", this);
    transitFilterAction->setToolTip("Filter transit data by date, planets and aspects");
    transitFilterAction->setStatusTip("Open transit filter dialog");
    transitFilterAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(transitFilterAction, &QAction::triggered, this, &MainWindow::openTransitFilter);
    toolsMenu->addAction(transitFilterAction);

    QAction *aspectFilterAction = new QAction("Aspects Filter", this);
    aspectFilterAction->setToolTip("Filter aspect data by planets and aspects");
    aspectFilterAction->setStatusTip("Open aspect filter dialog");
    aspectFilterAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    connect(aspectFilterAction, &QAction::triggered, this, &MainWindow::openAspectsFilter);
    toolsMenu->addAction(aspectFilterAction);

    QAction *synastryFilterAction = new QAction("Synastry Filter", this);
    synastryFilterAction->setToolTip("Filter synastry data by planets and aspects");
    synastryFilterAction->setStatusTip("Open synastry filter dialog");
    synastryFilterAction->setShortcut(QKeySequence("Ctrl+Shift+Y"));
    connect(synastryFilterAction, &QAction::triggered, this, &MainWindow::openSynastryFilter);
    toolsMenu->addAction(synastryFilterAction);

    toolsMenu->addSeparator();
    QAction *eclipseCalcAction = new QAction("Calculate Eclipses", this);
    eclipseCalcAction->setToolTip("Calculate solar and lunar eclipses in the selected date range");
    eclipseCalcAction->setStatusTip("Calculate eclipses for the current chart and date range");
    eclipseCalcAction->setShortcut(QKeySequence("Ctrl+Shift+E"));

    connect(eclipseCalcAction, &QAction::triggered, this, &MainWindow::CalculateEclipses);

    toolsMenu->addAction(eclipseCalcAction);

    toolsMenu->addSeparator();
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

    QAction *venusReturnCalcAction = new QAction("Calculate Venus Return", this);
    venusReturnCalcAction->setToolTip("Calculate the Venus return chart for a selected return number");
    venusReturnCalcAction->setStatusTip("Calculate the Venus return chart for the current birth data and chosen return number");
    venusReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+V"));
    connect(venusReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateVenusReturn);
    returnChartsMenu->addAction(venusReturnCalcAction);

    QAction *marsReturnCalcAction = new QAction("Calculate Mars Return", this);
    marsReturnCalcAction->setToolTip("Calculate the Mars return chart for a selected return number");
    marsReturnCalcAction->setStatusTip("Calculate the Mars return chart for the current birth data and chosen return number");
    marsReturnCalcAction->setShortcut(QKeySequence("Ctrl+Alt+R"));
    connect(marsReturnCalcAction, &QAction::triggered, this, &MainWindow::calculateMarsReturn);
    returnChartsMenu->addAction(marsReturnCalcAction);

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

    toolsMenu->addSeparator();
    QAction *secondaryProgressionAction = new QAction("Calculate Secondary Progression Chart", this);
    secondaryProgressionAction->setToolTip("Calculate a secondary progression chart for a selected year of life");
    secondaryProgressionAction->setStatusTip("Calculate the secondary progression chart for the current birth data and chosen progression year");
    secondaryProgressionAction->setShortcut(QKeySequence("Ctrl+G")); // Choose a shortcut that doesn't conflict
    connect(secondaryProgressionAction, &QAction::triggered, this, &MainWindow::calculateSecondaryProgression);
    toolsMenu->insertAction(nullptr, secondaryProgressionAction); // Add at the top of Tools menu

    toolsMenu->addSeparator();
    QAction *zodiacChartAction = new QAction("Calculate Zodiac Chart", this);
    zodiacChartAction->setToolTip("Calculate a chart for all Zodiac Signs");
    zodiacChartAction->setShortcut(QKeySequence("Ctrl+H")); // Choose a shortcut that doesn't conflict
    connect(zodiacChartAction, &QAction::triggered, this, &MainWindow::calculateZodiacSignsChart);
    toolsMenu->insertAction(nullptr, zodiacChartAction); // Add at the top of Tools menu

}

void MainWindow::setupConnections()
{
    connect(m_calculateButton, &QPushButton::clicked, this, &MainWindow::calculateChart);

    connect(m_getInterpretationButton, &QPushButton::clicked, this, &MainWindow::getInterpretation);

    connect(&m_mistralApi, &MistralAPI::interpretationReady, this, &MainWindow::displayInterpretation);
    connect(&m_mistralApi, &MistralAPI::error, this, &MainWindow::handleError);

    connect(&m_chartDataManager, &ChartDataManager::error, this, &MainWindow::handleError);

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

    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info

    m_chartRenderer->scene()->clear();

    birthDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {

        displayChart(m_currentChartData);
        m_chartCalculated = true;

        GlobalFlags::lastGeneratedChartType = "Natal Birth";

        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);
        statusBar()->showMessage("Chart calculated successfully", 3000);
    } else {
        handleError("Chart calculation error: " + m_chartDataManager.getLastError());
        m_chartCalculated = false;
        m_getInterpretationButton->setEnabled(false);
        getPredictionButton->setEnabled(false);
        m_chartRenderer->scene()->clear();
    }
}



void MainWindow::displayChart(const QJsonObject &chartData) {

    QStringList additionalBodies = {
        "Ceres", "Pallas", "Juno", "Vesta", "Lilith",
        "Vertex", "Part of Spirit", "East Point"
    };

    QJsonObject filteredChartData = chartData;

    if (!m_additionalBodiesCB->isChecked()) {
        QJsonArray planets = filteredChartData["planets"].toArray();
        QJsonArray filteredPlanets;

        for (int i = 0; i < planets.size(); i++) {
            QJsonObject planet = planets[i].toObject();
            QString planetId = planet["id"].toString();

            if (!additionalBodies.contains(planetId)) {
                filteredPlanets.append(planet);
            }
        }
        filteredChartData["planets"] = filteredPlanets;

        QJsonArray aspects = filteredChartData["aspects"].toArray();
        QJsonArray filteredAspects;

        for (int i = 0; i < aspects.size(); i++) {
            QJsonObject aspect = aspects[i].toObject();
            QString planet1 = aspect["planet1"].toString();
            QString planet2 = aspect["planet2"].toString();

            if (!additionalBodies.contains(planet1) && !additionalBodies.contains(planet2)) {
                filteredAspects.append(aspect);
            }
        }
        filteredChartData["aspects"] = filteredAspects;
    }

    ChartData data = convertJsonToChartData(filteredChartData);

    m_chartRenderer->setChartData(data);
    m_chartRenderer->renderChart();

    m_planetListWidget->updateData(data);
    m_aspectarianWidget->updateData(data);
    m_modalityElementWidget->updateData(data);

    updateChartDetailsTables(filteredChartData);

    chartInfoOverlay->setVisible(m_showInfoOverlay);
    populateInfoOverlay();

    m_centralTabWidget->setCurrentIndex(0);
}


void MainWindow::updateChartDetailsTables(const QJsonObject &chartData)
{
    QTableWidget *planetsTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Planets");
    QTableWidget *anglesTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Angles");
    QTableWidget *housesTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Houses");
    QTableWidget *aspectsTable = m_chartDetailsWidget->findChild<QTabWidget*>()->findChild<QTableWidget*>("Aspects");

    if (!planetsTable || !housesTable || !aspectsTable || ! anglesTable) {
        return;
    }

    planetsTable->setRowCount(0);
    anglesTable->setRowCount(0);
    housesTable->setRowCount(0);
    aspectsTable->setRowCount(0);

    if (chartData.contains("planets") && chartData["planets"].isArray()) {
        QJsonArray planets = chartData["planets"].toArray();
        planetsTable->setRowCount(planets.size());

        for (int i = 0; i < planets.size(); ++i) {
            QJsonObject planet = planets[i].toObject();

            QString planetName = planet["id"].toString();
            if (planet["isRetrograde"].toBool() && planetName != "North Node" && planetName != "South Node") {
                planetName += "   ℞"; // Using the official retrograde symbol (℞)
            }

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

    QMap<QString, QString> angleLongNames = {
        {"Asc", "Ascendant"},
        {"Desc", "Descendant"},
        {"MC", "Medium Coeli (Midheaven)"},
        {"IC", "Imum Coeli (Nadir)"}
    };
    if (chartData.contains("angles") && chartData["angles"].isArray()) {
        QJsonArray angles = chartData["angles"].toArray();
        anglesTable->setRowCount(angles.size());
        for (int i = 0; i < angles.size(); ++i) {
            QJsonObject angle = angles[i].toObject();
            QString angleName = angle["id"].toString(); // e.g., ASC, DESC, IC, MC
            QString longName = angleLongNames.value(angleName, angleName); // fallback to id if not found

            QString signName = angle["sign"].toString();
            QString degreeStr = QString::number(angle["longitude"].toDouble(), 'f', 2) + "°";
            QTableWidgetItem *nameItem = new QTableWidgetItem(longName);


            QTableWidgetItem *signItem = new QTableWidgetItem(signName);
            QTableWidgetItem *degreeItem = new QTableWidgetItem(degreeStr);
            anglesTable->setItem(i, 0, nameItem);
            anglesTable->setItem(i, 1, signItem);
            anglesTable->setItem(i, 2, degreeItem);
        }
    }


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

            QString planet1Name = aspect["planet1"].toString();
            QString planet2Name = aspect["planet2"].toString();

            bool planet1Retrograde = false;
            bool planet2Retrograde = false;

            for (int j = 0; j < planets.size(); ++j) {
                QJsonObject planet = planets[j].toObject();
                if (planet["id"].toString() == planet1Name) {
                    planet1Retrograde = planet["isRetrograde"].toBool();
                }
                if (planet["id"].toString() == planet2Name) {
                    planet2Retrograde = planet["isRetrograde"].toBool();
                }
            }

            QString planet1Display = planet1Name;
            if (planet1Retrograde && planet1Name != "North Node" && planet1Name != "South Node") {
                planet1Display += " ℞";
            }

            QString planet2Display = planet2Name;
            if (planet2Retrograde && planet2Name != "North Node" && planet2Name != "South Node") {
                planet2Display += " ℞";
            }

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

    if (!GlobalFlags::activeModelLoaded) {
        m_mistralApi.loadActiveModel();
        if (!GlobalFlags::activeModelLoaded) {

            QMessageBox::information(this, "AI Model Not Configured",
                                     "No active AI model found. Please go to Settings → Configure AI Models to set up a model.");
            return;
        }
    }

    QJsonObject dataToSend = m_currentChartData;

    QStringList additionalBodies = {
        "Ceres", "Pallas", "Juno", "Vesta", "Lilith",
        "Vertex", "Part of Spirit", "East Point"
    };

    if (!m_additionalBodiesCB->isChecked()) {
        QJsonArray planets = dataToSend["planets"].toArray();
        QJsonArray filteredPlanets;
        for (int i = 0; i < planets.size(); i++) {
            QJsonObject planet = planets[i].toObject();
            QString planetId = planet["id"].toString();
            if (!additionalBodies.contains(planetId)) {
                filteredPlanets.append(planet);
            }
        }
        dataToSend["planets"] = filteredPlanets;

        QJsonArray aspects = dataToSend["aspects"].toArray();
        QJsonArray filteredAspects;
        for (int i = 0; i < aspects.size(); i++) {
            QJsonObject aspect = aspects[i].toObject();
            QString planet1 = aspect["planet1"].toString();
            QString planet2 = aspect["planet2"].toString();

            if (!additionalBodies.contains(planet1) && !additionalBodies.contains(planet2)) {
                filteredAspects.append(aspect);
            }
        }
        dataToSend["aspects"] = filteredAspects;
    }

    m_interpretationtextEdit->append("Requesting interpretation from AI...\n");
    m_getInterpretationButton->setEnabled(false);
    statusBar()->showMessage("Requesting interpretation...");

    m_mistralApi.interpretChart(dataToSend);
}



void MainWindow::displayInterpretation(const QString &interpretation)
{
    m_currentInterpretation += interpretation;

    QString htmlInterpretation = markdownToHtml(interpretation);

    QString existingHtml = m_interpretationtextEdit->toHtml();

    QString header = QString(
                "<p><b>Chart reading for %1 %2</b> born on %3 at %4 in location %5</p>"
                ).arg(
                first_name->text(),
                last_name->text(),
                m_birthDateEdit->text(),
                m_birthTimeEdit->text(),
                m_googleCoordsEdit->text()
                );

    QString fullHtml = existingHtml + "\n" + header + "\n" + htmlInterpretation + "\n" +
            "<p><i>Received interpretation from AI...</i></p>";

    m_interpretationtextEdit->setAcceptRichText(true);
    m_interpretationtextEdit->setHtml(fullHtml);

    m_getInterpretationButton->setEnabled(true);
    statusBar()->showMessage("Interpretation received", 3000);
}

QString MainWindow::plainTextToHtml(const QString &plainText)
{
    if (plainText.isEmpty()) return "";

    QString html = plainText;
    html.replace("\n\n", "</p><p>");
    html.replace("\n", "<br>");
    return "<p>" + html + "</p>";
}


void MainWindow::newChart() {
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
    m_predictiveFromEdit->setText(QDate::currentDate().toString("dd/MM/yyyy"));
    m_predictiveToEdit->setPlaceholderText("DD/MM/YYYY");
    QDate defaultFutureDate = QDate::currentDate().addDays(1); // Just a default starting point
    m_predictiveToEdit->setText(defaultFutureDate.toString("dd/MM/yyyy"));

    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentInterpretation.clear();
    m_currentRelationshipInfo = QJsonObject();  // Reset relationship info

    m_chartRenderer->scene()->clear();

    m_interpretationtextEdit->clear();
    m_interpretationtextEdit->setPlaceholderText("Calculate a chart and click 'Get AI Interpretation'");
    m_getInterpretationButton->setEnabled(false);

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

    QJsonObject saveData;
    saveData["chartData"] = m_currentChartData;
    saveData["interpretation"] = m_currentInterpretation;

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

    if (m_currentRelationshipInfo.isEmpty() == false) {
        saveData["relationshipInfo"] = m_currentRelationshipInfo;
    }

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

    newChart();

    QString appName = QApplication::applicationName();
    QString appDir = GlobalFlags::appDir;
#ifdef FLATHUB_BUILD
#else
#endif

    QDir dir;
    if (!dir.exists(appDir))
        dir.mkpath(appDir);

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

            if (saveData.contains("chartData") && saveData["chartData"].isObject()) {
                m_currentChartData = saveData["chartData"].toObject();
                displayChart(m_currentChartData);
                m_chartCalculated = true;
                m_getInterpretationButton->setEnabled(true);
            }



            if (saveData.contains("birthInfo") && saveData["birthInfo"].isObject()) {
                QJsonObject birthInfo = saveData["birthInfo"].toObject();

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

            if (saveData.contains("interpretation") && saveData["interpretation"].isString()) {
                m_currentInterpretation = saveData["interpretation"].toString();
                displayInterpretation(m_currentInterpretation);
            }


            if (saveData.contains("relationshipInfo") && saveData["relationshipInfo"].isObject()) {
                m_currentRelationshipInfo = saveData["relationshipInfo"].toObject();

                if (m_currentRelationshipInfo.contains("displayName")) {
                    setWindowTitle("Asteria - Astrological Chart Analysis - " +
                                   m_currentRelationshipInfo["displayName"].toString());
                }
            } else {
                m_currentRelationshipInfo = QJsonObject();

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
#if defined(FLATHUB_BUILD) || defined(GENTOO_BUILD)
    QMessageBox::information(
                this,
                tr("Functionality Unavailable"),
                tr("This functionality is not available in the Flathub or Gentoo versions of Asteria.")
                );
    return;
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

    disconnect(this, &MainWindow::pdfExported, this, &MainWindow::printPdfFromPath); // prevent duplicates
    connect(this, &MainWindow::pdfExported, this, [this](const QString &path) {
        QTimer::singleShot(500, this, [this, path]() {
            printPdfFromPath(path);  // delayed to ensure PDF is ready
        });
    }, Qt::SingleShotConnection);

    exportAsPdf();
#endif
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
*/
/*
void MainWindow::showAboutDialog()
{
    QString version = QCoreApplication::applicationVersion();
    QMessageBox::about(
        this,
        "About Asteria",
        QString("Asteria - Astrological Chart Analysis\n\n"
                "Version %1\n\n"
                "A tool for calculating and interpreting astrological charts "
                "with AI-powered analysis.\n"
                "Available for Linux, Windows, Macos and Flatpak. \n"
                "https://github.com/alamahant/Asteria/releases/latest\n\n"
                "© 2025 Alamahant")
            .arg(version)
    );
}
*/
/*
void MainWindow::showAboutDialog()
{
    QString version = QCoreApplication::applicationVersion();
    QMessageBox::about(
                this,
                "About Asteria",
                QString("<h3>Asteria - Astrological Chart Analysis</h3>"
                        "<p>Version %1</p>"
                        "<p>A tool for calculating and interpreting astrological charts "
                        "with AI-powered analysis.</p>"
                        "<p>Available for Linux, Windows, Macos and Flatpak.</p>"
                        "<p><a href=\"https://github.com/alamahant/Asteria/releases/latest\">"
                        "https://github.com/alamahant/Asteria/releases/latest</a></p>"
                        "<p>© 2025 Alamahant</p>")
                .arg(version)
                );
}
*/


void MainWindow::showAboutDialog()
{
    QString version = QCoreApplication::applicationVersion();
    QMessageBox::about(
        this,
        "About Asteria",
        QString("<h3>Asteria - Astrological Chart Analysis</h3>"
                "<p>Version %1</p>"
                "<p>Free for Linux on Flathub.<br>"
                "Pre‑compiled binaries for <b>Windows & macOS</b> are available here:</p>"
                "<p><a href=\"https://jnanadhakini.gumroad.com/l/kwcxvj\">"
                "➡️ https://jnanadhakini.gumroad.com/l/kwcxvj</a></p>"
                "<p>View all Windows/macOS apps:<br>"
                "<a href=\"https://jnanadhakini.gumroad.com\">"
                "https://jnanadhakini.gumroad.com</a></p>"
                "<p>Source code & Linux version:<br>"
                "<a href=\"https://github.com/alamahant/Asteria\">"
                "https://github.com/alamahant/Asteria</a></p>"
                "<p>© 2025 Alamahant</p>")
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
    QString filePath;

    if (forSaving) {
        filePath = QFileDialog::getSaveFileName(this, "Save Chart",
                                                GlobalFlags::appDir, "Chart Files (*.chart)");
    } else {
        filePath = QFileDialog::getOpenFileName(this, "Open Chart",
                                                GlobalFlags::appDir, "Chart Files (*.chart)");
    }

    return filePath;
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());

    settings.setValue("chart/houseSystem", m_houseSystemCombo->currentText());

    settings.setValue("chart/utcOffset", m_utcOffsetCombo->currentText());

    settings.setValue("view/showInfoOverlay", m_showInfoOverlay);

    AspectSettings::instance().saveToSettings(settings);

}

void MainWindow::loadSettings()
{
    QSettings settings;


    /*
    if (settings.contains("mainWindow/geometry")) {
        restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    }

    if (settings.contains("mainWindow/windowState")) {
        restoreState(settings.value("mainWindow/windowState").toByteArray());
    }
    */

    if (settings.contains("chart/houseSystem")) {
        QString houseSystem = settings.value("chart/houseSystem").toString();
        int index = m_houseSystemCombo->findText(houseSystem);
        if (index >= 0) {
            m_houseSystemCombo->setCurrentIndex(index);
        }
    }

    if (settings.contains("chart/utcOffset")) {
        QString utcOffset = settings.value("chart/utcOffset").toString();
        int index = m_utcOffsetCombo->findText(utcOffset);
        if (index >= 0) {
            m_utcOffsetCombo->setCurrentIndex(index);
        }
    }

    if (settings.contains("view/showInfoOverlay")) {
        m_showInfoOverlay = settings.value("view/showInfoOverlay").toBool();
    } else {
        m_showInfoOverlay = false; // Default to false if setting doesn't exist
    }

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
    return QDate::currentDate();
}




void MainWindow::resizeEvent(QResizeEvent *event)
{


}

ChartData MainWindow::convertJsonToChartData(const QJsonObject &jsonData)
{
    ChartData chartData;

    if (jsonData.contains("planets") && jsonData["planets"].isArray()) {
        QJsonArray planetsArray = jsonData["planets"].toArray();
        for (const QJsonValue &value : planetsArray) {
            QJsonObject planetObj = value.toObject();
            PlanetData planet;
            planet.id = planetObj["id"].toString();
            planet.longitude = planetObj["longitude"].toDouble();
            planet.latitude = planetObj.contains("latitude") ? planetObj["latitude"].toDouble() : 0.0;
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


void MainWindow::getPrediction() {
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a birth chart first.");
        return;
    }

    if (!GlobalFlags::activeModelLoaded) {
           m_mistralApi.loadActiveModel();  // Try to reload once
           if (!GlobalFlags::activeModelLoaded) {
               QMessageBox::information(this, "AI Model Not Configured",
                   "No active AI model found. Please go to Settings → Configure AI Models to set up a model.");
               return;
           }
       }

    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();

    QDate fromDate = QDate::fromString(m_predictiveFromEdit->text(), "dd/MM/yyyy");
    QDate toDate = QDate::fromString(m_predictiveToEdit->text(), "dd/MM/yyyy");

    if (!fromDate.isValid() || !toDate.isValid()) {
        QMessageBox::warning(this, "Input Error", "Please enter valid dates for prediction range.");
        return;
    }

    if (fromDate > toDate) {
        QMessageBox::warning(this, "Input Error", "From date must be before To date.");
        return;
    }

    int transitDays = fromDate.daysTo(toDate) + 1;

    if (transitDays <= 0 || transitDays > 30) {
        QMessageBox::warning(this, "Input Error", "Prediction period must be between 1 and 30 days.");
        return;
    }

    m_interpretationtextEdit->setPlaceholderText("Calculating transits...");
    getPredictionButton->setEnabled(false);

    statusBar()->showMessage(QString("Calculating transits for %1 to %2...")
                             .arg(fromDate.toString("yyyy-MM-dd"))
                             .arg(toDate.toString("yyyy-MM-dd")));

    QJsonObject transitData = m_chartDataManager.calculateTransitsAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, fromDate, transitDays);

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayRawTransitData(transitData);

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

    QString htmlInterpretation = markdownToHtml(interpretation);

    QString existingHtml = m_interpretationtextEdit->toHtml();

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

    if (!m_currentChartData.isEmpty()) {
        if (m_currentChartData.contains("planets") && m_currentChartData["planets"].isArray()) {
            QJsonArray planets = m_currentChartData["planets"].toArray();

            for (const QJsonValue &planetValue : planets) {
                QJsonObject planet = planetValue.toObject();
                QString planetId = planet["id"].toString();

                if (planetId.toLower() == "sun") {
                    QString sunSign = planet["sign"].toString();
                    double sunDegree = planet["longitude"].toDouble();
                    m_sunSignLabel->setText(QString("Sun: %1").arg(sunSign));

                    break;
                }
            }
        }

        if (m_currentChartData.contains("angles") && m_currentChartData["angles"].isArray()) {
            QJsonArray angles = m_currentChartData["angles"].toArray();

            for (const QJsonValue &angleValue : angles) {
                QJsonObject angle = angleValue.toObject();
                QString angleId = angle["id"].toString();

                if (angleId.toLower() == "asc") {
                    QString ascSign = angle["sign"].toString();
                    double ascDegree = angle["longitude"].toDouble();
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

                QRegularExpression aspectRe("((?:North|South) Node|Pars Fortuna|Part of Spirit|East Point|\\w+)(?:\\s+\\(R\\))? (\\w+) ((?:North|South) Node|Pars Fortuna|Part of Spirit|East Point|\\w+(?:\\s+\\(R\\))?)\\( ([\\d.]+)°\\)");

                QRegularExpressionMatch aspectMatch = aspectRe.match(aspect);

                if (aspectMatch.hasMatch()) {
                    int row = rawTransitTable->rowCount();
                    rawTransitTable->insertRow(row);

                    QString transitPlanet = aspectMatch.captured(1);
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

    m_centralTabWidget->setCurrentIndex(0);

    QPixmap pixmap(m_chartView->scene()->sceneRect().size().toSize());
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    m_chartView->scene()->render(&painter);
    painter.end();

    if (pixmap.save(filePath)) {
        statusBar()->showMessage("Chart image exported to " + filePath, 3000);
    } else {
        QMessageBox::critical(this, "Export Error", "Could not save image to " + filePath);
    }
}

void MainWindow::exportAsPdf() {
#if defined(FLATHUB_BUILD) || defined(GENTOO_BUILD)
    QMessageBox::information(
                this,
                tr("Functionality Unavailable"),
                tr("This functionality is not available in the Flathub or Gentoo versions of Asteria.")
                );
    return;
#else
    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a chart first.");
        return;
    }
    
    QString filePath = getFilepath("pdf");
    if (filePath.isEmpty())
        return;
    
    QPixmap pixmap(m_chartView->size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    m_chartView->render(&painter);
    painter.end();
    
    const QString debugImagePath = QDir::tempPath() + "/chart_debug_render.png";
    if (!pixmap.save(debugImagePath)) {
        QMessageBox::critical(this, "Error", "Failed to save debug image. Check rendering.");
        return;
    }
    
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
    
    pdfPainter.setFont(titleFont);
    pdfPainter.drawText(QRect(0, margin, pageWidth, 70), Qt::AlignCenter, "Planets");
    
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

    QGraphicsScene* scene = m_chartView->scene();

    QTransform originalTransform = m_chartView->transform();

    m_chartView->resetTransform();
    m_chartView->scale(0.8, 0.8);

    QApplication::processEvents();

    QRectF sceneRect = scene->sceneRect();

    QSvgGenerator generator;
    generator.setFileName(filePath);
    generator.setSize(QSize(sceneRect.width(), sceneRect.height()));
    generator.setViewBox(sceneRect);
    generator.setTitle("Astrological Chart");
    generator.setDescription("Generated by Astrology Application");
    QPainter painter;
    painter.begin(&generator);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(sceneRect, Qt::white);
    scene->render(&painter, sceneRect, sceneRect);
    painter.end();
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
    QString appDir = GlobalFlags::appDir;

#ifdef FLATHUB_BUILD
#else

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

    if (!filePath.endsWith("." + format, Qt::CaseInsensitive))
        filePath += "." + format;

    return filePath;
}



void MainWindow::printPdfFromPath(const QString& filePath) {
#if defined(FLATHUB_BUILD) || defined(GENTOO_BUILD)
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

#if !defined(FLATHUB_BUILD) && !defined(GENTOO_BUILD)
void MainWindow::drawPage0(QPainter &painter, QPdfWriter &writer) {
    painter.save();
    const QRect pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    const int pageWidth = pageRect.width();
    const int margin = 50;
    QFont titleFont("Times", 20, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    QString title = "Asteria - Astrological Chart Generation and Analysis Tool";
    QRect titleRect(margin, margin, pageWidth - 2 * margin, 60);
    painter.drawText(titleRect, Qt::AlignCenter, title);
    QFont copyrightFont("Times", 10);
    painter.setFont(copyrightFont);
    int copyrightTop = margin + 100;
    QRect copyrightRect(margin, copyrightTop, pageWidth - 2 * margin, 30); // taller rect
    painter.drawText(copyrightRect, Qt::AlignCenter, "© 2025 Alamahant");
    int starTop = copyrightTop + 60;
    QRect starRect(pageWidth / 2 - 50, starTop, 100, 100);
    drawStarBanner(painter, starRect);
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
#if defined(FLATHUB_BUILD) || defined(GENTOO_BUILD)
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
    QString searchQuery = QString("coordinates of %1").arg(location);
    QString encodedQuery = QUrl::toPercentEncoding(searchQuery);
    QUrl url(QString("https://www.google.com/search?q=%1").arg(QString(encodedQuery)));

    QDesktopServices::openUrl(url);
    locationSearchEdit->clear();
#endif
}




void MainWindow::showSymbolsDialog()
{
    if (!m_symbolsDialog) {
        m_symbolsDialog = new SymbolsDialog(this, g_astroFontFamily);

        connect(m_symbolsDialog, &QDialog::finished, this, [this]() {
            m_symbolsDialog->deleteLater();
            m_symbolsDialog = nullptr;
        });
    }

    m_symbolsDialog->show();
    m_symbolsDialog->raise();
    m_symbolsDialog->activateWindow();
}


void MainWindow::showHowToUseDialog() {
    if (!m_howToUseDialog) {
        m_howToUseDialog = new QDialog(this);
        m_howToUseDialog->setWindowTitle("How to Use Asteria");
        m_howToUseDialog->setMinimumSize(500, 400);
        QVBoxLayout *layout = new QVBoxLayout(m_howToUseDialog);
        QTextBrowser *textBrowser = new QTextBrowser(m_howToUseDialog);
        textBrowser->setOpenExternalLinks(true);
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
        <h3>Synastry - Comparing Two Charts</h3>
        <p>Synastry compares two individual natal charts to analyze relationship dynamics between two people.</p>
        <ol>
            <li><b>Load Two Charts:</b> Go to File → Load Synastry Data and select two .astr natal chart files (one for Person A, one for Person B).</li>
            <li><b>View Synastry Table:</b> The Synastry tab in Chart Details will display all aspects between Person A and Person B's planets.</li>
            <li><b>Angles & House Overlays:</b> The interpretation text area shows both people's angles and house overlays (where each person's planets fall in the other's houses).</li>
            <li><b>Filter Synastry Aspects:</b> Use Tools → Synastry Filter to filter aspects by planet, aspect type, orb, or exclude specific terms.</li>
            <li><b>AI Interpretation:</b> Click "Get Birth Chart From AI" for a detailed synastry interpretation covering relationship dynamics, compatibility, strengths, and challenges.</li>
            <li><b>Save Synastry:</b> Use File → Save Synastry Data to save your synastry data (including AI interpretation). Auto-saves on calculation.</li>
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
                    <li>For synastry, load two charts and click "Get Birth Chart From AI" for relationship analysis</li>
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
            <li>Synastry data auto-saves to /SynastryCharts/ when calculated. Re-save anytime via File → Save Synastry Data.</li>
        </ul>
        <p>For accurate charts, ensure the birth time and location are as precise as possible. Use the "Astrological Symbols" reference to understand the chart's symbols. You can also save or print charts from the File menu.</p>
        <p>For more information about astrology and chart interpretation, consult astrological resources or books.</p>
        )";
        textBrowser->setHtml(helpText);
        layout->addWidget(textBrowser);
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_howToUseDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);
        connect(closeButton, &QPushButton::clicked, m_howToUseDialog, &QDialog::close);
        connect(m_howToUseDialog, &QDialog::finished, this, [this]() {
            m_howToUseDialog->deleteLater();
            m_howToUseDialog = nullptr;
        });
    }
    m_howToUseDialog->show();
    m_howToUseDialog->raise();
    m_howToUseDialog->activateWindow();
}


void MainWindow::onOpenMapClicked()
{
    OSMMapDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QGeoCoordinate coords = dialog.selectedCoordinates();

        QString latDir = (coords.latitude() >= 0) ? "N" : "S";
        QString longDir = (coords.longitude() >= 0) ? "E" : "W";

        m_googleCoordsEdit->setText(QString("%1° %2, %3° %4")
                                    .arg(qAbs(coords.latitude()), 0, 'f', 4)
                                    .arg(latDir)
                                    .arg(qAbs(coords.longitude()), 0, 'f', 4)
                                    .arg(longDir));

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
    OSMMapDialog *preloadDialog = new OSMMapDialog(this);
    preloadDialog->hide();  // Make sure it's hidden

    QTimer::singleShot(1000, [preloadDialog]() {
        preloadDialog->deleteLater();
    });
}

void MainWindow::showAspectSettings()
{
    AspectSettingsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        if (m_chartCalculated) {
            displayChart(m_currentChartData);
        }
    }
}




void MainWindow::createCompositeChart() {
    QMessageBox::information(this, "Select Charts",
                             "Please select two natal charts to create a composite chart.");
    QString appName = QApplication::applicationName();
    QString appDir = GlobalFlags::appDir;
#ifdef FLATHUB_BUILD
#else
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

    QJsonObject saveData1;
    QJsonObject saveData2;
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

    QJsonObject chartData1 = saveData1["chartData"].toObject();
    QJsonObject chartData2 = saveData2["chartData"].toObject();

    QJsonObject birthInfo1 = saveData1["birthInfo"].toObject();
    QJsonObject birthInfo2 = saveData2["birthInfo"].toObject();

    QString name1 = birthInfo1["firstName"].toString();
    QString name2 = birthInfo2["firstName"].toString();
    QString surname1 = birthInfo1["lastName"].toString();
    QString surname2 = birthInfo2["lastName"].toString();

    QJsonObject compositeChartData;

    QJsonArray planets1 = chartData1["planets"].toArray();
    QJsonArray planets2 = chartData2["planets"].toArray();
    QJsonArray compositePlanets;

    QMap<QString, QJsonObject> planetMap2;
    for (const QJsonValue &planetValue : planets2) {
        QJsonObject planet = planetValue.toObject();
        planetMap2[planet["id"].toString()] = planet;
    }

    for (const QJsonValue &planetValue1 : planets1) {
        QJsonObject planet1 = planetValue1.toObject();
        QString planetId = planet1["id"].toString();
        if (planetMap2.contains(planetId)) {
            QJsonObject planet2 = planetMap2[planetId];
            QJsonObject compositePlanet;
            compositePlanet["id"] = planetId;
            double long1 = planet1["longitude"].toDouble();
            double long2 = planet2["longitude"].toDouble();
            double diff = fmod(long2 - long1 + 540.0, 360.0) - 180.0;
            double midpoint = fmod(long1 + diff/2.0 + 360.0, 360.0);
            compositePlanet["longitude"] = midpoint;
            int signIndex = static_cast<int>(midpoint) / 30;
            QStringList signs = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
                                 "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
            compositePlanet["sign"] = signs[signIndex];
            if (planet1.contains("retrograde"))
                compositePlanet["retrograde"] = planet1["retrograde"];
            if (planet1.contains("house"))
                compositePlanet["house"] = planet1["house"];
            compositePlanets.append(compositePlanet);
        }
    }

    QJsonArray angles1 = chartData1["angles"].toArray();
    QJsonArray angles2 = chartData2["angles"].toArray();
    QJsonArray compositeAngles;

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

    QStringList angleIds = {"Asc", "MC", "Desc", "IC"};
    for (const QString &id : angleIds) {
        if (angleMap1.contains(id) && angleMap2.contains(id)) {
            QJsonObject angle1 = angleMap1[id];
            QJsonObject angle2 = angleMap2[id];
            double longitude1 = angle1["longitude"].toDouble();
            double longitude2 = angle2["longitude"].toDouble();
            double diff = fmod(longitude2 - longitude1 + 540.0, 360.0) - 180.0;
            double midpoint = fmod(longitude1 + diff/2.0 + 360.0, 360.0);
            int signIndex = static_cast<int>(midpoint) / 30;
            QStringList signs = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
                                 "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
            QString sign = signs[signIndex];
            QJsonObject compositeAngle;
            compositeAngle["id"] = id;
            compositeAngle["longitude"] = midpoint;
            compositeAngle["sign"] = sign;
            compositeAngles.append(compositeAngle);
        }
    }

    QJsonArray houses1 = chartData1["houses"].toArray();
    QJsonArray houses2 = chartData2["houses"].toArray();

    QMap<int, QJsonObject> houseMap1;
    QMap<int, QJsonObject> houseMap2;
    for (const QJsonValue &houseValue : houses1) {
        QJsonObject house = houseValue.toObject();
        QString id = house["id"].toString();
        int houseNumber = id.mid(5).toInt();
        if (houseNumber > 0 && houseNumber <= 12) {
            houseMap1[houseNumber] = house;
        }
    }
    for (const QJsonValue &houseValue : houses2) {
        QJsonObject house = houseValue.toObject();
        QString id = house["id"].toString();
        int houseNumber = id.mid(5).toInt();
        if (houseNumber > 0 && houseNumber <= 12) {
            houseMap2[houseNumber] = house;
        }
    }

    QJsonObject house1_1 = houseMap1[1];
    QJsonObject house1_2 = houseMap2[1];
    double longitude1_1 = house1_1["longitude"].toDouble();
    double longitude1_2 = house1_2["longitude"].toDouble();
    double diff1 = fmod(longitude1_2 - longitude1_1 + 540.0, 360.0) - 180.0;
    double compositeAsc = fmod(longitude1_1 + diff1/2.0 + 360.0, 360.0);

    QJsonArray compositeHouses;
    for (int i = 1; i <= 12; i++) {
        double houseLongitude = fmod(compositeAsc + (i-1) * 30.0, 360.0);

        int signIndex = static_cast<int>(houseLongitude) / 30;
        QStringList signs = {"Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
                             "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"};
        QString sign = signs[signIndex];

        QJsonObject compositeHouse;
        compositeHouse["id"] = QString("House%1").arg(i);
        compositeHouse["longitude"] = houseLongitude;
        compositeHouse["sign"] = sign;
        compositeHouses.append(compositeHouse);
    }

    for (int i = 0; i < compositeHouses.size(); i++) {
        QJsonObject house = compositeHouses[i].toObject();
    }


    compositeChartData["planets"] = compositePlanets;
    compositeChartData["angles"] = compositeAngles;
    compositeChartData["houses"] = compositeHouses;

    QJsonArray compositeAspects;
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

    for (int i = 0; i < compositePlanets.size(); i++) {
        QJsonObject planet1 = compositePlanets[i].toObject();
        for (int j = i + 1; j < compositePlanets.size(); j++) {
            QJsonObject planet2 = compositePlanets[j].toObject();
            double long1 = planet1["longitude"].toDouble();
            double long2 = planet2["longitude"].toDouble();
            double distance = fabs(long1 - long2);
            if (distance > 180.0) distance = 360.0 - distance;
            for (const AspectType &aspectType : aspectTypes) {
                double orb = fabs(distance - aspectType.angle);
                if (orb <= aspectType.orb) {
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

    compositeChartData["aspects"] = compositeAspects;

    QJsonObject compositeSaveData;
    compositeSaveData["chartData"] = compositeChartData;

    QJsonObject relationshipInfo;
    QString compositeFirstName = name1 + " " + surname1;
    QString compositeLastName = name2 + " " + surname2;
    relationshipInfo["type"] = "Composite";
    relationshipInfo["person1"] = name1 + " " + birthInfo1["lastName"].toString();
    relationshipInfo["person2"] = name2 + " " + birthInfo2["lastName"].toString();
    relationshipInfo["displayName"] = "Composite Chart: " + compositeFirstName + " & " + compositeLastName;
    m_currentRelationshipInfo = relationshipInfo;

    QDateTime dateTime1, dateTime2;
    QString dateStr1 = birthInfo1["date"].toString();
    QString timeStr1 = birthInfo1["time"].toString();
    QString dateStr2 = birthInfo2["date"].toString();
    QString timeStr2 = birthInfo2["time"].toString();

    if (dateStr1.contains("/")) {
        QDate date1 = QDate::fromString(dateStr1, "dd/MM/yyyy");
        QTime time1 = QTime::fromString(timeStr1, "HH:mm");
        dateTime1 = QDateTime(date1, time1);
    } else {
        dateTime1 = QDateTime::fromString(dateStr1 + " " + timeStr1, "yyyy-MM-dd HH:mm");
    }
    if (dateStr2.contains("/")) {
        QDate date2 = QDate::fromString(dateStr2, "dd/MM/yyyy");
        QTime time2 = QTime::fromString(timeStr2, "HH:mm");
        dateTime2 = QDateTime(date2, time2);
    } else {
        dateTime2 = QDateTime::fromString(dateStr2 + " " + timeStr2, "yyyy-MM-dd HH:mm");
    }

    qint64 timestamp1 = dateTime1.toSecsSinceEpoch();
    qint64 timestamp2 = dateTime2.toSecsSinceEpoch();
    qint64 midpointTimestamp = (timestamp1 + timestamp2) / 2;

    QDateTime midpointDateTime = QDateTime::fromSecsSinceEpoch(midpointTimestamp);

    QString compositeDateStr = midpointDateTime.toString("dd/MM/yyyy");
    QString compositeTimeStr = midpointDateTime.toString("HH:mm");

    relationshipInfo["date"] = compositeDateStr;
    relationshipInfo["time"] = compositeTimeStr;
    compositeSaveData["relationshipInfo"] = relationshipInfo;

    double lat1 = birthInfo1["latitude"].toString().toDouble();
    double lon1 = birthInfo1["longitude"].toString().toDouble();
    double lat2 = birthInfo2["latitude"].toString().toDouble();
    double lon2 = birthInfo2["longitude"].toString().toDouble();
    double midLat = (lat1 + lat2) / 2.0;
    double midLon = (lon1 + lon2) / 2.0;
    QString compositeLatStr = QString::number(midLat, 'f', 6);
    QString compositeLonStr = QString::number(midLon, 'f', 6);

    QString latDirection = midLat >= 0 ? "N" : "S";
    QString lonDirection = midLon >= 0 ? "E" : "W";
    QString compositeGoogleCoords = QString("%1° %2, %3° %4")
            .arg(fabs(midLat), 0, 'f', 4)
            .arg(latDirection)
            .arg(fabs(midLon), 0, 'f', 4)
            .arg(lonDirection);

    QJsonObject compositeBirthInfo;
    compositeBirthInfo["firstName"] = compositeFirstName;
    compositeBirthInfo["lastName"] = compositeLastName;
    compositeBirthInfo["date"] = compositeDateStr;  // Midpoint date
    compositeBirthInfo["time"] = compositeTimeStr;  // Midpoint time
    compositeBirthInfo["latitude"] = compositeLatStr;  // Midpoint latitude
    compositeBirthInfo["longitude"] = compositeLonStr;  // Midpoint longitude
    compositeBirthInfo["googleCoords"] = compositeGoogleCoords;  // Formatted Google coordinates

    QString compositeUtcOffsetStr = "+0:00";
    compositeBirthInfo["utcOffset"] = compositeUtcOffsetStr;
    int index = m_utcOffsetCombo->findText(compositeUtcOffsetStr);
    if (index >= 0) {
        m_utcOffsetCombo->setCurrentIndex(index);
    } else {
        index = m_utcOffsetCombo->findText("UTC+0", Qt::MatchContains);
        if (index >= 0) {
            m_utcOffsetCombo->setCurrentIndex(index);
        }
    }

    compositeBirthInfo["houseSystem"] = birthInfo1["houseSystem"].toString();  // Keep first person's house system
    compositeSaveData["birthInfo"] = compositeBirthInfo;

    first_name->setText(compositeFirstName);
    last_name->setText(compositeLastName);
    m_birthDateEdit->setText(compositeDateStr);
    m_birthTimeEdit->setText(compositeTimeStr);
    m_googleCoordsEdit->setText(compositeGoogleCoords);  // Use the formatted Google coordinates

    m_currentChartData = compositeChartData;
    displayChart(compositeChartData);
    m_chartCalculated = true;
    GlobalFlags::lastGeneratedChartType = "Composite Relationship";
    populateInfoOverlay();

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

    setWindowTitle("Asteria - Astrological Chart Analysis - " + relationshipInfo["displayName"].toString());
}




void MainWindow::createDavisonChart() {
    QMessageBox::information(this, "Select Charts",
                             "Please select two natal charts to create a Davison chart.");
    QString appName = QApplication::applicationName();
    QString appDir = GlobalFlags::appDir;
#ifdef FLATHUB_BUILD
#else
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

    QString dateStr1 = birthInfo1["date"].toString();
    QString timeStr1 = birthInfo1["time"].toString();
    QString dateStr2 = birthInfo2["date"].toString();
    QString timeStr2 = birthInfo2["time"].toString();

    QDateTime dateTime1 = QDateTime::fromString(dateStr1 + " " + timeStr1, "dd/MM/yyyy HH:mm:ss");
    QDateTime dateTime2 = QDateTime::fromString(dateStr2 + " " + timeStr2, "dd/MM/yyyy HH:mm:ss");

    if (!dateTime1.isValid()) {
        dateTime1 = QDateTime::fromString(dateStr1 + " " + timeStr1, "dd/MM/yyyy HH:mm");
    }
    if (!dateTime2.isValid()) {
        dateTime2 = QDateTime::fromString(dateStr2 + " " + timeStr2, "dd/MM/yyyy HH:mm");
    }

    qint64 midpointTimestamp = (dateTime1.toSecsSinceEpoch() + dateTime2.toSecsSinceEpoch()) / 2;
    QDateTime midpointDateTime = QDateTime::fromSecsSinceEpoch(midpointTimestamp);

    int hour1 = dateTime1.time().hour();
    int minute1 = dateTime1.time().minute();
    int hour2 = dateTime2.time().hour();
    int minute2 = dateTime2.time().minute();
    int totalMinutes1 = hour1 * 60 + minute1;
    int totalMinutes2 = hour2 * 60 + minute2;
    int midpointTotalMinutes = (totalMinutes1 + totalMinutes2) / 2;
    int midpointHour = midpointTotalMinutes / 60;
    int midpointMinute = midpointTotalMinutes % 60;

    midpointDateTime.setTime(QTime(midpointHour, midpointMinute));

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
    utcIndex = m_utcOffsetCombo->findText(midpointUtcOffsetStr);
    if (utcIndex < 0) {
        utcIndex = m_utcOffsetCombo->findText("UTC" + midpointUtcOffsetStr);
    }
    if (utcIndex < 0) {
        QString numericPart = midpointUtcOffsetStr.mid(1);
        for (int i = 0; i < m_utcOffsetCombo->count(); i++) {
            QString itemText = m_utcOffsetCombo->itemText(i);
            if (itemText.contains(numericPart)) {
                utcIndex = i;
                break;
            }
        }
    }
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
    if (utcIndex >= 0) {
        m_utcOffsetCombo->setCurrentIndex(utcIndex);
    } else {
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
    if (!m_relationshipChartsDialog) {
        m_relationshipChartsDialog = new QDialog(this);
        m_relationshipChartsDialog->setWindowTitle("About Relationship Charts");
        m_relationshipChartsDialog->setMinimumSize(500, 400);

        QVBoxLayout *layout = new QVBoxLayout(m_relationshipChartsDialog);

        QTextBrowser *textBrowser = new QTextBrowser(m_relationshipChartsDialog);
        textBrowser->setOpenExternalLinks(true);

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

        <h3>Synastry</h3>
        <p>Synastry compares two individual natal charts by examining the planetary aspects between them.</p>

        <p><b>How it's calculated:</b> Each planet in Person A's chart is compared to each planet in Person B's chart. Aspects (Conjunctions, Oppositions, Trines, Squares, Sextiles, etc.) are identified based on the angular distance between the planets.</p>

        <p><b>Purpose:</b> Synastry reveals the interaction dynamics between two people. It shows areas of harmony, tension, attraction, and challenge in the relationship.</p>

        <p><b>Insights offered:</b></p>
        <ul>
            <li>Emotional compatibility (Moon aspects)</li>
            <li>Romantic and sexual chemistry (Venus and Mars aspects)</li>
            <li>Communication styles (Mercury aspects)</li>
            <li>Power dynamics and growth opportunities (outer planet aspects)</li>
            <li>House overlays showing where each person influences the other's life areas</li>
        </ul>

        <p><b>Using Synastry in Asteria:</b></p>
        <ul>
            <li>Load two natal charts via File → Load Synastry Data</li>
            <li>View all aspects between the two charts in the Synastry table</li>
            <li>Angles and house overlays are displayed in the interpretation area</li>
            <li>Filter aspects using Tools → Synastry Filter</li>
            <li>Get AI interpretation for a detailed relationship analysis</li>
        </ul>

        <h3>Which Chart to Use?</h3>
        <p>Each tool offers unique insights:</p>
        <ul>
            <li><b>Composite:</b> Better for understanding the relationship's purpose and inherent dynamics</li>
            <li><b>Davison:</b> Better for timing events in the relationship and understanding its evolution</li>
            <li><b>Synastry:</b> Better for understanding the interaction dynamics between two individuals</li>
        </ul>

        <p>For a complete relationship analysis, it's beneficial to examine all three: Synastry, Composite, and Davison charts alongside each other.</p>
        )";

        textBrowser->setHtml(helpText);
        layout->addWidget(textBrowser);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_relationshipChartsDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        connect(closeButton, &QPushButton::clicked, m_relationshipChartsDialog, &QDialog::close);

        connect(m_relationshipChartsDialog, &QDialog::finished, this, [this]() {
            m_relationshipChartsDialog->deleteLater();
            m_relationshipChartsDialog = nullptr;
        });
    }

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
            if (saveData.contains("chartData") && saveData["chartData"].isObject()) {
                chartData = saveData["chartData"].toObject();
            }
            if (saveData.contains("birthInfo") && saveData["birthInfo"].isObject()) {
                chartData["birthInfo"] = saveData["birthInfo"].toObject();
            }
        }
    }

    return chartData;
}


void MainWindow::showChangelog(){

    if (!m_showChangelogDialog) {
        m_showChangelogDialog = new QDialog(this);
        m_showChangelogDialog->setWindowTitle("Changelog");
        m_showChangelogDialog->setMinimumSize(600, 500);

        QVBoxLayout *layout = new QVBoxLayout(m_showChangelogDialog);

        QTextBrowser *textBrowser = new QTextBrowser(m_showChangelogDialog);
        textBrowser->setOpenExternalLinks(true);

        QString changelogText = R"(

<h1>Changelog</h1>

<h2>Version 2.4.8 (2026-09-04) <span style='color:#2980b9;'>— Synastry & Relationship Analysis</span></h2>
<ul>
  <li><b>Synastry Chart Comparison:</b> Load two natal charts and compare planetary aspects between two people</li>
  <li><b>Synastry Table:</b> Display all aspects between Person A and Person B with orb values</li>
  <li><b>House Overlays:</b> View where each person's planets fall in the other's houses in the interpretation area</li>
<li><b>Synastry Filter:</b> Filter synastry aspects by planet, aspect type, orb, or exclude terms</li>
<li><b>Aspects Filter:</b> Filter natal aspects by planet, aspect type, orb, or exclude terms</li>
<li><b>Transit Filter:</b> Filter transit data by date, planet, aspect, or exclude terms</li>
<li>All three filters available via Tools menu for quick data exploration</li>
  <li><b>Save/Load Synastry:</b> Save synastry data including AI interpretations, auto-saves on calculation</li>
  <li><b>AI Synastry Interpretation:</b> Get detailed relationship analysis from Mistral AI</li>
  <li><b>Angles Display:</b> View Ascendant, MC, Descendant, and IC for both individuals</li>
</ul>

<h2>Version 2.4.7 (2026-06-14) <span style='color:#27ae60;'>— Chart Sharing & MSA Language Support</span></h2>
<ul>
  <li><b>Chart Sharing:</b> Capture chart screenshots with watermark and share via social dialog</li>
  <li><b>File Management:</b> Open Folder and Create Symlink actions with Flatpak permission handling</li>
  <li><b>MSA Support:</b> Added Modern Standard Arabic for AI chart interpretations</li>
  <li><b>Clipboard Integration:</b> Copy both chart image and text description simultaneously</li>
  <li><b>Symlink Creator:</b> Create home directory symlinks with overwrite confirmation and error handling</li>
  <li><b>Non-modal Sharing:</b> Share dialog stays open while continuing work on charts</li>
</ul>

<h2>Version 2.4.6 (2026-05-16) <span style='color:#27ae60;'>— Major Cleanup & Size Reduction</span></h2>
<ul>
  <li><b>Cleanup & Packaging:</b> Major Flatpak cleanup reducing overall size; removed dev files, headers, and build artifacts from runtime bundle</li>
  <li><b>Swiss Ephemeris:</b> Confirmed static integration; runtime now only depends on .se1 files</li>
  <li><b>Removed Unused Files:</b> Cleaned up sat/ and ep4/ ephemeris directories from final package</li>
  <li><b>Build Improvements:</b> Better separation between build-time and runtime assets in CMake/Flatpak</li>
  <li><b>Stability:</b> No functional changes to astrology calculations or AI features; fully backward compatible</li>
</ul>

<h2>Version 2.4.5 (2026-03-02) <span style='color:#27ae60;'>— Multi-Provider AI Model Selector</span></h2>
<ul>
  <li><b>Model Selector Dialog:</b> Add, edit, delete, and set active AI models via Settings menu</li>
  <li><b>Multi-Provider Support:</b> Works with Mistral, OpenAI, Groq, Ollama, and any OpenAI-compatible API</li>
  <li><b>Dynamic Configuration:</b> Endpoint, API key, model name, temperature, max tokens stored in QSettings</li>
  <li><b>API Refactor:</b> MistralAPI now provider-agnostic with loadActiveModel() method</li>
  <li><b>Status Check:</b> New "Check AI Model Status" option showing active model details with API key warnings</li>
  <li><b>Incompatible Notice:</b> Claude and Gemini not supported (different API formats)</li>
</ul>


<h2>Version 2.1.3 (2026-01-02) <span style='color:#27ae60;'>&mdash; Data Management & Organization Update</span></h2>
<ul>
  <li><b>Dedicated Documents Directory:</b> Created a dedicated <code>~/Documents/Asteria</code> directory where all charts and user data are now saved and loaded from for improved accessibility and organization.</li>
  <li><b>Data Migration Support:</b> Existing user data is not automatically migrated from the previous location. To manually migrate your charts and materials, please open a terminal and run:</li>
  <pre style="background: #f5f5f5; padding: 10px; border-radius: 5px; overflow-x: auto;">
cp -a ~/.var/app/io.github.alamahant.Asteria/data/Asteria/* ~/Documents/Asteria/</pre>
  <li><b>Improved File Management:</b> Simplified chart backup, sharing, and management through standard documents location.</li>
  <li><b>Cross-Platform Compatibility:</b> Ensures better compatibility with backup systems and cloud synchronization services.</li>
  <li><b>User Experience Enhancement:</b> More intuitive file access without navigating through hidden application directories.</li>
</ul>

<h2>Version 2.1.2 (2025-10-29) <span style='color:#27ae60;'>&mdash; Polish & Compatibility Update</span></h2>
<ul>
  <li><b>Updated KDE Runtime:</b> Upgraded to KDE Platform 6.9 for enhanced stability and performance across all systems.</li>
  <li><b>Improved Qt 6.9 Compatibility:</b> Optimized QTableWidget rendering and font handling for the latest Qt framework.</li>
  <li><b>Western Chart Convention:</b> Chart rendering now places the Ascendant at 9 o'clock position to align with standard Western astrological practices.</li>
  <li><b>Zodiac Sign Coloring:</b> Planet list widget now displays zodiac signs in their traditional corresponding colors for better visual recognition.</li>
  <li><b>Code Polish & Optimization:</b> Various code improvements and performance enhancements throughout the application.</li>
  <li><b>Enhanced Stability:</b> General bug fixes and maintenance improvements for smoother user experience.</li>
</ul>

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

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_showChangelogDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        connect(closeButton, &QPushButton::clicked, m_showChangelogDialog, &QDialog::close);

        connect(m_showChangelogDialog, &QDialog::finished, this, [this]() {
            m_showChangelogDialog->deleteLater();
            m_showChangelogDialog = nullptr;
        });
    }

    m_showChangelogDialog->show();
    m_showChangelogDialog->raise();
    m_showChangelogDialog->activateWindow();
}



void MainWindow::CalculateTransits() {

    if (!m_chartCalculated) {
        QMessageBox::warning(this, "No Chart", "Please calculate a birth chart first.");
        return;
    }

    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");
    QString utcOffset = m_utcOffsetCombo->currentText();
    QString latitude = m_latitudeEdit->text();
    QString longitude = m_longitudeEdit->text();

    QDate fromDate = QDate::fromString(m_predictiveFromEdit->text(), "dd/MM/yyyy");
    QDate toDate = QDate::fromString(m_predictiveToEdit->text(), "dd/MM/yyyy");


    QDate gregorianStart(1582, 10, 15);

    if (birthDate < gregorianStart || fromDate < gregorianStart || toDate < gregorianStart) {
        QMessageBox::warning(
                    this,
                    tr("Unsupported Date"),
                    tr("All dates (birth, start, and end) must be after 15 October 1582 (Gregorian calendar reform) for transit calculations.\n"
                       "Please enter valid post-Gregorian dates.")
                    );
        return;
    }

    if (!fromDate.isValid() || !toDate.isValid()) {
        QMessageBox::warning(this, "Input Error", "Please enter valid dates for prediction range.");
        return;
    }

    if (fromDate > toDate) {
        QMessageBox::warning(this, "Input Error", "From date must be before To date.");
        return;
    }

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


    statusBar()->showMessage(QString("Calculating transits for %1 to %2...")
                             .arg(fromDate.toString("yyyy-MM-dd"))
                             .arg(toDate.toString("yyyy-MM-dd")));

    this->setEnabled(false); // Disable all widgets in the main window

    QJsonObject transitData = m_chartDataManager.calculateTransitsAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, fromDate, transitDays);

    this->setEnabled(true); // Enable all widgets in the main window


    if (m_chartDataManager.getLastError().isEmpty()) {
        displayRawTransitData(transitData);
        QMessageBox::information(this, "Transit Data", "Transit data has been generated successfully.\n"
                                                       "Please Navigate to the 'Raw Transit Data Table' to view the data.\n"
                                                       "You may use 'Tools->Transit Filter' for advanced filtering.");


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
    if (m_transitSearchDialog && m_transitSearchDialog->statusLabel) {
        m_transitSearchDialog->statusLabel->setText("Please wait...");
        qApp->processEvents();
    }
    m_savedScrollPosition = rawTransitTable->verticalScrollBar()->value();
    m_savedSelection = rawTransitTable->selectionModel()->selection();

    int matchCount = 0;

    for(int row = 0; row < rawTransitTable->rowCount(); ++row) {
        bool match = true;

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
        if (match && !maxOrbPattern.isEmpty()) {
            bool ok = false;
            double maxOrb = maxOrbPattern.toDouble(&ok);
            if (ok) {
                QString natalText = rawTransitTable->item(row, 3)->text();
                QRegularExpression orbRegex("\\((\\d+(?:\\.\\d+)?)°\\)");
                QRegularExpressionMatch orbMatch = orbRegex.match(natalText);
                if (orbMatch.hasMatch()) {
                    double orbValue = orbMatch.captured(1).toDouble();
                    if (orbValue > maxOrb) {
                        match = false;
                    }
                }
            }
        }


        if(match && !excludePattern.isEmpty()) {
            QStringList excludeTerms = excludePattern.split(',', Qt::SkipEmptyParts);
            for(const QString &term : excludeTerms) {
                QString trimmedTerm = term.trimmed();
                if(!trimmedTerm.isEmpty()) {
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
   // m_transitSearchDialog->raise();
}

void MainWindow::openAspectsFilter() {
    if (!m_aspectSearchDialog) {
        m_aspectSearchDialog = new AspectSearchDialog(this);
        connect(m_aspectSearchDialog, &AspectSearchDialog::filterChanged,
                this, &MainWindow::applyAspectFilter);
    }
    m_aspectSearchDialog->show();
   // m_aspectSearchDialog->raise();

}

void MainWindow::openSynastryFilter() {
    if (!m_synastrySearchDialog) {
        m_synastrySearchDialog = new AspectSearchDialog(this);
        connect(m_synastrySearchDialog, &AspectSearchDialog::filterChanged,
                this, &MainWindow::applySynastryFilter);
    }
    m_synastrySearchDialog->show();

}


void MainWindow::exportChartData(){
    if (!m_chartDetailsWidget) {
        QMessageBox::warning(this, "No Chart Data", "Please generate a chart first.");
        return;
    }

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

        stream << "ASTROLOGICAL CHART DATA\n";
        stream << "=======================\n\n";

        for (int tabIndex = 0; tabIndex < detailsTabs->count(); ++tabIndex) {
            QWidget *tabWidget = detailsTabs->widget(tabIndex);
            QString tabName = detailsTabs->tabText(tabIndex).toUpper();
            QTableWidget *table = qobject_cast<QTableWidget*>(tabWidget);

            if (!table || table->rowCount() == 0) {
                continue; // Skip empty tables
            }

            stream << tabName << "\n";
            stream << QString("-").repeated(tabName.length()) << "\n";

            QStringList headers;
            for (int col = 0; col < table->columnCount(); ++col) {
                headers << table->horizontalHeaderItem(col)->text();
            }

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

            for (int col = 0; col < headers.size(); ++col) {
                stream << headers[col].leftJustified(columnWidths[col]);
            }
            stream << "\n";

            for (int col = 0; col < headers.size(); ++col) {
                stream << QString("-").repeated(headers[col].length()).leftJustified(columnWidths[col]);
            }
            stream << "\n";

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

    QDate fromDate = QDate::fromString(m_predictiveFromEdit->text(), "dd/MM/yyyy");
    QDate toDate = QDate::fromString(m_predictiveToEdit->text(), "dd/MM/yyyy");

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
    QTableWidget *eclipseTable = findChild<QTableWidget*>("Eclipses");
    if (!eclipseTable) return;

    eclipseTable->setRowCount(0); // Clear previous data

    for (const QJsonValue &val : eclipseData) {
        QJsonObject obj = val.toObject();
        int row = eclipseTable->rowCount();
        eclipseTable->insertRow(row);

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

    eclipseTable->sortItems(0);
}


void MainWindow::calculateSolarReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    bool ok = false;
    int year = QInputDialog::getInt(
                this,
                tr("Solar Return Year"),
                tr("Enter the year for the solar return:"),
                QDate::currentDate().year(), // default value
                1900, 2999, 1, &ok
                );
    if (!ok) {
        return;
    }

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
        doSolarReturnCalculation(birthDate, birthTime, year);
    });

    dialog->show();
}

void MainWindow::doSolarReturnCalculation(const QDate& birthDate, const QTime& birthTime, int year)
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
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info
    m_chartRenderer->scene()->clear();

    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateSolarReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, year
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;

        GlobalFlags::lastGeneratedChartType = "Solar Return";


        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);


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
        m_chartRenderer->scene()->clear();
    }
}

void MainWindow::calculateLunarReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    bool ok = false;
    int year = QInputDialog::getInt(
                this,
                tr("Lunar Return Year"),
                tr("Enter the year for the lunar return:"),
                QDate::currentDate().year(),
                1900, 2100, 1, &ok
                );
    if (!ok) {
        return;
    }

    int month = QInputDialog::getInt(
                this,
                tr("Lunar Return Month"),
                tr("Enter the month for the lunar return (1-12):"),
                QDate::currentDate().month(),
                1, 12, 1, &ok
                );
    if (!ok) {
        return;
    }

    QDate targetDate(year, month, 1);

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
        doLunarReturnCalculation(birthDate, birthTime, targetDate);
    });

    dialog->show();
}

void MainWindow::doLunarReturnCalculation(const QDate& birthDate, const QTime& birthTime, const QDate& targetDate)
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
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info
    m_chartRenderer->scene()->clear();

    QDate chartDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateLunarReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, targetDate
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Lunar Return";

        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);


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
        m_chartRenderer->scene()->clear();
    }
}


void MainWindow::calculateSaturnReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double saturnPeriod = 29.4571; // Saturn orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / saturnPeriod) + 1;

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
        doSaturnReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}



void MainWindow::doSaturnReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
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


    m_currentChartData = m_chartDataManager.calculateSaturnReturnAsJson(
                chartDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;
        GlobalFlags::lastGeneratedChartType = "Saturn Return";
        m_getInterpretationButton->setEnabled(true);
        getPredictionButton->setEnabled(true);
        getTransitsButton->setEnabled(true);


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
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double jupiterPeriod = 11.862; // Jupiter orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / jupiterPeriod) + 1;

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
        doJupiterReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}




void MainWindow::doJupiterReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
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

    m_currentChartData = m_chartDataManager.calculateJupiterReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
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


void MainWindow::calculateVenusReturn()
{
    QString dateText = m_birthDateEdit->text();

    if (!validateDateFormat(dateText, this)) {

        return;

    }
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double venusPeriod = 0.61519726; // Venus orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / venusPeriod) + 1;

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
        doVenusReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}

void MainWindow::doVenusReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
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

    m_currentChartData = m_chartDataManager.calculateVenusReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
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
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double marsPeriod = 1.8808476; // Mars orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / marsPeriod) + 1;

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
        doMarsReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}

void MainWindow::doMarsReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
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

    m_currentChartData = m_chartDataManager.calculateMarsReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
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
    QDate birthDate = getBirthDate();
    QTime birthTime = QTime::fromString(m_birthTimeEdit->text(), "HH:mm");

    double mercuryPeriod = 0.2408467; // Mercury orbital period in years
    int daysSinceBirth = birthDate.daysTo(QDate::currentDate());
    double yearsSinceBirth = daysSinceBirth / 365.25;
    int approxReturn = static_cast<int>(yearsSinceBirth / mercuryPeriod) + 1;

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
        doMercuryReturnCalculation(birthDate, birthTime, returnNumber);
    });

    dialog->show();
}

void MainWindow::doMercuryReturnCalculation(const QDate& birthDate, const QTime& birthTime, int returnNumber)
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

    m_currentChartData = m_chartDataManager.calculateMercuryReturnAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem, returnNumber
                );

    if (m_chartDataManager.getLastError().isEmpty()) {
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
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;
    int julianDay = day + ((153 * m + 2) / 5) + 365 * y + y / 4 - 32083;

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

    QDate progressedDate = birthDate.addYears(progressionYear);

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
    if (!m_showNewFeaturesDialog) {
        m_showNewFeaturesDialog = new QDialog(this);
        m_showNewFeaturesDialog->setWindowTitle("What's New!");
        m_showNewFeaturesDialog->setMinimumSize(650, 600);

        QVBoxLayout *layout = new QVBoxLayout(m_showNewFeaturesDialog);

        QTextBrowser *textBrowser = new QTextBrowser(m_showNewFeaturesDialog);
        textBrowser->setOpenExternalLinks(true);

        QString featuresText = R"(

<h1 style="color:#27ae60;">What's New in Version 2.4.5</h1>
<p><i>March 2, 2026</i></p>

<h2 style="color:#2980b9;">✨ Multi-Provider AI Model Selector</h2>
<p>
A complete overhaul of AI provider management! You can now configure and switch between multiple AI models directly from the Settings menu.
</p>
<ul>
<li><b>Model Selector Dialog</b>: Add, edit, delete, and set active AI models with ease</li>
<li><b>Persistent Storage</b>: All model configurations saved in <code>QSettings</code></li>
<li><b>Provider Support</b>: Works with Mistral, OpenAI (ChatGPT), Groq, Ollama (local), and any OpenAI-compatible API</li>
</ul>

<h2 style="color:#2980b9;">🔧 API Improvements</h2>
<ul>
<li>Refactored <code>MistralAPI</code> to be provider-agnostic with dynamic configuration loading</li>
<li>Temperature and max tokens now loaded from model settings instead of hardcoded values</li>
<li>Removed deprecated API key management methods</li>
</ul>

<h2 style="color:#2980b9;">🖥️ Enhanced User Experience</h2>
<ul>
<li><b>Check AI Model Status</b>: New menu option showing active model details with color-coded API key status</li>
<li><b>Smart Warnings</b>: Alerts when cloud providers are missing API keys</li>
<li><b>Automatic Reload</b>: Model configuration updates instantly when changed</li>
<li><b>Informational Tooltip</b>: Lists compatible providers (Mistral, OpenAI, Groq, Ollama) and incompatible ones (Claude, Gemini)</li>
</ul>

<h2 style="color:#2980b9;">📁 New Files Added</h2>
<ul>
<li><code>model.h</code> - Model struct definition</li>
<li><code>modelselectordialog.h/.cpp</code> - Model management interface</li>
</ul>

<div style="background-color:#f8f9fa; border-left:4px solid #27ae60; padding:10px; margin-top:20px;">
<p><b>Note:</b> If you've been using Mistral, you can continue with your existing API key or explore new providers like Groq (free tier) or Ollama (local models).</p>
</div>

<h1 style="color:#27ae60;">What’s New in Version 2.1.3</h1>
<p>
This update introduces a streamlined data management system with a dedicated documents directory, making your astrological charts and materials more accessible and easier to manage.
</p>

<h2>Dedicated Documents Directory</h2>
<p>
All charts and user data are now saved and loaded from <code>~/Documents/Asteria</code>, providing improved organization and accessibility outside of hidden application directories.
</p>

<h2>Data Migration Support</h2>
<p>
Your existing data can be easily migrated to the new location. To manually transfer your charts and materials, open a terminal and run:
</p>
<pre style="background: #f5f5f5; padding: 10px; border-radius: 5px; overflow-x: auto; margin: 10px 20px;">
cp -a ~/.var/app/io.github.alamahant.Asteria/data/Asteria/* ~/Documents/Asteria/
</pre>

<h1 style="color:#27ae60;">What’s New in Version 2.1.2</h1>
<p>
Welcome to the 2.1.2 update! This release brings platform upgrades, improved compatibility, and astrological refinements that enhance chart accuracy and visual clarity.
</p>

<h2>Platform & Runtime Upgrade</h2>
<p>
Updated to KDE Platform 6.9 runtime for enhanced stability and performance, ensuring smoother operation across all supported systems.
</p>

<h2>Qt 6.9 Compatibility</h2>
<p>
Optimized QTableWidget rendering and font handling for full compatibility with Qt 6.9, providing better text clarity and interface responsiveness.
</p>

<h2>Western Chart Convention</h2>
<p>
Chart rendering now places the Ascendant at the 9 o'clock position to align with standard Western astrological practices, ensuring traditional chart layout accuracy.
</p>

<h2>Zodiac Sign Coloring</h2>
<p>
Planet list widget now displays zodiac signs in their traditional corresponding colors, making sign identification more intuitive and visually consistent.
</p>

<h2>Code Improvements</h2>
<ul>
  <li>Various code polish and optimization throughout the application</li>
  <li>Enhanced stability and performance for smoother user experience</li>
  <li>General bug fixes and maintenance improvements</li>
</ul>


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

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *closeButton = new QPushButton("Close", m_showNewFeaturesDialog);
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);

        connect(closeButton, &QPushButton::clicked, m_showNewFeaturesDialog, &QDialog::close);

        connect(m_showNewFeaturesDialog, &QDialog::finished, this, [this]() {
            m_showNewFeaturesDialog->deleteLater();
            m_showNewFeaturesDialog = nullptr;
        });
    }

    m_showNewFeaturesDialog->show();
    m_showNewFeaturesDialog->raise();
    m_showNewFeaturesDialog->activateWindow();
}

void MainWindow::toggleChartOnlyView(bool chartOnly)
{
    if (chartOnly) {
        m_inputDock->hide();
        m_interpretationDock->hide();
        if (chartInfoOverlay) {
            chartInfoOverlay->setVisible(false);
        }
    } else {
        m_inputDock->show();
        m_interpretationDock->show();
        if (chartInfoOverlay && m_chartCalculated) {
            chartInfoOverlay->setVisible(m_showInfoOverlay);
        }
    }
}

/*
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_chartView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if ((mouseEvent->buttons() & Qt::LeftButton) &&
                (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
                m_dragStartPosition = mouseEvent->pos();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if ((mouseEvent->buttons() & Qt::LeftButton) &&
                (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
                if ((mouseEvent->pos() - m_dragStartPosition).manhattanLength()
                    >= QApplication::startDragDistance()) {
                    startChartDrag();
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
*/

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_chartView->viewport()) {
        bool ctrlPressed = (QApplication::keyboardModifiers() & Qt::ControlModifier);

        if (event->type() == QEvent::Wheel && ctrlPressed) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            QPoint numDegrees = wheelEvent->angleDelta() / 8;

            if (!numDegrees.isNull()) {
                qreal currentScale = m_chartView->transform().m11();
                qreal zoomFactor = numDegrees.y() > 0 ? 1.2 : 0.8;
                qreal newScale = currentScale * zoomFactor;

                if (newScale >= 0.1 && newScale <= 10.0) {
                    m_chartView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
                    m_chartView->scale(zoomFactor, zoomFactor);
                }
                return true;
            }
        }
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

    QJsonObject inputData;
    inputData["birthDate"] = m_birthDateEdit->text();
    inputData["birthTime"] = m_birthTimeEdit->text();
    inputData["utcOffset"] = m_utcOffsetCombo->currentText();
    inputData["latitude"] = m_latitudeEdit->text();
    inputData["longitude"] = m_longitudeEdit->text();
    inputData["houseSystem"] = m_houseSystemCombo->currentText();
    inputData["useJulian"] = useJulianForPre1582Action->isChecked();

    inputData["chartType"] = GlobalFlags::lastGeneratedChartType;

    if (m_interpretationtextEdit && !m_interpretationtextEdit->toPlainText().isEmpty()) {

        inputData["interpretationText"] = m_interpretationtextEdit->toPlainText();
    }

    inputData["chartData"] = m_currentChartData;

    QMimeData *mimeData = new QMimeData();
    QByteArray jsonData = QJsonDocument(inputData).toJson();
    mimeData->setData("application/x-astrological-input", jsonData);
    mimeData->setText("Astrological Chart Input Data");

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    drag->setPixmap(QIcon::fromTheme("x-office-calendar").pixmap(32, 32));

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
    m_birthDateEdit->setText(inputData["birthDate"].toString());
    m_birthTimeEdit->setText(inputData["birthTime"].toString());
    m_utcOffsetCombo->setCurrentText(inputData["utcOffset"].toString());
    m_latitudeEdit->setText(inputData["latitude"].toString());
    m_longitudeEdit->setText(inputData["longitude"].toString());
    m_houseSystemCombo->setCurrentText(inputData["houseSystem"].toString());
    useJulianForPre1582Action->setChecked(inputData["useJulian"].toBool());

    if (inputData.contains("chartType")) {
        GlobalFlags::lastGeneratedChartType = inputData["chartType"].toString();
    }

    if (inputData.contains("interpretationText") && m_interpretationtextEdit) {
        QString interpretation = inputData["interpretationText"].toString();
        m_interpretationtextEdit->setPlainText(interpretation);
    }

    QJsonObject chartData = inputData["chartData"].toObject();
    displayChart(chartData);

    m_chartCalculated = true;
    m_currentChartData = chartData;
    statusBar()->showMessage("Chart imported via drag & drop", 3000);
    setWindowTitle("Asteria - " + GlobalFlags::lastGeneratedChartType + " Chart");

}

QString MainWindow::markdownToHtml(const QString &markdown)
{
    QString html = markdown;

    html.replace(QRegularExpression("^###### (.*)$", QRegularExpression::MultilineOption), "<h6>\\1</h6>");
    html.replace(QRegularExpression("^##### (.*)$", QRegularExpression::MultilineOption), "<h5>\\1</h5>");
    html.replace(QRegularExpression("^#### (.*)$", QRegularExpression::MultilineOption), "<h4>\\1</h4>");
    html.replace(QRegularExpression("^### (.*)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.*)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.*)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");

    html.replace(QRegularExpression("\\*\\*(.*?)\\*\\*"), "<b>\\1</b>");

    html.replace(QRegularExpression("\\*(.*?)\\*"), "<i>\\1</i>");


    html.replace(QRegularExpression("^[\\s]*\\•[\\s]+(.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    html.replace(QRegularExpression("^[\\s]*\\-[\\s]+(.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    html.replace(QRegularExpression("^[\\s]*\\*[\\s]+(.*)$", QRegularExpression::MultilineOption), "• \\1<br>");

    html.replace(QRegularExpression("^\\s*(---|\\*\\*\\*|___)\\s*$", QRegularExpression::MultilineOption), "<hr>");

    html.replace("\n", "<br>");

    return html;
}



void MainWindow::calculateZodiacSignsChart()
{
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

    m_chartCalculated = false;
    m_currentChartData = QJsonObject();
    m_currentRelationshipInfo = QJsonObject(); // Reset relationship info

    m_chartRenderer->scene()->clear();

    birthDate = checkAndConvertJulian(birthDate, useJulianForPre1582Action->isChecked());

    m_currentChartData = m_chartDataManager.calculateChartAsJson(
                birthDate, birthTime, utcOffset, latitude, longitude, houseSystem);

    if (m_chartDataManager.getLastError().isEmpty()) {
        displayChart(m_currentChartData);
        m_chartCalculated = true;

        first_name->setText("no");
        last_name->setText("name");

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

        m_chartRenderer->scene()->clear();
    }
}

void MainWindow::copySavePath()
{
#ifdef FLATHUB_BUILD
    QString dataDirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/" + QApplication::applicationName();
#else
    QString dataDirPath = QDir::homePath() + "/" + QApplication::applicationName();

#endif

    QApplication::clipboard()->setText(dataDirPath);
    QMessageBox::information(this, "Path Copied",
                             QString("Save location copied to clipboard:\n%1").arg(dataDirPath));
}

void MainWindow::configureAIModels()
{
    ModelSelectorDialog dlg(this);

    connect(&dlg, &ModelSelectorDialog::activeModelChanged, this, [this](const QString &modelName) {
        m_mistralApi.loadActiveModel();

        statusBar()->showMessage(tr("Active model changed to: %1").arg(modelName), 3000);
    });

    dlg.exec();  // Just show the dialog, no need to process results

    m_mistralApi.loadActiveModel();

}


void MainWindow::setupShareButton()
{
    QPushButton *shareButton = new QPushButton(this);
    shareButton->setIcon(QIcon(":/icons/share-2.svg"));
    shareButton->setToolTip("Share this spread");
    shareButton->setFlat(true);
    shareButton->setFixedSize(32, 32);


    if (m_centralTabWidget) {
        m_centralTabWidget->setCornerWidget(shareButton, Qt::TopRightCorner);
    }

    connect(shareButton, &QPushButton::clicked, this, &MainWindow::onShareClicked);
}

void MainWindow::onShareClicked()
{
    QPixmap screenshot = m_chartView->grab();

    QPainter watermarkPainter(&screenshot);
    watermarkPainter.setPen(QPen(QColor(80, 80, 80, 200), 2));
    watermarkPainter.setFont(QFont("Arial", 20, QFont::Bold));
    watermarkPainter.drawText(screenshot.rect(), Qt::AlignBottom | Qt::AlignRight,
                              "  Created with Asteria  ");
    watermarkPainter.end();

    QString shareText = QString("My %1 chart")
                        .arg(GlobalFlags::lastGeneratedChartType);

    QClipboard *clipboard = QApplication::clipboard();
    QMimeData *mimeData = new QMimeData();
    mimeData->setText(shareText);
    mimeData->setImageData(screenshot);
    clipboard->setMimeData(mimeData);

    SocialShareDialog *dialog = new SocialShareDialog(shareText, screenshot, m_socialShare, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
    dialog->show();
}

void MainWindow::openFolder() {
    QDir dir(GlobalFlags::appDir);
    if (!dir.exists()) {
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(GlobalFlags::appDir))) {
    }
}

void MainWindow::createSymlink()
{
#ifdef FLATHUB_BUILD
    QString msg = "";
    QMessageBox msgBox;
    msgBox.setWindowTitle("Flatpak Permission Required");
    msgBox.setText(QString(
                       "%1 is running as a Flatpak and may not have access to your home directory.\n\n"
                       "To create a symlink, you may need to grant home directory access first.\n\n"
                       "Option 1 - Terminal:\n"
                       "  Grant access:\n"
                       "    flatpak override --user --filesystem=home io.github.alamahant.%1\n\n"
                       "  Revoke access later:\n"
                       "    flatpak override --user --nofilesystem=home io.github.alamahant.%1\n\n"
                       "Option 2 - Flatseal:\n"
                       "  Install Flatseal from Flathub and grant 'Home' access to %1.\n\n"
                       "If you have already granted permissions, you can continue."
                       ).arg(QApplication::applicationName()));

    msgBox.setIcon(QMessageBox::Information);

    QPushButton *continueButton = msgBox.addButton("Continue", QMessageBox::AcceptRole);
    QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.setDefaultButton(cancelButton);

    msgBox.exec();

    if (msgBox.clickedButton() != continueButton) {
        return; // User cancelled
    }

#endif
    QString destinationDir = QFileDialog::getExistingDirectory(
                this,
                "Select Destination Folder for Symlink",
                QDir::homePath(),
                QFileDialog::ShowDirsOnly
                );

    if (destinationDir.isEmpty()) {
        return; // User cancelled
    }

    QString symlinkPath = QDir(destinationDir).filePath(QApplication::applicationName());
    if (QFile::exists(symlinkPath) || QFileInfo(symlinkPath).isSymLink()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "Symlink Exists",
                    QString("A file or symlink already exists at:\n%1\n\nOverwrite?").arg(symlinkPath),
                    QMessageBox::Yes | QMessageBox::No
                    );

        if (reply != QMessageBox::Yes) {
            return;
        }

        if (!QFile::remove(symlinkPath)) {
            QMessageBox::warning(this, "Error", "Could not remove existing file/symlink");
            return;
        }
    }

    QString targetPath = GlobalFlags::appDir;

    if (!QFile::exists(targetPath)) {
        QMessageBox::warning(this, "Error",
                             QString("Target directory does not exist:\n%1").arg(targetPath));
        return;
    }

    if (QFile::link(targetPath, symlinkPath)) {
        QMessageBox::information(
                    this,
                    "Symlink Created",
                    QString("Symlink created successfully!\n\n"
                            "Name: %3\n"
                            "Location: %1\n\n"
                            "Now you can access Ermis data from:\n%2")
                    .arg(destinationDir)
                    .arg(symlinkPath)
                    .arg(QApplication::applicationName())
                    );
    } else {
        QMessageBox::warning(
                    this,
                    "Error",
                    QString("Failed to create symlink.\n\n"
                            "Destination: %1\n"
                            "Target: %2\n\n"
                            "Possible reasons:\n"
                            "• Insufficient permissions\n"
                            "• Invalid destination path\n"
                            "• Filesystem doesn't support symlinks")
                    .arg(symlinkPath)
                    .arg(targetPath)
                    );
    }
}

void MainWindow::onResetSettings()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Reset Settings",
        "Are you sure you want to reset all application settings?\n\n"
        "This will restore all preferences to their default values.\n\n"
        "Your personal data and files will NOT be affected.\n\n"
        "However any AI model configuration WILL be reset.\n\n"
        "This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QSettings settings;
        QString settingsFile = settings.fileName();
        settings.sync();

        if (QFile::remove(settingsFile)) {
            QMessageBox::information(
                this,
                "Settings Reset",
                "Settings file has been deleted successfully.\n\n"
                "The application will now restart with default settings."
            );
        } else {
            QMessageBox::warning(
                this,
                "Reset Failed",
                "Settings remained unchanged.\n\n"
                "Please check file permissions:\n" + settingsFile
            );
            return;
        }

        qApp->quit();
        QProcess::startDetached(qApp->applicationFilePath(), QStringList());
    }
}

void MainWindow::showAIConfigGuide()
{
    QDialog dialog(this);
    dialog.setWindowTitle("AI Interpretation Guide");
    dialog.resize(600, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTextEdit *textEdit = new QTextEdit(&dialog);
    textEdit->setReadOnly(true);
    textEdit->setHtml(
        "<h2>AI-Powered I-Ching Interpretations</h2>"

        "<p>The app can use AI models to provide rich, contextual interpretations of your divinations. "
        "You can connect to various AI providers by configuring them in the Model Selector.</p>"

        "<h3>Getting Started:</h3>"
        "<ol>"
        "<li><b>Open AI Model Selector:</b> Tools → AI Model Selector</li>"
        "<li><b>Add a new model:</b> Click 'Add' and fill in the details</li>"
        "<li><b>Set as active:</b> Select the model and click 'Set Active'</li>"
        "<li><b>Get interpretations:</b> Complete a hexagram and click 'Get AI Interpretation'</li>"
        "</ol>"

        "<h3>Compatible Providers (OpenAI-compatible API format):</h3>"
        "<table width='100%' border='1' cellpadding='5'>"
        "<tr><th>Provider</th><th>Endpoint</th><th>Example Model</th><th>API Key</th></tr>"

        "<tr><td><b>Groq</b></td>"
        "<td><code>https://api.groq.com/openai/v1/chat/completions</code></td>"
        "<td><code>openai/gpt-oss-120b</code></td>"
        "<td>gsk_... (free tier)</td></tr>"

        "<tr><td><b>Mistral</b></td>"
        "<td><code>https://api.mistral.ai/v1/chat/completions</code></td>"
        "<td><code>mistral-medium</code></td>"
        "<td>Free trial</td></tr>"


        "<tr><td><b>Gemini</b></td>"
        "<td><code>https://generativelanguage.googleapis.com/v1beta/openai/chat/completions</code></td>"
        "<td><code>gemini-3.5-flash</code></td>"
        "<td>Free trial</td></tr>"


        "<tr><td><b>OpenAI</b></td>"
        "<td><code>https://api.openai.com/v1/chat/completions</code></td>"
        "<td><code>gpt-4</code> or <code>gpt-3.5-turbo</code></td>"
        "<td>Paid access</td></tr>"

        "<tr><td><b>Ollama (local)</b></td>"
        "<td><code>http://localhost:11434/v1/chat/completions</code></td>"
        "<td><code>llama3</code> or <code>mistral</code></td>"
        "<td><i>None</i></td></tr>"

        "<tr><td><b>Together AI</b></td>"
        "<td><code>https://api.together.xyz/v1/chat/completions</code></td>"
        "<td><code>mistralai/Mixtral-8x7B-Instruct</code></td>"
        "<td>Required</td></tr>"

        "<tr><td><b>DeepSeek</b></td>"
        "<td><code>https://api.deepseek.com/v1/chat/completions</code></td>"
        "<td><code>deepseek-chat</code></td>"
        "<td>Required</td></tr>"
        "</table>"

        "<h3>Configuration Tips:</h3>"
        "<ul>"
        "<li><b>Friendly Name:</b> Any name to identify this config (e.g., 'My Groq Llama')</li>"
        "<li><b>Provider:</b> Just for reference (e.g., 'Groq', 'OpenAI')</li>"
        "<li><b>Endpoint URL:</b> The full API URL from the table above</li>"
        "<li><b>API Key:</b> Get from provider's website (except Ollama)</li>"
        "<li><b>Model Name:</b> The specific model identifier from the provider</li>"
        "<li><b>Temperature:</b> Keep at 0.7 for balanced interpretations</li>"
        "<li><b>Max Tokens:</b> 4096 is usually sufficient</li>"
        "</ul>"

        "<h3>Finding Model Names and Endpoints:</h3>"
        "<p>If you're unsure about which model to use or need the exact endpoint URL:</p>"
        "<ul>"
        "<li><b>Ask AI assistants</b> like ChatGPT, Claude: "
        "\"What's the API endpoint and available models for [Provider]?\"</li>"
        "<li><b>Check provider documentation</b> - most have clear API reference pages</li>"
        "<li><b>Search online:</b> '[Provider] API documentation'</li>"
        "</ul>"

        "<h3>Recommended Settings by Provider:</h3>"
        "<ul>"
        "<li><b>Groq:</b> <code>openai/gpt-oss-120b</code> for best quality</li>"
        "<li><b>Mistral:</b> <code>mistral-medium</code> works well</li>"
        "<li><b>Gemini(Google):</b> <code>gemini-3.5-flash</code> works well</li>"

        "<li><b>OpenAI:</b> <code>gpt-4</code> best results, <code>gpt-3.5-turbo</code> faster/cheaper</li>"
        "<li><b>Ollama:</b> Install Ollama first, then pull <code>llama3</code> or <code>mistral</code></li>"
        "</ul>"

        "<h3 style='color: #ff6b6b;'>Important Notes:</h3>"
        "<ul>"
        "<li><b>API keys are stored locally</b> in your system's secure settings</li>"
        "<li><b>Not compatible:</b> Claude (Anthropic) - different API formats</li>"
        "<li><b>Restart app</b> after configuring your first model</li>"
        "<li><b>Hexagram data and question</b> are sent to the configured AI service</li>"
        "</ul>"
    );

    layout->addWidget(textEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *openConfigButton = new QPushButton("Open Model Selector", &dialog);
    QPushButton *closeButton = new QPushButton("Close", &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(openConfigButton);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    connect(openConfigButton, &QPushButton::clicked, &dialog, [this, &dialog]() {
        dialog.accept();
        ModelSelectorDialog dlg(this);
        dlg.exec();
        //aiManager->loadActiveModel();
    });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    dialog.exec();
}

void MainWindow::applyAspectFilter(const QString &planet1Pattern,
                                    const QString &aspectPattern,
                                    const QString &planet2Pattern,
                                    const QString &maxOrbPattern,
                                    const QString &excludePattern)
{
    if (m_aspectSearchDialog && m_aspectSearchDialog->statusLabel) {
        m_aspectSearchDialog->statusLabel->setText("Please wait...");
        qApp->processEvents();
    }

    m_savedAspectScrollPosition = aspectsTable->verticalScrollBar()->value();
    m_savedAspectSelection = aspectsTable->selectionModel()->selection();

    int matchCount = 0;

    for(int row = 0; row < aspectsTable->rowCount(); ++row) {
        bool match = true;

        if(!planet1Pattern.isEmpty()) {
            match &= aspectsTable->item(row, 0)->text().contains(QRegularExpression(planet1Pattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !aspectPattern.isEmpty()) {
            match &= aspectsTable->item(row, 1)->text().contains(QRegularExpression(aspectPattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !planet2Pattern.isEmpty()) {
            match &= aspectsTable->item(row, 2)->text().contains(QRegularExpression(planet2Pattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !maxOrbPattern.isEmpty()) {
            bool ok = false;
            double maxOrb = maxOrbPattern.toDouble(&ok);
            if(ok) {
                QString orbText = aspectsTable->item(row, 3)->text();
                // Remove the degree symbol and any other non-numeric characters
                QString cleanOrb = orbText.remove("°").trimmed();
                double orbValue = cleanOrb.toDouble(&ok);
                if(ok && orbValue > maxOrb) {
                    match = false;
                }
            }
        }

        if(match && !excludePattern.isEmpty()) {
            QStringList excludeTerms = excludePattern.split(',', Qt::SkipEmptyParts);
            for(const QString &term : excludeTerms) {
                QString trimmedTerm = term.trimmed();
                if(!trimmedTerm.isEmpty()) {
                    bool containsExcludeTerm =
                            aspectsTable->item(row, 0)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            aspectsTable->item(row, 1)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            aspectsTable->item(row, 2)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            aspectsTable->item(row, 3)->text().contains(trimmedTerm, Qt::CaseInsensitive);

                    if(containsExcludeTerm) {
                        match = false;
                        break;
                    }
                }
            }
        }

        aspectsTable->setRowHidden(row, !match);
        if(match) matchCount++;
    }

    if(m_aspectSearchDialog && m_aspectSearchDialog->statusLabel) {
        m_aspectSearchDialog->statusLabel->setText("Filter applied");
    }
}


////////////////////////////

void MainWindow::loadSynastryCharts() {
    QString appDir = GlobalFlags::appDir;
    QDir dir;
    if (!dir.exists(appDir))
        dir.mkpath(appDir);

    QString filePathA = QFileDialog::getOpenFileName(this,
                                                     "Select Person A Chart",
                                                     appDir,
                                                     "Astrological Chart (*.astr)");
    if (filePathA.isEmpty())
        return;

    QFile fileA(filePathA);
    if (!fileA.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file: " + filePathA);
        return;
    }

    QJsonDocument docA = QJsonDocument::fromJson(fileA.readAll());
    fileA.close();

    if (!docA.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid chart file format");
        return;
    }

    m_synastryChartDataA = docA.object();

    // Extract name from birthInfo
    if (m_synastryChartDataA.contains("birthInfo")) {
        QJsonObject birthInfo = m_synastryChartDataA["birthInfo"].toObject();
        QString firstName = birthInfo["firstName"].toString();
        QString lastName = birthInfo["lastName"].toString();
        m_synastryNameA = firstName + " " + lastName;
    } else {
        m_synastryNameA = "Person A";
    }

    QString filePathB = QFileDialog::getOpenFileName(this,
                                                     "Select Person B Chart",
                                                     appDir,
                                                     "Astrological Chart (*.astr)");
    if (filePathB.isEmpty())
        return;

    QFile fileB(filePathB);
    if (!fileB.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file: " + filePathB);
        return;
    }

    QJsonDocument docB = QJsonDocument::fromJson(fileB.readAll());
    fileB.close();

    if (!docB.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid chart file format");
        return;
    }

    m_synastryChartDataB = docB.object();

    if (m_synastryChartDataB.contains("birthInfo")) {
        QJsonObject birthInfo = m_synastryChartDataB["birthInfo"].toObject();
        QString firstName = birthInfo["firstName"].toString();
        QString lastName = birthInfo["lastName"].toString();
        m_synastryNameB = firstName + " " + lastName;
    } else {
        m_synastryNameB = "Person B";
    }

    calculateSynastry();
    saveSynastry();

    // Switch to synastry tab
    // Switch to Chart Details tab first
    int detailsTabIndex = m_centralTabWidget->indexOf(m_chartDetailsWidget);
    if (detailsTabIndex >= 0) {
        m_centralTabWidget->setCurrentIndex(detailsTabIndex);
    }

    // Then switch to Synastry tab in detailsTabs
    int synastryTabIndex = detailsTabs->indexOf(m_synastryTable);
    if (synastryTabIndex >= 0) {
        detailsTabs->setCurrentIndex(synastryTabIndex);
    }
}

void MainWindow::calculateSynastry() {
    // Extract planets from chartData
    QJsonObject chartDataA = m_synastryChartDataA["chartData"].toObject();
    QJsonObject chartDataB = m_synastryChartDataB["chartData"].toObject();

    QJsonArray planetsArrayA = chartDataA["planets"].toArray();
    QJsonArray planetsArrayB = chartDataB["planets"].toArray();
    QJsonArray housesArrayA = chartDataA["houses"].toArray();
    QJsonArray housesArrayB = chartDataB["houses"].toArray();
    QJsonArray anglesArrayA = chartDataA["angles"].toArray();
    QJsonArray anglesArrayB = chartDataB["angles"].toArray();

    // Convert to PlanetData, HouseData, AngleData
    QVector<PlanetData> planetsA, planetsB;
    QVector<HouseData> housesA, housesB;
    QVector<AngleData> anglesA, anglesB;

    for (const QJsonValue &val : planetsArrayA) {
        QJsonObject obj = val.toObject();
        PlanetData planet;
        planet.id = obj["id"].toString();
        planet.longitude = obj["longitude"].toDouble();
        planet.sign = obj["sign"].toString();
        planet.house = obj["house"].toString();
        planet.isRetrograde = obj["isRetrograde"].toBool();
        planetsA.append(planet);
    }

    for (const QJsonValue &val : planetsArrayB) {
        QJsonObject obj = val.toObject();
        PlanetData planet;
        planet.id = obj["id"].toString();
        planet.longitude = obj["longitude"].toDouble();
        planet.sign = obj["sign"].toString();
        planet.house = obj["house"].toString();
        planet.isRetrograde = obj["isRetrograde"].toBool();
        planetsB.append(planet);
    }

    for (const QJsonValue &val : housesArrayA) {
        QJsonObject obj = val.toObject();
        HouseData house;
        house.id = obj["id"].toString();
        house.longitude = obj["longitude"].toDouble();
        house.sign = obj["sign"].toString();
        housesA.append(house);
    }

    for (const QJsonValue &val : housesArrayB) {
        QJsonObject obj = val.toObject();
        HouseData house;
        house.id = obj["id"].toString();
        house.longitude = obj["longitude"].toDouble();
        house.sign = obj["sign"].toString();
        housesB.append(house);
    }

    for (const QJsonValue &val : anglesArrayA) {
        QJsonObject obj = val.toObject();
        AngleData angle;
        angle.id = obj["id"].toString();
        angle.longitude = obj["longitude"].toDouble();
        angle.sign = obj["sign"].toString();
        anglesA.append(angle);
    }

    for (const QJsonValue &val : anglesArrayB) {
        QJsonObject obj = val.toObject();
        AngleData angle;
        angle.id = obj["id"].toString();
        angle.longitude = obj["longitude"].toDouble();
        angle.sign = obj["sign"].toString();
        anglesB.append(angle);
    }

    // Calculate synastry aspects
    QVector<AspectData> aspects = calculateSynastryAspects(planetsA, planetsB);

    // Display table
    displaySynastryTable(aspects, m_synastryNameA, m_synastryNameB,
                         planetsA, planetsB, housesA, housesB,
                         anglesA, anglesB);

    // Display text with angles and house overlays
    displaySynastryText(m_synastryNameA, m_synastryNameB,
                        anglesA, anglesB, planetsA, planetsB,
                        housesA, housesB);


    QJsonObject synastryChartData = createSynastryChartData(m_synastryNameA, m_synastryNameB,
                                                            planetsA, planetsB,
                                                            housesA, housesB,
                                                            anglesA, anglesB,
                                                            m_lastSynastryAspects);

    // Store for AI interpretation
    m_currentChartData = synastryChartData;
    GlobalFlags::lastGeneratedChartType = "Synastry";
    m_chartCalculated = true;
    m_getInterpretationButton->setEnabled(true);
}

void MainWindow::saveSynastry() {
    if (m_synastryTable->rowCount() == 0) {
        QMessageBox::warning(this, "No Data", "No synastry data to save. Please calculate a synastry first.");
        return;
    }

    QString appDir = GlobalFlags::appDir;
    QDir dir(appDir + "/SynastryCharts");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString nameA = m_synastryNameA;
    QString nameB = m_synastryNameB;
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString fileName = QString("Synastry-%1-%2-%3.astr")
                       .arg(nameA.replace(" ", "_"))
                       .arg(nameB.replace(" ", "_"))
                       .arg(timestamp);

    QString filePath = dir.absolutePath() + "/" + fileName;

    QJsonObject saveData;
    saveData["chartType"] = "Synastry";
    saveData["personA"] = m_synastryNameA;
    saveData["personB"] = m_synastryNameB;
    saveData["chartDataA"] = m_synastryChartDataA;
    saveData["chartDataB"] = m_synastryChartDataB;
    saveData["interpretation"] = m_interpretationtextEdit->toPlainText();

    // Save aspects from table
    QJsonArray aspectsArray;
    for (int row = 0; row < m_synastryTable->rowCount(); row++) {
        QJsonObject aspect;
        aspect["planetA"] = m_synastryTable->item(row, 0)->text();
        aspect["aspect"] = m_synastryTable->item(row, 1)->text();
        aspect["planetB"] = m_synastryTable->item(row, 2)->text();
        aspect["orb"] = m_synastryTable->item(row, 3)->text().replace("°", "");
        aspectsArray.append(aspect);
    }
    saveData["aspects"] = aspectsArray;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(saveData);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "Synastry Saved",
                                 "Synastry saved to:\n" + filePath +
                                 "\n\nYou can re-save at any time via File → Save Synastry Data.\n"
                                 "To load, use File → Load Synastry Data.");
    } else {
        QMessageBox::critical(this, "Save Error", "Could not save synastry to " + filePath);
    }
}


void MainWindow::loadSynastry() {
    QString appDir = GlobalFlags::appDir;
    QDir dir(appDir + "/SynastryCharts");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "Load Synastry",
                                                    appDir + "/SynastryCharts",
                                                    "Synastry Chart (*.astr)");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Could not open file");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        QMessageBox::critical(this, "Error", "Invalid file format");
        return;
    }

    QJsonObject saveData = doc.object();

    // Clear existing data
    m_synastryTable->setRowCount(0);
    m_interpretationtextEdit->clear();

    // Load chart data
    m_synastryChartDataA = saveData["chartDataA"].toObject();
    m_synastryChartDataB = saveData["chartDataB"].toObject();
    m_synastryNameA = saveData["personA"].toString();
    m_synastryNameB = saveData["personB"].toString();

    // Load interpretation
    if (saveData.contains("interpretation")) {
        m_interpretationtextEdit->setText(saveData["interpretation"].toString());
    }

    // Load aspects into table
    QJsonArray aspectsArray = saveData["aspects"].toArray();
    for (const QJsonValue &val : aspectsArray) {
        QJsonObject aspect = val.toObject();
        int row = m_synastryTable->rowCount();
        m_synastryTable->insertRow(row);
        m_synastryTable->setItem(row, 0, new QTableWidgetItem(aspect["planetA"].toString()));
        m_synastryTable->setItem(row, 1, new QTableWidgetItem(aspect["aspect"].toString()));
        m_synastryTable->setItem(row, 2, new QTableWidgetItem(aspect["planetB"].toString()));
        m_synastryTable->setItem(row, 3, new QTableWidgetItem(aspect["orb"].toString() + "°"));
    }

    // Switch to synastry tab
    // Switch to Chart Details tab first
    int detailsTabIndex = m_centralTabWidget->indexOf(m_chartDetailsWidget);
    if (detailsTabIndex >= 0) {
        m_centralTabWidget->setCurrentIndex(detailsTabIndex);
    }

    // Then switch to Synastry tab in detailsTabs
    int synastryTabIndex = detailsTabs->indexOf(m_synastryTable);
    if (synastryTabIndex >= 0) {
        detailsTabs->setCurrentIndex(synastryTabIndex);
    }

    statusBar()->showMessage("Synastry loaded from " + filePath, 3000);
}


void MainWindow::displaySynastryTable(const QVector<AspectData> &aspects, const QString &nameA, const QString &nameB, const QVector<PlanetData> &planetsA, const QVector<PlanetData> &planetsB, const QVector<HouseData> &housesA, const QVector<HouseData> &housesB, const QVector<AngleData> &anglesA, const QVector<AngleData> &anglesB)
{
    m_synastryTable->setRowCount(0);

    for (const AspectData &aspect : aspects) {
        int row = m_synastryTable->rowCount();
        m_synastryTable->insertRow(row);
        m_synastryTable->setItem(row, 0, new QTableWidgetItem(aspect.planet1));
        m_synastryTable->setItem(row, 1, new QTableWidgetItem(aspect.aspectType));
        m_synastryTable->setItem(row, 2, new QTableWidgetItem(aspect.planet2));
        m_synastryTable->setItem(row, 3, new QTableWidgetItem(QString::number(aspect.orb, 'f', 2) + "°"));
    }
}

void MainWindow::displaySynastryText(const QString &nameA, const QString &nameB, const QVector<AngleData> &anglesA, const QVector<AngleData> &anglesB, const QVector<PlanetData> &planetsA, const QVector<PlanetData> &planetsB, const QVector<HouseData> &housesA, const QVector<HouseData> &housesB)
{
    QString text;

    text += "=== SYNASTRY ANALYSIS ===\n\n";
    text += "Person A: " + nameA + "\n";
    text += "Person B: " + nameB + "\n\n";

    text += "=== PERSON A ANGLES ===\n";
    for (const AngleData &angle : anglesA) {
        text += angle.id + ": " + angle.sign + "\n";
    }
    text += "\n";

    text += "=== PERSON B ANGLES ===\n";
    for (const AngleData &angle : anglesB) {
        text += angle.id + ": " + angle.sign + "\n";
    }
    text += "\n";

    text += "=== HOUSE OVERLAYS ===\n";
    text += "Person A's Planets in Person B's Houses:\n";
    for (const PlanetData &planet : planetsA) {
        //QString houseId = findHouse(planet.longitude, housesB);
        QString houseId = m_chartDataManager.calculator()->findHouse(planet.longitude, housesB);

        QString houseSign = "";
        for (const HouseData &h : housesB) {
            if (h.id == houseId) {
                houseSign = h.sign;
                break;
            }
        }
        text += "• " + planet.id + " in " + houseId + " (" + houseSign + ")\n";
    }
    text += "\n";

    text += "Person B's Planets in Person A's Houses:\n";
    for (const PlanetData &planet : planetsB) {
        //QString houseId = findHouse(planet.longitude, housesA);
        QString houseId = m_chartDataManager.calculator()->findHouse(planet.longitude, housesB);
        QString houseSign = "";
        for (const HouseData &h : housesA) {
            if (h.id == houseId) {
                houseSign = h.sign;
                break;
            }
        }
        text += "• " + planet.id + " in " + houseId + " (" + houseSign + ")\n";
    }

    m_interpretationtextEdit->setText(text);
}


QVector<AspectData> MainWindow::calculateSynastryAspects(const QVector<PlanetData> &planetsA,
                                                          const QVector<PlanetData> &planetsB) {
    QVector<AspectData> aspects;

    struct AspectType {
        QString name;
        double angle;
        double orb;
    };

    double orbMax = getOrbMax();
    const AspectType aspectTypes[] = {
        {"CON", 0.0, orbMax},
        {"OPP", 180.0, orbMax},
        {"TRI", 120.0, orbMax},
        {"SQR", 90.0, orbMax},
        {"SEX", 60.0, orbMax},
        {"QUI", 150.0, orbMax * 0.75},
        {"SSQ", 45.0, orbMax * 0.75},
        {"SQQ", 135.0, orbMax * 0.75},
        {"SSX", 30.0, orbMax * 0.75}
    };

    for (const PlanetData &planetA : planetsA) {
        for (const PlanetData &planetB : planetsB) {
            double diff = fabs(planetA.longitude - planetB.longitude);
            if (diff > 180.0) diff = 360.0 - diff;

            for (const AspectType &aspectType : aspectTypes) {
                double orb = fabs(diff - aspectType.angle);
                if (orb <= aspectType.orb) {
                    AspectData aspect;
                    aspect.planet1 = planetA.id;
                    aspect.planet2 = planetB.id;
                    aspect.aspectType = aspectType.name;
                    aspect.orb = orb;
                    aspects.append(aspect);
                    break;
                }
            }
        }
    }

    return aspects;
}


QJsonObject MainWindow::createSynastryChartData(const QString &nameA,
                                                 const QString &nameB,
                                                 const QVector<PlanetData> &planetsA,
                                                 const QVector<PlanetData> &planetsB,
                                                 const QVector<HouseData> &housesA,
                                                 const QVector<HouseData> &housesB,
                                                 const QVector<AngleData> &anglesA,
                                                 const QVector<AngleData> &anglesB,
                                                 const QVector<AspectData> &aspects)
{
    QJsonObject chartData;
    chartData["chartType"] = "Synastry";

    // Person A
    QJsonObject personA;
    QJsonArray planetsArrayA;
    for (const PlanetData &p : planetsA) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["longitude"] = p.longitude;
        obj["sign"] = p.sign;
        obj["house"] = p.house;
        obj["isRetrograde"] = p.isRetrograde;
        planetsArrayA.append(obj);
    }
    personA["name"] = nameA;
    personA["planets"] = planetsArrayA;

    QJsonArray housesArrayA;
    for (const HouseData &h : housesA) {
        QJsonObject obj;
        obj["id"] = h.id;
        obj["longitude"] = h.longitude;
        obj["sign"] = h.sign;
        housesArrayA.append(obj);
    }
    personA["houses"] = housesArrayA;

    QJsonArray anglesArrayA;
    for (const AngleData &a : anglesA) {
        QJsonObject obj;
        obj["id"] = a.id;
        obj["longitude"] = a.longitude;
        obj["sign"] = a.sign;
        anglesArrayA.append(obj);
    }
    personA["angles"] = anglesArrayA;

    // Person B
    QJsonObject personB;
    QJsonArray planetsArrayB;
    for (const PlanetData &p : planetsB) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["longitude"] = p.longitude;
        obj["sign"] = p.sign;
        obj["house"] = p.house;
        obj["isRetrograde"] = p.isRetrograde;
        planetsArrayB.append(obj);
    }
    personB["name"] = nameB;
    personB["planets"] = planetsArrayB;

    QJsonArray housesArrayB;
    for (const HouseData &h : housesB) {
        QJsonObject obj;
        obj["id"] = h.id;
        obj["longitude"] = h.longitude;
        obj["sign"] = h.sign;
        housesArrayB.append(obj);
    }
    personB["houses"] = housesArrayB;

    QJsonArray anglesArrayB;
    for (const AngleData &a : anglesB) {
        QJsonObject obj;
        obj["id"] = a.id;
        obj["longitude"] = a.longitude;
        obj["sign"] = a.sign;
        anglesArrayB.append(obj);
    }
    personB["angles"] = anglesArrayB;

    // Synastry aspects
    QJsonArray aspectsArray;
    for (const AspectData &a : aspects) {
        QJsonObject obj;
        obj["planetA"] = a.planet1;
        obj["aspect"] = a.aspectType;
        obj["planetB"] = a.planet2;
        obj["orb"] = a.orb;
        aspectsArray.append(obj);
    }

    // House overlays
    QJsonArray houseOverlaysAtoB;
    for (const PlanetData &p : planetsA) {
        QJsonObject obj;
        obj["planet"] = p.id;
        obj["houseB"] = m_chartDataManager.calculator()->findHouse(p.longitude, housesB);
        houseOverlaysAtoB.append(obj);
    }

    QJsonArray houseOverlaysBtoA;
    for (const PlanetData &p : planetsB) {
        QJsonObject obj;
        obj["planet"] = p.id;
        obj["houseA"] =  m_chartDataManager.calculator()->findHouse(p.longitude, housesA);
        houseOverlaysBtoA.append(obj);
    }

    QJsonObject synastry;
    synastry["aspects"] = aspectsArray;
    synastry["houseOverlaysAtoB"] = houseOverlaysAtoB;
    synastry["houseOverlaysBtoA"] = houseOverlaysBtoA;

    chartData["personA"] = personA;
    chartData["personB"] = personB;
    chartData["synastry"] = synastry;

    return chartData;
}


void MainWindow::applySynastryFilter(const QString &planet1Pattern,
                                      const QString &aspectPattern,
                                      const QString &planet2Pattern,
                                      const QString &maxOrbPattern,
                                      const QString &excludePattern)
{
    if (m_aspectSearchDialog && m_aspectSearchDialog->statusLabel) {
        m_aspectSearchDialog->statusLabel->setText("Please wait...");
        qApp->processEvents();
    }

    int matchCount = 0;

    for(int row = 0; row < m_synastryTable->rowCount(); ++row) {
        bool match = true;

        if(!planet1Pattern.isEmpty()) {
            match &= m_synastryTable->item(row, 0)->text().contains(QRegularExpression(planet1Pattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !aspectPattern.isEmpty()) {
            match &= m_synastryTable->item(row, 1)->text().contains(QRegularExpression(aspectPattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !planet2Pattern.isEmpty()) {
            match &= m_synastryTable->item(row, 2)->text().contains(QRegularExpression(planet2Pattern, QRegularExpression::CaseInsensitiveOption));
        }
        if(match && !maxOrbPattern.isEmpty()) {
            bool ok = false;
            double maxOrb = maxOrbPattern.toDouble(&ok);
            if(ok) {
                QString orbText = m_synastryTable->item(row, 3)->text();
                QString cleanOrb = orbText.remove("°").trimmed();
                double orbValue = cleanOrb.toDouble(&ok);
                if(ok && orbValue > maxOrb) {
                    match = false;
                }
            }
        }

        if(match && !excludePattern.isEmpty()) {
            QStringList excludeTerms = excludePattern.split(',', Qt::SkipEmptyParts);
            for(const QString &term : excludeTerms) {
                QString trimmedTerm = term.trimmed();
                if(!trimmedTerm.isEmpty()) {
                    bool containsExcludeTerm =
                            m_synastryTable->item(row, 0)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            m_synastryTable->item(row, 1)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            m_synastryTable->item(row, 2)->text().contains(trimmedTerm, Qt::CaseInsensitive) ||
                            m_synastryTable->item(row, 3)->text().contains(trimmedTerm, Qt::CaseInsensitive);

                    if(containsExcludeTerm) {
                        match = false;
                        break;
                    }
                }
            }
        }

        m_synastryTable->setRowHidden(row, !match);
        if(match) matchCount++;
    }

    if(m_aspectSearchDialog && m_aspectSearchDialog->statusLabel) {
        m_aspectSearchDialog->statusLabel->setText("Filter applied");
    }
}

