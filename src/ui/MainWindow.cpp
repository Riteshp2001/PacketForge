#include "MainWindow.h"
#include "ui_MainWindow.h" 

// Network Classes
#include "SerialQTClass.h"
#include "TcpClientClass.h"
#include "TcpServer_SingleClientClass.h"
#include "UdpClass.h"
#include "Debugger.h"

#define BASEUI_DEBUG true

// Macros

// Qt Includes
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QScrollBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QSettings>
#include <QCloseEvent>
#include <QButtonGroup>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
    , m_handler(nullptr)
    , m_autoSendTimer(new QTimer(this))
    , isConnected(false)
    , m_isDarkTheme(true)
{
    m_autoSendTimer->setTimerType(Qt::PreciseTimer);
    ui->setupUi(this);

    // Standard Window
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    resize(1100, 750);

    // Load Persistence (Size & State)
    QSettings settings("PacketForge", "PacketTransmitter");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }

    // Connect auto-send timer
    connect(m_autoSendTimer, &QTimer::timeout, this, &MainWindow::onAutoSendTimerTimeout);

    ui->grpSerial->setEnabled(true);
    ui->grpNetwork->setEnabled(true);

    // Initial Setup
    QIcon appIcon(":/Images/app_icon.png");
    setWindowIcon(appIcon);
    
    setupUiDefaults();
    applyDarkTheme();

    // Connect Theme Button
    QPushButton *btnTheme = this->findChild<QPushButton*>("btnTheme");
    if (btnTheme) {
        connect(btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    }
    
    // Connect Enter Key on Payload to Send mechanism
    connect(ui->txtPayload, &QLineEdit::returnPressed, this, &MainWindow::on_btnSend_clicked);

    // Hardware Pins Logic
    connect(ui->chkDTR, &QCheckBox::toggled, [this](bool checked){
        if(m_handler && isConnected) m_handler->setDtr(checked);
    });
    connect(ui->chkRTS, &QCheckBox::toggled, [this](bool checked){
        if(m_handler && isConnected) m_handler->setRts(checked);
    });

    QTimer *pinPollingTimer = new QTimer(this);
    connect(pinPollingTimer, &QTimer::timeout, [this](){
        if(isConnected && m_handler) {
            int pins = m_handler->getPinStatus();
            // QSerialPort::PinoutSignals: NoSignal=0, Tx=1, Rx=2, Dsr=4, Dcd=8, Ring=16, Cts=32 
            // Mapping: 4=DSR, 8=CD, 16=RI, 32=CTS
            
            ui->lblDSR->setStyleSheet( (pins & 4) ? "color: #00FF00; font-weight: bold;" : "color: gray; font-weight: bold;");
            ui->lblCD->setStyleSheet( (pins & 8) ? "color: #00FF00; font-weight: bold;" : "color: gray; font-weight: bold;");
            ui->lblRI->setStyleSheet( (pins & 16) ? "color: #00FF00; font-weight: bold;" : "color: gray; font-weight: bold;");
            ui->lblCTS->setStyleSheet( (pins & 32) ? "color: #00FF00; font-weight: bold;" : "color: gray; font-weight: bold;");
        } else {
             // Reset to gray
            ui->lblDSR->setStyleSheet("color: gray; font-weight: bold;");
            ui->lblCD->setStyleSheet("color: gray; font-weight: bold;");
            ui->lblRI->setStyleSheet("color: gray; font-weight: bold;");
            ui->lblCTS->setStyleSheet("color: gray; font-weight: bold;");
        }
    });
    pinPollingTimer->start(200);

    // Connect Macro Buttons
    // Connect Macro Buttons (Send and Config)
    for (int i = 1; i <= 12; ++i) {
        QString sendName = QString("btnM%1").arg(i);
        QString cfgName = QString("btnM%1_Cfg").arg(i);

        QPushButton* btnSend = this->findChild<QPushButton*>(sendName);
        QPushButton* btnCfg = this->findChild<QPushButton*>(cfgName);

        if (btnSend && btnCfg) {
            // Styling for "Merged" look
            btnSend->setStyleSheet("QPushButton { border-top-right-radius: 0px; border-bottom-right-radius: 0px; border-right: 0px; margin-right: 0px; }");
            btnCfg->setStyleSheet("QPushButton { border-top-left-radius: 0px; border-bottom-left-radius: 0px; margin-left: 0px; }");
            btnCfg->setMaximumWidth(30);
            btnCfg->setText("...");

            // Connect Send
            connect(btnSend, &QPushButton::clicked, [this, i]() {
                BASEUI_DEBUG && DEBUG("Macro Send Clicked: " << i);
                onMacroClicked(i);
            });

            // Connect Config
            connect(btnCfg, &QPushButton::clicked, [this, i]() {
                BASEUI_DEBUG && DEBUG("Macro Config Clicked: " << i);
                configureMacro(i);
            });

            // Load Settings (Set Text for Send Button)
            QSettings settings("PacketForge", "PacketTransmitter");
            settings.beginGroup("Macro" + QString::number(i));
            MacroSettings ms;
            ms.name = settings.value("name", "M" + QString::number(i)).toString();
            ms.data = settings.value("data", "").toString();
            ms.autoSend = settings.value("autoSend", false).toBool();
            ms.intervalMs = settings.value("intervalMs", 1000).toInt();
            ms.packetMode = settings.value("packetMode", 1).toInt();
            ms.sof = settings.value("sof", "").toString();
            ms.eof = settings.value("eof", "").toString();
            settings.endGroup();
            
            m_macros[i] = ms;
            if (!ms.name.isEmpty()) btnSend->setText(ms.name);
        }
    }
    
    // Connect Traffic Table Double Click
    connect(ui->tablePackets, &QTableWidget::cellDoubleClicked, this, &MainWindow::onTableDoubleClicked);
}

MainWindow::~MainWindow()
{
    if (m_handler) delete m_handler;
    delete ui;
}

void MainWindow::setupUiDefaults()
{
    // Populate Serial Ports
    refreshSerialPorts();

    // Populate Baud Rates
    ui->comboBaudRate->clear();
    QStringList bauds = {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"};
    for(const QString &b : bauds) ui->comboBaudRate->addItem(b, b.toInt());
    ui->comboBaudRate->setCurrentText("115200");
    ui->comboBaudRate->setEditable(true); // Allow custom baud rates

    // Init Counters
    updateCounters(0, 0);
    
    // Init Table
    ui->tablePackets->setColumnWidth(0, 100); // Time
    ui->tablePackets->setColumnWidth(1, 50);  // Dir
    ui->tablePackets->setColumnWidth(2, 400); // Hex
    ui->tablePackets->horizontalHeader()->setStretchLastSection(true); // ASCII
    ui->tablePackets->setEditTriggers(QAbstractItemView::NoEditTriggers); // Read Only
    ui->tablePackets->setSelectionMode(QAbstractItemView::NoSelection); // No Selection
    ui->tablePackets->setFocusPolicy(Qt::NoFocus); // No Focus


    // Populate Serial Parameters
    // Data Bits
    ui->comboDataBits->addItem("5", 5);
    ui->comboDataBits->addItem("6", 6);
    ui->comboDataBits->addItem("7", 7);
    ui->comboDataBits->addItem("8", 8);
    ui->comboDataBits->setCurrentIndex(3); // Default 8

    // Parity
    ui->comboParity->addItem("None", 0);
    ui->comboParity->addItem("Even", 2);
    ui->comboParity->addItem("Odd", 3);
    ui->comboParity->addItem("Space", 4);
    ui->comboParity->addItem("Mark", 5);
    ui->comboParity->setCurrentIndex(0); // Default None

    // Stop Bits
    ui->comboStopBits->addItem("1", 1);
    ui->comboStopBits->addItem("1.5", 3);
    ui->comboStopBits->addItem("2", 2);
    ui->comboStopBits->setCurrentIndex(0); // Default 1

    // Flow Control
    ui->comboFlowControl->addItem("Hardware (RTS/CTS)", 1);
    ui->comboFlowControl->addItem("Software (XON/XOFF)", 2);
    ui->comboFlowControl->setCurrentIndex(0); // Default None

    // Set Interval Range to allow 1ms
    ui->spinInterval->setRange(OFFSET_ONE, MAX_INTERVAL_MS);
}

void MainWindow::refreshSerialPorts()
{
    QString currentPort = ui->comboPort->currentData().toString(); // Use stored data
    ui->comboPort->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ui->comboPort->addItem("🔌 " + info.portName(), info.portName());
    }
    
    // Restore selection if still available
    int idx = ui->comboPort->findData(currentPort);
    if(idx != -1) ui->comboPort->setCurrentIndex(idx);
}

void MainWindow::toggleTheme()
{
    m_isDarkTheme = !m_isDarkTheme;
    
    if (m_isDarkTheme) {
        applyDarkTheme();
    } else {
        // Light Theme
        QString lightStyle = R"(
        /* Main Window */
        QWidget {
            background-color: #f3f3f3;
            color: #333333;
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
        }

        /* Labels */
        QLabel {
            color: #333333;
            font-weight: bold;
            padding: 2px;
            background-color: transparent;
        }
        
        QCheckBox, QRadioButton {
            spacing: 8px;
            color: #333333;
            font-weight: 500;
            background-color: transparent;
        }
        QRadioButton::indicator {
            width: 14px; height: 14px;
            border-radius: 7px;
            border: 2px solid #555555;
            background-color: #ffffff;
        }
        QRadioButton::indicator:checked {
            background-color: #0078d7;
            border: 2px solid #0078d7;
        }
        QCheckBox::indicator {
            width: 14px; height: 14px;
            border-radius: 3px;
            border: 2px solid #555555;
            background-color: #ffffff;
        }
        QCheckBox::indicator:checked {
            background-color: #0078d7;
            border-color: #0078d7;
        }

        /* Group Boxes */
        QGroupBox {
            border: 1px solid #cccccc;
            border-radius: 6px;
            margin-top: 24px;
            font-weight: bold;
            background-color: transparent;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #0078d7; /* Accent Blue */
            background-color: transparent;
        }

        /* Input Fields */
        QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
            background-color: #ffffff;
            border: 1px solid #bbbbbb;
            border-radius: 4px;
            padding: 5px;
            color: #333333;
            selection-background-color: #0078d7;
            font-weight: bold;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus {
            border: 1px solid #0078d7;
            background-color: #ffffff;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            subcontrol-origin: border;
            width: 20px;
            background-color: #e1e1e1; 
            border-left: 1px solid #cccccc;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #d0d0d0;
        }
        QSpinBox::up-arrow {
            image: url(:/Images/arrow_up.svg);
            width: 10px; height: 10px;
        }
        QSpinBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 10px; height: 10px;
        }

        QComboBox {
            padding-right: 20px; /* Space for arrow */
        }
        
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 25px;
            border-left-width: 0px;
            border-top-right-radius: 3px;
            border-bottom-right-radius: 3px;
            background-color: #e1e1e1; /* Distinct button area */
        }
        
        QComboBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 12px; height: 12px;
        }
        QComboBox::down-arrow:on {
            top: 1px;
            left: 1px;
        }

        /* Buttons */
        QPushButton {
            background-color: #e1e1e1;
            border: 1px solid #adadad;
            border-radius: 4px;
            padding: 6px 16px;
            color: #333333;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #dbeeff;
            border-color: #0078d7;
        }
        QPushButton:pressed {
            background-color: #cce4f7;
            border-color: #005a9e;
        }
        QPushButton:checked {
            background-color: #b3d7f3; /* Lighter Blue */
        }

        /* Theme Button Specific */
        QPushButton#btnTheme {
             background-color: transparent;
             border: 1px solid #aaa;
             color: #333;
        }
        QPushButton#btnTheme:hover {
             border: 1px solid #0078d7;
             background-color: #e1e1e1;
        }
        
        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #cccccc;
            background: #f3f3f3;
        }
        QTabBar::tab {
            background: #e1e1e1;
            border: 1px solid #cccccc;
            padding: 8px 24px;
            border-bottom-color: #cccccc;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            color: #555555;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background: #f3f3f3;
            border-bottom-color: #f3f3f3; /* Blend */
            color: #0078d7;
            font-weight: bold;
        }
        QTabBar::tab:hover {
            background: #ffffff;
        }

        /* Splitter (Light) */
        QSplitter::handle {
            background-color: #d0d0d0;
            width: 6px; 
            height: 6px;
        }
        QSplitter::handle:hover {
            background-color: #0078d7; 
        }
        
        /* Scrollbars (Light) */
        QScrollBar:vertical {
            border: none;
            background: #f3f3f3;
            width: 12px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #cdcdcd;
            min-height: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #a0a0a0;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* QTableWidget */
        QTableWidget {
            background-color: #ffffff;
            color: #333333;
            gridline-color: #e0e0e0;
            border: 1px solid #cccccc;
            font-family: 'Consolas', monospace;
            font-size: 10pt;
        }
        QHeaderView::section {
            background-color: #e1e1e1;
            color: #333333;
            padding: 4px;
            border: 1px solid #cccccc;
            font-weight: bold;
        }
        QTableWidget::item {
            padding: 2px;
        }
        QTableWidget::item:selected {
            background-color: #0078d7;
            color: white;
        }
        )";
        this->setStyleSheet(lightStyle);
        BASEUI_DEBUG && DEBUG("Light Theme Applied");
    }
}

void MainWindow::applyDarkTheme()
{
    QString darkStyle = R"(
        /* Main Window */
        QWidget {
            background-color: #2b2b2b;
            color: #f0f0f0;
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
        }

        /* Labels */
        QLabel {
            color: #d0d0d0;
            font-weight: bold;
            padding: 2px;
            background-color: transparent; /* Fix for dark box */
        }
        
        QCheckBox, QRadioButton {
            spacing: 8px;
            color: #e0e0e0;
            font-weight: 500;
            background-color: transparent;
        }
        QRadioButton::indicator {
            width: 14px; height: 14px;
            border-radius: 7px;
            border: 2px solid #888888;
            background-color: #3b3b3b;
        }
        QRadioButton::indicator:checked {
            background-color: #007acc;
            border: 2px solid #007acc;
        }
        QCheckBox::indicator {
            width: 14px; height: 14px;
            border-radius: 3px;
            border: 2px solid #888888;
            background-color: #3b3b3b;
        }
        QCheckBox::indicator:checked {
            background-color: #007acc;
            border-color: #007acc;
        }

        /* Group Boxes */
        QGroupBox {
            border: 1px solid #505050;
            border-radius: 6px;
            margin-top: 24px;
            font-weight: bold;
            background-color: transparent;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #4da6ff; /* Accent Blue */
            background-color: transparent;
        }

        /* Input Fields */
        QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
            background-color: #3b3b3b;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 5px;
            color: #ffffff;
            selection-background-color: #007acc;
            font-weight: bold;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus {
            border: 1px solid #4da6ff;
            background-color: #404040;
        }
        
        QSpinBox::up-button, QSpinBox::down-button {
            subcontrol-origin: border;
            width: 20px;
            background-color: #444444; 
            border-left: 1px solid #555555;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #555555;
        }
        QSpinBox::up-arrow {
            image: url(:/Images/arrow_up.svg);
            width: 10px; height: 10px;
        }
        QSpinBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 10px; height: 10px;
        }

        QComboBox {
            padding-right: 20px; /* Space for arrow */
        }
        
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 25px;
            border-left-width: 0px;
            border-top-right-radius: 3px;
            border-bottom-right-radius: 3px;
            background-color: #444444; /* Distinct button area */
        }
        
        QComboBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 12px; height: 12px;
        }
        QComboBox::down-arrow:on { /* shift the arrow when popup is open */
            top: 1px;
            left: 1px;
        }

        /* Buttons */
        QPushButton {
            background-color: #444444;
            border: 1px solid #5f5f5f;
            border-radius: 4px;
            padding: 6px 16px;
            color: #ffffff;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #505050;
            border-color: #4da6ff;
        }
        QPushButton:pressed {
            background-color: #007acc;
            border-color: #007acc;
        }
        QPushButton:checked {
            background-color: #005a9e; /* Darker Blue */
        }
        
        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #444444;
            background: #2b2b2b;
        }
        QTabBar::tab {
            background: #3a3a3a;
            border: 1px solid #444444;
            padding: 8px 24px;
            border-bottom-color: #444444; /* Blend with pane */
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            color: #aaaaaa;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background: #2b2b2b; /* Match window bg */
            border-bottom-color: #2b2b2b;
            color: #ffffff;
            font-weight: bold;
        }
        QTabBar::tab:hover {
            background: #404040;
        }

        /* Splitter (Dark) */
        QSplitter::handle {
            background-color: #444444;
            width: 6px; /* Wider for better grip */
            height: 6px;
        }
        QSplitter::handle:hover {
            background-color: #0078d7; /* Distinct Blue Hover */
        }
        
        /* Scrollbars (Dark) */
        QScrollBar:vertical {
            border: none;
            background: #2b2b2b;
            width: 12px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #555;
            min-height: 20px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #777; /* Lighter on hover */
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* QTableWidget */
        QTableWidget {
            background-color: #1e1e1e;
            color: #dcdcdc;
            gridline-color: #333333;
            border: 1px solid #444;
            font-family: 'Consolas', monospace;
            font-size: 10pt;
        }
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #dcdcdc;
            padding: 4px;
            border: 1px solid #444;
            font-weight: bold;
        }
        QTableWidget::item {
            padding: 2px;
        }
        QTableWidget::item:selected {
            background-color: #007acc;
            color: white;
        }

    )";
    this->setStyleSheet(darkStyle);
    BASEUI_DEBUG && DEBUG("Dark Theme Applied");
}






/**
 * @brief Handles the Connect button click.
 * 
 * Establishes a connection based on the selected mode (Serial, TCP Client, TCP Server, or UDP).
 * Initializes the appropriate communication handler and connects its signals.
 */
void MainWindow::on_btnConnect_clicked()
{
    if (isConnected) {
        on_btnDisconnect_clicked();
        return;
    }
    
    // Safety check
    if (m_handler) {
        delete m_handler;
        m_handler = nullptr;
    }

    // Mode based on Active Tab
    // Tab 0 = Serial, Tab 1 = Network
    int pIndex = ui->tabSettings->currentIndex();
    
    if (pIndex == 0) {
        // --- SERIAL ---
        QString portName = ui->comboPort->currentData().toString();
        if (portName.isEmpty()) portName = ui->comboPort->currentText();
        
        int baudRate = ui->comboBaudRate->currentData().toInt();
        if (baudRate == 0) baudRate = ui->comboBaudRate->currentText().toInt();
        
        if (baudRate == 0) baudRate = ui->comboBaudRate->currentText().toInt();
        
        int dataBits = ui->comboDataBits->currentData().toInt();
        int parity = ui->comboParity->currentData().toInt();
        int stopBits = ui->comboStopBits->currentData().toInt();
        int flowControl = ui->comboFlowControl->currentData().toInt();
        
        SerialQT* serial = new SerialQT();
        if (serial->initialize(portName, baudRate, dataBits, parity, stopBits, flowControl)) {
            m_handler = serial;
        } else {
             delete serial;
             m_handler = nullptr;
             showCustomMessage("Connection Failed", "Could not open serial port.", true);
             return; 
        }
        
    } else {
        // --- NETWORK ---
        QString netType = ui->comboNetProto->currentText();
        QString ip = ui->txtIpAddress->text();
        int port = ui->spinPort->value();
        
        if (netType.contains("TCP Client")) {
            TcpClient* tcp = new TcpClient();
            tcp->initialize(ip, port);
            m_handler = tcp;
        } else if (netType.contains("TCP Server")) {
            TcpServer_SingleClient* svr = new TcpServer_SingleClient();
            svr->initialize(port);
            m_handler = svr;
        } else if (netType.contains("UDP")) {
             Udp* udp = new Udp();
             udp->initialize(ip, port);
             m_handler = udp;
        }
    }
    
    if (m_handler) {
        connect(m_handler, &AbstractCommunicationHandler::connected, this, &MainWindow::onConnected);
        connect(m_handler, &AbstractCommunicationHandler::disconnected, this, &MainWindow::onDisconnected);
        connect(m_handler, &AbstractCommunicationHandler::error, this, &MainWindow::onError);
        connect(m_handler, &AbstractCommunicationHandler::receivedData, this, &MainWindow::onDataReceived);
    }
}

void MainWindow::on_btnDisconnect_clicked()
{
    if (m_handler) {
        // Close first
        m_handler->close();
        
        // Ensure UI is unblocked immediately before potentially blocking delete
        onDisconnected(); 

        delete m_handler; 
        m_handler = nullptr;
    } else {
        // Fallback if null but isConnected was true
        onDisconnected();
    }
    
    // (Redundant check removed as we moved onDisconnected call up)
}

void MainWindow::on_btnSend_clicked()
{
    sendPacket();
}

void MainWindow::on_chkAutoSend_toggled(bool checked)
{
    if (!isConnected) {
        if (checked) {
            ui->chkAutoSend->setChecked(false); // Revert if not connected
            showCustomMessage("Not Connected", "Please connect before starting auto-send.", true);
        }
        return;
    }
    
    if (checked) {
        int interval = ui->spinInterval->value();
        m_autoSendTimer->start(interval);
    } else {
        if (m_autoSendTimer->isActive()) m_autoSendTimer->stop();
    }
}

// void MainWindow::on_btnClearTxLog_clicked() // Removed


/**
 * @brief Sends a packet through the active connection.
 * @param overrideData Optional data to send instead of the requested UI payload.
 * 
 * If overrideData is empty, constructs the packet from the UI fields (SOF, Payload, EOF)
 * or Raw Hex field depending on the selected Packet Mode.
 * Appends CRLF if configured. Logging and Tx counting are also handled here.
 */
void MainWindow::sendPacket(QByteArray overrideData)
{
    if (!isConnected || !m_handler) return;

    QByteArray dataToSend;

    if (!overrideData.isEmpty()) {
        dataToSend = overrideData;
    } else {
        dataToSend = getPacketData();
    }

    // Check Line Endings
    if (ui->chkCR->isChecked()) dataToSend.append('\r');
    if (ui->chkLF->isChecked()) dataToSend.append('\n');

    if (dataToSend.isEmpty()) return;

    m_handler->send(dataToSend);
    
    // Log to Table
    txCount += dataToSend.size();
    updateCounters(rxCount, txCount);
    addPacketToTable(true, dataToSend);
    
    writeLog(true, dataToSend);
}

QByteArray MainWindow::getPacketData()
{
    QString text = ui->txtPayload->text();
    return text.toUtf8();
}

void MainWindow::onDataReceived(QByteArray data)
{
    writeLog(false, data);
    rxCount += data.size();

    updateCounters(rxCount, txCount);
    
    addPacketToTable(false, data);
}

void MainWindow::addPacketToTable(bool isTx, const QByteArray &data)
{
    int row = ui->tablePackets->rowCount();
    ui->tablePackets->insertRow(row);
    
    // 1. Time
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->tablePackets->setItem(row, 0, new QTableWidgetItem(timeStr));
    
    // 2. Dir
    QTableWidgetItem *itemDir = new QTableWidgetItem(isTx ? "TX" : "RX");
    itemDir->setForeground(isTx ? QBrush(Qt::green) : QBrush(Qt::cyan));
    ui->tablePackets->setItem(row, 1, itemDir);
    
    // 3. HEX
    QString hex = data.toHex(' ').toUpper();
    ui->tablePackets->setItem(row, 2, new QTableWidgetItem(hex));
    
    // 4. ASCII
    QString ascii;
    for (char c : data) {
        if (c >= 32 && c <= 126) ascii.append(c);
        else ascii.append('.');
    }
    ui->tablePackets->setItem(row, 3, new QTableWidgetItem(ascii));
    
    ui->tablePackets->scrollToBottom();
}

void MainWindow::on_btnSendFile_clicked()
{
    if (!isConnected) {
        QMessageBox::warning(this, "Not Connected", "Please connect first.");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Send File");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        m_handler->send(data);
        txCount += data.size();
        updateCounters(rxCount, txCount);
        QMessageBox::information(this, "Sent", QString("Sent %1 bytes form file.").arg(data.size()));
    }
}

void MainWindow::on_chkStayOnTop_clicked(bool checked)
{
    Qt::WindowFlags flags = this->windowFlags();
    if (checked) {
        this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    } else {
        this->setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
    }
    this->show();
}

void MainWindow::updateCounters(int rx, int tx)
{
    ui->lblRxCount->setText(QString("Rx: %1").arg(rx));
    ui->lblTxCount->setText(QString("Tx: %1").arg(tx));
}

void MainWindow::on_closeApp_clicked()
{
    close();
}

void MainWindow::on_btnClearRx_clicked() { ui->tablePackets->setRowCount(0); rxCount=0; txCount=0; updateCounters(rxCount, txCount); }


void MainWindow::onAutoSendTimerTimeout()
{
    sendPacket();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->pos().y() < 60) {
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (event->pos().y() < 60) {
            move(event->globalPosition().toPoint() - m_dragPosition);
            event->accept();
        }
    }
}

void MainWindow::showCustomMessage(const QString &title, const QString &text, bool isError)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(isError ? QMessageBox::Critical : QMessageBox::Information);
    msgBox.setStyleSheet(R"(
        QMessageBox { background-color: #2b2b2b; color: #f0f0f0; }
        QLabel { color: #f0f0f0; font-size: 11pt; }
        QPushButton {
            background-color: #444; color: white;
            border: 1px solid #555; padding: 5px 15px;
            border-radius: 3px; min-width: 60px;
        }
        QPushButton:hover { background-color: #505050; }
    )");
    msgBox.exec();
}

void MainWindow::onError(int err)
{
    showCustomMessage("Connection Error", "An error occurred. Code: " + QString::number(err), true);
    on_btnDisconnect_clicked();
}

void MainWindow::onConnected()
{
    isConnected = true;
    ui->btnConnect->setChecked(true);
    ui->btnConnect->setText("CONNECTED");
    ui->btnConnect->setStyleSheet("background-color: #006400; border: 1px solid #008000;"); 
    ui->btnDisconnect->setEnabled(true);
    
    // Disable Settings Tabs
    ui->tabSettings->setEnabled(false);
}

void MainWindow::onDisconnected()
{
    isConnected = false;
    ui->btnConnect->setChecked(false);
    ui->btnConnect->setText("CONNECT");
    ui->btnConnect->setStyleSheet(""); 
    ui->btnDisconnect->setEnabled(false);
    
    ui->tabSettings->setEnabled(true);
    
    if (m_autoSendTimer->isActive()) {
        m_autoSendTimer->stop();
        ui->chkAutoSend->setChecked(false);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("PacketForge", "PacketTransmitter");
    settings.setValue("geometry", saveGeometry());
    
    // Save Macros
    for(auto it = m_macros.begin(); it != m_macros.end(); ++it) {
        settings.beginGroup("Macro" + QString::number(it.key()));
        settings.setValue("name", it.value().name);
        settings.setValue("data", it.value().data);
        settings.setValue("autoSend", it.value().autoSend);
        settings.setValue("intervalMs", it.value().intervalMs);
        settings.setValue("packetMode", it.value().packetMode);
        settings.setValue("sof", it.value().sof);
        settings.setValue("eof", it.value().eof);
        settings.endGroup();
    }
    
    // Close Log if Open
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }

    QWidget::closeEvent(event);

}

void MainWindow::configureMacro(int index)
{
    MacroSettings current = m_macros[index];
    MacroDialog dlg(current, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_macros[index] = dlg.getSettings();
        
        // Update Button Text
        QPushButton *btn = this->findChild<QPushButton*>("btnM" + QString::number(index));
        if (btn) btn->setText(m_macros[index].name);
    }
}

void MainWindow::onMacroClicked(int index)
{
    MacroSettings s = m_macros[index];
    
    if (s.autoSend) {
        // Toggle Timer Logic
        if (!m_macroTimers.contains(index)) {
             m_macroTimers[index] = new QTimer(this);
             m_macroTimers[index]->setTimerType(Qt::PreciseTimer);
             connect(m_macroTimers[index], &QTimer::timeout, [this, s]() {
                 QByteArray toSend;
                 // Smart Detect: If contains ONLY hex chars and spaces, treat as hex
                 static QRegularExpression hexRegex("^[0-9A-Fa-f\\s]*$");
                 if (hexRegex.match(s.data).hasMatch()) {
                     toSend = QByteArray::fromHex(s.data.toUtf8());
                 } else {
                     toSend = s.data.toUtf8();
                 }
                 
                 // Fallback if empty (e.g. empty hex string)
                 if (toSend.isEmpty() && !s.data.isEmpty()) toSend = s.data.toUtf8();
                 
                 sendPacket(toSend);
             });
        }
        
        QTimer *timer = m_macroTimers[index];
        QPushButton *btn = this->findChild<QPushButton*>("btnM" + QString::number(index));
        
        if (timer->isActive()) {
            timer->stop();
             // Visual indication stopped
             if(btn) {
                 btn->setStyleSheet("QPushButton { border-top-right-radius: 0px; border-bottom-right-radius: 0px; border-right: 0px; margin-right: 0px; }"); // Revert to default split style
             }
             ui->grpTransmit->setEnabled(true); // Re-enable manual transmit
        } else {
            timer->start(s.intervalMs);
             // Visual indication started
             if(btn) {
                 btn->setStyleSheet("QPushButton { background-color: #2e7d32; color: white; border: 1px solid #4caf50; border-top-right-radius: 0px; border-bottom-right-radius: 0px; border-right: 0px; margin-right: 0px; }");
             }
             ui->grpTransmit->setEnabled(false); // Disable manual transmit to prevent conflict
        }
        
    } else {
        // Send Macro Data (One-shot)
        QByteArray dataToSend;
        
        // Smart Detect: If contains ONLY hex chars and spaces, treat as hex
        // Ignoring packetMode 1 check in favor of Smart Detect for better UX on "Hello"
        static QRegularExpression hexRegex("^[0-9A-Fa-f\\s]*$");
        if (hexRegex.match(s.data).hasMatch()) {
             // It's pure hex
             if (s.packetMode == 0) { // Structured
                 QString payloadHex = s.data;
                 payloadHex.remove(QRegularExpression("[^0-9A-Fa-f]"));
                 QString sofHex = s.sof; sofHex.remove(QRegularExpression("[^0-9A-Fa-f]"));
                 QString eofHex = s.eof; eofHex.remove(QRegularExpression("[^0-9A-Fa-f]"));
                 
                 if (!sofHex.isEmpty()) dataToSend.append(QByteArray::fromHex(sofHex.toLatin1()));
                 dataToSend.append(QByteArray::fromHex(payloadHex.toLatin1()));
                 if (!eofHex.isEmpty()) dataToSend.append(QByteArray::fromHex(eofHex.toLatin1()));
             } else {
                 // Raw Hex
                 QString raw = s.data;
                 raw.remove(QRegularExpression("[^0-9A-Fa-f]")); 
                 dataToSend = QByteArray::fromHex(raw.toLatin1());
             }
        } else {
            // Contains non-hex chars (like 'H', 'l', 'o') -> Treat as Text
            dataToSend = s.data.toUtf8();
        }

        sendPacket(dataToSend);
    }
}

// --- Logging ---

void MainWindow::on_chkLogToFile_toggled(bool checked)
{
    if (checked) {
        QString logsPath = LOG_FOLDER_PATH;
        QDir dir(logsPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString fileName = QString("%1/PacketLog_%2.txt").arg(logsPath, timestamp);

        m_logFile.setFileName(fileName);
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_logStream.setDevice(&m_logFile);
            ui->chkLogToFile->setText(tr("Logging to %1").arg(QFileInfo(fileName).fileName()));
        } else {
            showCustomMessage("Logging Error", "Could not create log file.", true);
            ui->chkLogToFile->setChecked(false); // Revert
        }
    } else {
        if (m_logFile.isOpen()) {
            m_logFile.close();
            ui->chkLogToFile->setText("Log to File");
        }
    }
}

void MainWindow::writeLog(bool isTx, const QByteArray &data)
{
    if (!m_logFile.isOpen()) return;

    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss.zzz]");
    QString direction = isTx ? "[TX]" : "[RX]";
    
    // Format: Timestamp [TX] HEX_DATA (ASCII)
    QString hex = data.toHex(' ').toUpper();
    
    // Create a safe ASCII representation (replace non-printables with dot)
    QString ascii;
    for (char c : data) {
        if (c >= 32 && c <= 126) {
            ascii.append(c);
        } else {
            ascii.append('.');
        }
    }

    m_logStream << timestamp << " " << direction << " " << hex << "  (" << ascii << ")\n";
    m_logStream.flush();
}

void MainWindow::onTableDoubleClicked(int row, int column)
{
    QTableWidgetItem *item = ui->tablePackets->item(row, column);
    if(!item) return;
    
    QString content = item->text();
    if(content.isEmpty()) return;
    
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("Cell Content Viewer");
    dlg->resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(dlg);
    
    QTextEdit *txt = new QTextEdit(dlg);
    txt->setReadOnly(true);
    txt->setPlainText(content);
    txt->setFont(QFont("Consolas", 10)); // Monospace for better hex/data viewing
    
    QPushButton *btnClose = new QPushButton("Close", dlg);
    connect(btnClose, &QPushButton::clicked, dlg, &QDialog::accept);
    
    layout->addWidget(new QLabel("Full Cell Content:", dlg));
    layout->addWidget(txt);
    layout->addWidget(btnClose);
    
    dlg->exec();
    delete dlg;
}

