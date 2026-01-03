#include "ConnectionTab.h"
#include "ui_ConnectionTab.h" 

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
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>

ConnectionTab::ConnectionTab(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConnectionTab)
    , m_handler(nullptr)
    , m_autoSendTimer(new QTimer(this))
    , isConnected(false)
    , m_isDarkTheme(true)
{
    m_autoSendTimer->setTimerType(Qt::PreciseTimer);
    ui->setupUi(this);

    // Standard Window
    // setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    // resize(1100, 750);

    // Load Persistence (Size & State)
    /*
    QSettings settings("PacketForge", "PacketTransmitter");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    */

    // Connect auto-send timer
    connect(m_autoSendTimer, &QTimer::timeout, this, &ConnectionTab::onAutoSendTimerTimeout);

    ui->grpSerial->setEnabled(true);
    ui->grpNetwork->setEnabled(true);

    // Initial Setup
    // QIcon appIcon(":/Images/app_icon.png");
    // setWindowIcon(appIcon);
    
    setupUiDefaults();
    // applyDarkTheme();

    /*
    // Connect Theme Button
    QPushButton *btnTheme = this->findChild<QPushButton*>("btnTheme");
    if (btnTheme) {
        connect(btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    }
    */
    
    // Connect Enter Key on Payload to Send mechanism
    connect(ui->txtPayload, &QLineEdit::returnPressed, this, &ConnectionTab::on_btnSend_clicked);

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
    connect(ui->tablePackets, &QTableWidget::cellDoubleClicked, this, &ConnectionTab::onTableDoubleClicked);
}

ConnectionTab::~ConnectionTab()
{
    if (m_handler) delete m_handler;
    delete ui;
}

void ConnectionTab::setupUiDefaults()
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
    ui->tablePackets->setSelectionMode(QAbstractItemView::SingleSelection); // Allow Selection
    ui->tablePackets->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tablePackets->setFocusPolicy(Qt::NoFocus); // No Focus
    connect(ui->tablePackets, &QTableWidget::cellDoubleClicked, this, &ConnectionTab::onTableDoubleClicked);


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
    
    // Set Initial State
    onDisconnected();
}

void ConnectionTab::refreshSerialPorts()
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








/**
 * @brief Handles the Connect button click.
 * 
 * Establishes a connection based on the selected mode (Serial, TCP Client, TCP Server, or UDP).
 * Initializes the appropriate communication handler and connects its signals.
 */
void ConnectionTab::on_btnConnect_clicked()
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
        connect(m_handler, &AbstractCommunicationHandler::connected, this, &ConnectionTab::onConnected);
        connect(m_handler, &AbstractCommunicationHandler::disconnected, this, &ConnectionTab::onDisconnected);
        connect(m_handler, &AbstractCommunicationHandler::error, this, &ConnectionTab::onError);
        connect(m_handler, &AbstractCommunicationHandler::receivedData, this, &ConnectionTab::onDataReceived);
    }
}

void ConnectionTab::on_btnDisconnect_clicked()
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

void ConnectionTab::on_btnSend_clicked()
{
    sendPacket();
}

void ConnectionTab::on_chkAutoSend_toggled(bool checked)
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
void ConnectionTab::sendPacket(QByteArray overrideData)
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

QByteArray ConnectionTab::getPacketData()
{
    QString text = ui->txtPayload->text();
    return text.toUtf8();
}

void ConnectionTab::onDataReceived(QByteArray data)
{
    writeLog(false, data);
    rxCount += data.size();

    updateCounters(rxCount, txCount);
    
    addPacketToTable(false, data);
}

void ConnectionTab::addPacketToTable(bool isTx, const QByteArray &data)
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
    QTableWidgetItem *itemHex = new QTableWidgetItem(hex);
    itemHex->setData(Qt::UserRole, data); // Store Raw Data for Double-Click Viewer
    ui->tablePackets->setItem(row, 2, itemHex);
    
    // 4. ASCII
    QString ascii;
    for (char c : data) {
        if (c >= 32 && c <= 126) ascii.append(c);
        else ascii.append('.');
    }
    ui->tablePackets->setItem(row, 3, new QTableWidgetItem(ascii));
    
    ui->tablePackets->scrollToBottom();
}

void ConnectionTab::on_btnSendFile_clicked()
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



void ConnectionTab::updateCounters(int rx, int tx)
{
    ui->lblRxCount->setText(QString("Rx: %1").arg(rx));
    ui->lblTxCount->setText(QString("Tx: %1").arg(tx));
}



void ConnectionTab::on_btnClearRx_clicked() { ui->tablePackets->setRowCount(0); rxCount=0; txCount=0; updateCounters(rxCount, txCount); }


void ConnectionTab::onAutoSendTimerTimeout()
{
    sendPacket();
}

void ConnectionTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->pos().y() < 60) {
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void ConnectionTab::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        if (event->pos().y() < 60) {
            move(event->globalPosition().toPoint() - m_dragPosition);
            event->accept();
        }
    }
}

void ConnectionTab::showCustomMessage(const QString &title, const QString &text, bool isError)
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

void ConnectionTab::onError(int err)
{
    showCustomMessage("Connection Error", "An error occurred. Code: " + QString::number(err), true);
    on_btnDisconnect_clicked();
}

void ConnectionTab::onConnected()
{
    isConnected = true;
    ui->btnConnect->setChecked(true);
    ui->btnConnect->setText("CONNECTED");
    ui->btnConnect->setStyleSheet("background-color: #006400; border: 1px solid #008000;"); 
    ui->btnDisconnect->setEnabled(true);
    
    // Disable Settings Tabs
    ui->tabSettings->setEnabled(false);
    
    // Enable Transmit & Macros
    ui->grpTransmit->setEnabled(true);
    ui->grpMacros->setEnabled(true);
}

void ConnectionTab::onDisconnected()
{
    isConnected = false;
    ui->btnConnect->setChecked(false);
    ui->btnConnect->setText("CONNECT");
    ui->btnConnect->setStyleSheet(""); 
    ui->btnDisconnect->setEnabled(false);
    
    ui->tabSettings->setEnabled(true);
    
    // Disable Transmit & Macros
    ui->grpTransmit->setEnabled(false);
    ui->grpMacros->setEnabled(false);
    
    if (m_autoSendTimer->isActive()) {
        m_autoSendTimer->stop();
        ui->chkAutoSend->setChecked(false);
    }
}

void ConnectionTab::closeEvent(QCloseEvent *event)
{
    QSettings settings("PacketForge", "PacketTransmitter");
    // settings.setValue("geometry", saveGeometry()); // Handled by MainWindow
    
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

void ConnectionTab::configureMacro(int index)
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

void ConnectionTab::onMacroClicked(int index)
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

void ConnectionTab::on_chkLogToFile_toggled(bool checked)
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

void ConnectionTab::writeLog(bool isTx, const QByteArray &data)
{
    emit logData(isTx, data);

    if (!m_logFile.isOpen()) return;

    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss.zzz]");
    QString direction = isTx ? "[TX]" : "[RX]";
    
    // Format: Timestamp [TX] HEX_DATA (ASCII)
    QString hex = data.toHex(' ').toUpper();
    
    // Create a safe ASCII representation
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

void ConnectionTab::onTableDoubleClicked(int row, int column)
{
    QTableWidgetItem *item = ui->tablePackets->item(row, column);
    if(!item) return;

    // Try to get UserRole data (original raw bytes)
    QByteArray data = item->data(Qt::UserRole).toByteArray();
    
    // If empty (e.g. user clicked Timestamp or Dir column), try to find the Hex column (index 2)
    if (data.isEmpty()) {
        QTableWidgetItem *hexItem = ui->tablePackets->item(row, 2);
        if (hexItem) {
            data = hexItem->data(Qt::UserRole).toByteArray();
        }
    }
    
    if (data.isEmpty()) return;

    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("Raw Data Inspector");
    dlg->resize(600, 450);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // 1. Hex View
    QGroupBox *grpHex = new QGroupBox("Raw Hex (Space Separated)", dlg);
    QVBoxLayout *hexLayout = new QVBoxLayout(grpHex);
    QTextEdit *txtHex = new QTextEdit(grpHex);
    txtHex->setReadOnly(true);
    txtHex->setFont(QFont("Consolas", 10));
    txtHex->setPlainText(data.toHex(' ').toUpper());
    hexLayout->addWidget(txtHex);
    layout->addWidget(grpHex);

    // 2. ASCII View (Original)
    QGroupBox *grpAscii = new QGroupBox("Original ASCII (Raw)", dlg);
    QVBoxLayout *asciiLayout = new QVBoxLayout(grpAscii);
    QTextEdit *txtAscii = new QTextEdit(grpAscii);
    txtAscii->setReadOnly(true);
    txtAscii->setFont(QFont("Consolas", 10));
    // Provide raw latin1 view to preserve 1-byte-per-char visual as much as possible
    txtAscii->setPlainText(QString::fromLatin1(data));
    asciiLayout->addWidget(txtAscii);
    layout->addWidget(grpAscii);

    // Close
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(btnBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    layout->addWidget(btnBox);

    dlg->exec();
    delete dlg;
}
