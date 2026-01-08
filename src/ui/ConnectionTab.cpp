/**
 * @file ConnectionTab.cpp
 * @brief Implementation of the connection tab widget.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "ConnectionTab.h"
#include "ui_ConnectionTab.h"

// Qt Core
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QTextStream>

// Qt Widgets
#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>

// Qt Layouts
#include <QGridLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

// Qt Events
#include <QCloseEvent>

// Project - Network
#include "Debugger.h"
#include "SerialQTClass.h"
#include "TcpClientClass.h"
#include "TcpServer_SingleClientClass.h"
#include "UdpClass.h"

#define BASEUI_DEBUG false

ConnectionTab::ConnectionTab(QWidget *parent)
    : QWidget(parent), ui(new Ui::ConnectionTab), m_handler(nullptr),
      m_autoSendTimer(new QTimer(this)), isConnected(false),
      m_isDarkTheme(true), m_uiRefreshTimer(new QTimer(this)) {
  m_autoSendTimer->setTimerType(Qt::PreciseTimer);

  m_uiRefreshTimer->setInterval(100);
  connect(m_uiRefreshTimer, &QTimer::timeout, this,
          &ConnectionTab::flushPacketBufferToTable);

  ui->setupUi(this);

  QHBoxLayout *hLayoutFormat = new QHBoxLayout();
  hLayoutFormat->setContentsMargins(0, 0, 0, 5);

  rbInputAscii = new QRadioButton("ASCII", this);
  rbInputHex = new QRadioButton("HEX", this);
  rbInputBinary = new QRadioButton("BINARY", this);
  rbInputAscii->setChecked(true);

  grpInputFormat = new QButtonGroup(this);
  grpInputFormat->addButton(rbInputAscii, 0);
  grpInputFormat->addButton(rbInputHex, 1);
  grpInputFormat->addButton(rbInputBinary, 2);

  QString asciiStyle = R"(
        QRadioButton { font-weight: bold; color: #4CAF50; }
        QRadioButton::indicator:checked { background-color: #4CAF50; border: 2px solid #2E7D32; border-radius: 7px; }
    )";
  QString hexStyle = R"(
        QRadioButton { font-weight: bold; color: #FF9800; }
        QRadioButton::indicator:checked { background-color: #FF9800; border: 2px solid #E65100; border-radius: 7px; }
    )";
  QString binaryStyle = R"(
        QRadioButton { font-weight: bold; color: #2196F3; }
        QRadioButton::indicator:checked { background-color: #2196F3; border: 2px solid #1565C0; border-radius: 7px; }
    )";
  rbInputAscii->setStyleSheet(asciiStyle);
  rbInputHex->setStyleSheet(hexStyle);
  rbInputBinary->setStyleSheet(binaryStyle);

  hLayoutFormat->addWidget(rbInputAscii);
  hLayoutFormat->addWidget(rbInputHex);
  hLayoutFormat->addWidget(rbInputBinary);
  hLayoutFormat->addStretch();

  ui->verticalLayout_Tx->insertLayout(0, hLayoutFormat);

  connect(rbInputAscii, &QRadioButton::toggled, [this](bool checked) {
    if (checked)
      convertInputFormat(0);
  });
  connect(rbInputHex, &QRadioButton::toggled, [this](bool checked) {
    if (checked)
      convertInputFormat(1);
  });
  connect(rbInputBinary, &QRadioButton::toggled, [this](bool checked) {
    if (checked)
      convertInputFormat(2);
  });

  ui->gridLayout_Params->setColumnStretch(3, 1);

  connect(m_autoSendTimer, &QTimer::timeout, this,
          &ConnectionTab::onAutoSendTimerTimeout);

  ui->grpSerial->setEnabled(true);
  ui->grpNetwork->setEnabled(true);

  setupUiDefaults();
  connect(ui->txtPayload, &QLineEdit::returnPressed, this,
          &ConnectionTab::on_btnSend_clicked);

  connect(ui->chkDTR, &QCheckBox::toggled, [this](bool checked) {
    if (m_handler && isConnected)
      m_handler->setDtr(checked);
  });
  connect(ui->chkRTS, &QCheckBox::toggled, [this](bool checked) {
    if (m_handler && isConnected)
      m_handler->setRts(checked);
  });

  QTimer *pinPollingTimer = new QTimer(this);
  connect(pinPollingTimer, &QTimer::timeout, [this]() {
    if (isConnected && m_handler) {
      int pins = m_handler->getPinStatus();
      // QSerialPort::PinoutSignals: NoSignal=0, Tx=1, Rx=2, Dsr=4, Dcd=8,
      // Ring=16, Cts=32 Mapping: 4=DSR, 8=CD, 16=RI, 32=CTS

      ui->lblDSR->setStyleSheet((pins & 4)
                                    ? "color: #00FF00; font-weight: bold;"
                                    : "color: gray; font-weight: bold;");
      ui->lblCD->setStyleSheet((pins & 8) ? "color: #00FF00; font-weight: bold;"
                                          : "color: gray; font-weight: bold;");
      ui->lblRI->setStyleSheet((pins & 16)
                                   ? "color: #00FF00; font-weight: bold;"
                                   : "color: gray; font-weight: bold;");
      ui->lblCTS->setStyleSheet((pins & 32)
                                    ? "color: #00FF00; font-weight: bold;"
                                    : "color: gray; font-weight: bold;");
    } else {
      ui->lblDSR->setStyleSheet("color: gray; font-weight: bold;");
      ui->lblCD->setStyleSheet("color: gray; font-weight: bold;");
      ui->lblRI->setStyleSheet("color: gray; font-weight: bold;");
      ui->lblCTS->setStyleSheet("color: gray; font-weight: bold;");
    }
  });
  pinPollingTimer->start(200);

  QTimer *portPollingTimer = new QTimer(this);
  connect(portPollingTimer, &QTimer::timeout, this,
          &ConnectionTab::refreshSerialPorts);
  portPollingTimer->start(1000);

  for (int i = 1; i <= 12; ++i) {
    QString sendName = QString("btnM%1").arg(i);
    QString cfgName = QString("btnM%1_Cfg").arg(i);

    QPushButton *btnSend = this->findChild<QPushButton *>(sendName);
    QPushButton *btnCfg = this->findChild<QPushButton *>(cfgName);

    if (btnSend && btnCfg) {
      btnSend->setProperty("type", "macro_main");
      btnCfg->setProperty("type", "macro_config");

      btnSend->style()->unpolish(btnSend);
      btnSend->style()->polish(btnSend);
      btnCfg->style()->unpolish(btnCfg);
      btnCfg->style()->polish(btnCfg);

      btnSend->setMinimumWidth(50);
      btnCfg->setFixedWidth(36);
      btnCfg->setFixedHeight(31);
      btnCfg->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
      btnCfg->setText("");
      btnCfg->setIcon(QIcon(":/Images/setting.svg"));
      btnCfg->setIconSize(QSize(24, 24));

      connect(btnSend, &QPushButton::clicked,
              [this, i]() { onMacroClicked(i); });
      connect(btnCfg, &QPushButton::clicked,
              [this, i]() { configureMacro(i); });

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
      if (!ms.name.isEmpty())
        btnSend->setText(ms.name);
    }
  }

  for (int i = 1; i <= 12; ++i) {
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_F1 + i - 1), this);
    connect(shortcut, &QShortcut::activated, [this, i]() {
      if (isConnected)
        onMacroClicked(i);
    });
  }

  connect(ui->tablePackets, &QTableWidget::cellDoubleClicked, this,
          &ConnectionTab::onTableDoubleClicked);

  connect(ui->tabSettings, &QTabWidget::currentChanged, [this](int index) {
    if (isConnected)
      return;
    (index == 1) ? ui->btnConnect->setEnabled(true) : refreshSerialPorts();
  });

  connect(ui->btnAutoTriggers, &QPushButton::clicked, this,
          &ConnectionTab::openTriggerConfigDialog);
  connect(ui->btnChecksum, &QPushButton::clicked, this,
          &ConnectionTab::openChecksumCalculator);

  connect(
      ui->txtFilter, &QLineEdit::textChanged, [this](const QString &filter) {
        QString filterLower = filter.toLower();
        for (int row = 0; row < ui->tablePackets->rowCount(); ++row) {
          bool match = filter.isEmpty();

          if (!match) {
            for (int col = 0; col < ui->tablePackets->columnCount(); ++col) {
              QTableWidgetItem *item = ui->tablePackets->item(row, col);
              if (item && item->text().toLower().contains(filterLower)) {
                match = true;
                break;
              }
            }
          }

          ui->tablePackets->setRowHidden(row, !match);
        }
      });
}

ConnectionTab::~ConnectionTab() {
  if (m_handler)
    delete m_handler;
  delete ui;
}

/**
 * @brief Initializes UI elements with default values.
 *
 * Configures serial port parameters, baud rates, and table properties.
 */
void ConnectionTab::setupUiDefaults() {
  refreshSerialPorts();

  // Populate Baud Rates
  ui->comboBaudRate->clear();
  QStringList bauds = {"1200",  "2400",   "4800",   "9600",   "19200", "38400",
                       "57600", "115200", "230400", "460800", "921600"};
  for (const QString &b : bauds)
    ui->comboBaudRate->addItem(b, b.toInt());
  ui->comboBaudRate->setCurrentText("115200");
  ui->comboBaudRate->setEditable(true);

  updateCounters(0, 0);

  ui->tablePackets->setColumnWidth(0, 100);
  ui->tablePackets->setColumnWidth(1, 50);
  ui->tablePackets->setColumnWidth(2, 400);
  ui->tablePackets->horizontalHeader()->setStretchLastSection(true);
  ui->tablePackets->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tablePackets->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->tablePackets->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePackets->setFocusPolicy(Qt::NoFocus);

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

void ConnectionTab::refreshSerialPorts() {
  const auto infos = QSerialPortInfo::availablePorts();

  // UI Toggling Logic
  bool hasPorts = !infos.isEmpty();

  ui->grpSerial->setVisible(hasPorts);
  ui->grpParams->setVisible(hasPorts);
  ui->grpPins->setVisible(hasPorts);
  ui->lblNoPorts->setVisible(!hasPorts);

  // Disable Connect button if in Serial Tab and no ports (and not already
  // connected)
  if (!isConnected && ui->tabSettings->currentIndex() == 0) {
    ui->btnConnect->setEnabled(hasPorts);
  }

  if (!hasPorts)
    return;

  QString currentPort =
      ui->comboPort->currentData().toString(); // Use stored data

  // Only update if list changed to avoid UI flickering/resetting user selection
  // while interacting Simple heuristic: compare counts or check if current
  // invalid. For smoothness, we can just rebuild.

  // To avoid resetting selection if the port still exists:
  QStringList currentItems;
  for (int i = 0; i < ui->comboPort->count(); i++)
    currentItems << ui->comboPort->itemData(i).toString();

  QStringList newItems;
  for (const QSerialPortInfo &info : infos)
    newItems << info.portName();

  // If lists are identical, do nothing (prevents closing popup)
  if (currentItems == newItems)
    return;

  ui->comboPort->blockSignals(true);
  ui->comboPort->clear();
  for (const QSerialPortInfo &info : infos) {
    ui->comboPort->addItem("🔌 " + info.portName(), info.portName());
  }

  // Restore selection if still available
  int idx = ui->comboPort->findData(currentPort);
  if (idx != -1)
    ui->comboPort->setCurrentIndex(idx);
  else if (ui->comboPort->count() > 0)
    ui->comboPort->setCurrentIndex(0);

  ui->comboPort->blockSignals(false);
}

/**
 * @brief Handles the Connect button click.
 *
 * Establishes a connection based on the selected mode (Serial, TCP Client, TCP
 * Server, or UDP). Initializes the appropriate communication handler and
 * connects its signals.
 */
void ConnectionTab::on_btnConnect_clicked() {
  if (isConnected) {
    on_btnDisconnect_clicked();
    return;
  }

  if (m_handler) {
    delete m_handler;
    m_handler = nullptr;
  }

  int pIndex = ui->tabSettings->currentIndex();
  bool initSuccess = false;

  if (pIndex == 0) {
    m_handler = new SerialQT();
  } else {
    QString netType = ui->comboNetProto->currentText();
    if (netType.contains("TCP Client")) {
      m_handler = new TcpClient();
    } else if (netType.contains("TCP Server")) {
      m_handler = new TcpServer_SingleClient();
    } else if (netType.contains("UDP")) {
      m_handler = new Udp();
    }
  }

  if (!m_handler)
    return;

  connect(m_handler, &AbstractCommunicationHandler::connected, this,
          &ConnectionTab::onConnected);
  connect(m_handler, &AbstractCommunicationHandler::disconnected, this,
          &ConnectionTab::onDisconnected);
  connect(m_handler, &AbstractCommunicationHandler::error, this,
          &ConnectionTab::onError);
  connect(m_handler, &AbstractCommunicationHandler::receivedData, this,
          &ConnectionTab::onDataReceived);

  if (pIndex == 0) {
    // Serial Init
    QString portName = ui->comboPort->currentData().toString();
    if (portName.isEmpty())
      portName = ui->comboPort->currentText();

    int baudRate = ui->comboBaudRate->currentData().toInt();
    if (baudRate == 0)
      baudRate = ui->comboBaudRate->currentText().toInt();

    int dataBits = ui->comboDataBits->currentData().toInt();
    int parity = ui->comboParity->currentData().toInt();
    int stopBits = ui->comboStopBits->currentData().toInt();
    int flowControl = ui->comboFlowControl->currentData().toInt();

    SerialQT *serial = static_cast<SerialQT *>(m_handler);
    initSuccess = serial->initialize(portName, baudRate, dataBits, parity,
                                     stopBits, flowControl);

  } else {
    QString netType = ui->comboNetProto->currentText();
    QString ip = ui->txtIpAddress->text().trimmed();
    int port = ui->spinPort->value();

    if (netType.contains("UDP")) {
      QHostAddress testAddr;
      if (ip.isEmpty() || !testAddr.setAddress(ip)) {
        showCustomMessage(
            "Invalid Configuration",
            "UDP requires a valid IP Address.\nExample: 127.0.0.1", true);
        return;
      }
    } else if (netType.contains("TCP Client") && ip.isEmpty()) {
      showCustomMessage("Invalid Configuration",
                        "Please enter a Hostname or IP Address.", true);
      return;
    }

    if (port <= 0 || port > 65535) {
      showCustomMessage("Invalid Configuration",
                        "Port must be between 1 and 65535.", true);
      return;
    }

    if (netType.contains("TCP Client")) {
      TcpClient *tcp = static_cast<TcpClient *>(m_handler);
      initSuccess = tcp->initialize(ip, port);
    } else if (netType.contains("TCP Server")) {
      TcpServer_SingleClient *svr =
          static_cast<TcpServer_SingleClient *>(m_handler);
      initSuccess = svr->initialize(port);
    } else if (netType.contains("UDP")) {
      Udp *udp = static_cast<Udp *>(m_handler);
      initSuccess = udp->initialize(ip, port);
    }
  }

  if (!initSuccess) {
    if (m_handler) {
      m_handler->disconnect(this);
      delete m_handler;
      m_handler = nullptr;
    }

    if (pIndex == 0) {
      showCustomMessage(
          "Connection Failed",
          "Could not open serial port.\nCheck if port is already in use.",
          true);
    } else {
      QString netType = ui->comboNetProto->currentText();
      if (netType.contains("TCP Server")) {
        showCustomMessage("Server Error",
                          "Could not start server on port " +
                              QString::number(ui->spinPort->value()) +
                              ".\nPort might be in use or restricted.",
                          true);
      } else {
        showCustomMessage("Connection Failed",
                          "Could not connect to host.\nCheck IP/Port.\n(Error "
                          "Code 0 = Connection Refused, 5 = Timeout)",
                          true);
      }
    }
  } else {
    if (pIndex == 1 &&
        ui->comboNetProto->currentText().contains("TCP Client")) {
      ui->btnConnect->setEnabled(false);
      ui->btnConnect->setText("CONNECTING...");
      ui->btnConnect->setStyleSheet("background-color: #FFA500; color: black; "
                                    "border: 1px solid #CC8400;");
    }
  }
}

/**
 * @brief Handles disconnect button click and closes the connection.
 */
void ConnectionTab::on_btnDisconnect_clicked() {
  if (m_handler) {
    m_handler->close();
    onDisconnected();
    delete m_handler;
    m_handler = nullptr;
  } else {
    onDisconnected();
  }
}

void ConnectionTab::on_btnSend_clicked() { sendPacket(); }

void ConnectionTab::on_chkAutoSend_toggled(bool checked) {
  if (!isConnected) {
    if (checked) {
      ui->chkAutoSend->setChecked(false); // Revert if not connected
      showCustomMessage("Not Connected",
                        "Please connect before starting auto-send.", true);
    }
    return;
  }

  if (checked) {
    int interval = ui->spinInterval->value();
    m_isHighPerformanceMode = (interval < 50);

    if (m_isHighPerformanceMode) {
      m_cachedSendData = getPacketData();
      if (ui->chkCR->isChecked())
        m_cachedSendData.append('\r');
      if (ui->chkLF->isChecked())
        m_cachedSendData.append('\n');

      if (m_cachedSendData.isEmpty()) {
        showCustomMessage("Empty Payload", "Nothing to send. Enter data first.",
                          true);
        ui->chkAutoSend->setChecked(false);
        return;
      }

      m_packetBuffer.clear();
      m_uiRefreshTimer->start();
      m_perfPacketCount = 0;
      m_perfTimer.start();
    }

    m_autoSendTimer->start(interval);
  } else {
    if (m_autoSendTimer->isActive())
      m_autoSendTimer->stop();

    // Stop high-performance mode
    if (m_isHighPerformanceMode) {
      m_uiRefreshTimer->stop();
      flushPacketBufferToTable(); // Flush remaining packets

      // Report performance
      qint64 elapsedMs = m_perfTimer.elapsed();
      if (elapsedMs > 0 && m_perfPacketCount > 0) {
        double pps = (double)m_perfPacketCount * 1000.0 / (double)elapsedMs;
        qDebug() << "[HighPerf] Sent" << m_perfPacketCount << "packets in"
                 << elapsedMs << "ms (" << pps << "pkt/s)";
      }

      m_isHighPerformanceMode = false;
      m_cachedSendData.clear();
    }
  }
}

/**
 * @brief Sends a packet through the active connection.
 * @param overrideData Optional data to send instead of the requested UI
 * payload.
 *
 * If overrideData is empty, constructs the packet from the UI fields (SOF,
 * Payload, EOF) or Raw Hex field depending on the selected Packet Mode. Appends
 * CRLF if configured. Logging and Tx counting are also handled here.
 */
// Helper to check if string is valid hex
bool isHexString(const QString &s) {
  for (QChar c : s) {
    if (!c.isDigit() && (c.toUpper() < 'A' || c.toUpper() > 'F'))
      return false;
  }
  return true;
}

void ConnectionTab::sendPacket(QByteArray overrideData) {
  if (!isConnected || !m_handler)
    return;

  QByteArray dataToSend;

  if (!overrideData.isEmpty()) {
    dataToSend = overrideData;
  } else {
    dataToSend = getPacketData();

    // Edge Case: Logic for Empty Data
    if (dataToSend.isEmpty()) {
      QString text = ui->txtPayload->text().trimmed();
      if (!text.isEmpty() && rbInputHex->isChecked()) {
        // Text exists but data is empty -> Invalid Hex
        showCustomMessage("Invalid Hex",
                          "The payload contains non-hexadecimal "
                          "characters.\nAllowed: 0-9, A-F, Spaces.",
                          true);
        return;
      } else if (text.isEmpty()) {
        // Just empty input, do nothing or maybe flash?
        // For now, silent return is standard behavior (don't annoy user if they
        // accidentally click)
        return;
      }
    }
  }

  // Check Line Endings
  if (ui->chkCR->isChecked())
    dataToSend.append('\r');
  if (ui->chkLF->isChecked())
    dataToSend.append('\n');

  if (dataToSend.isEmpty())
    return;

  m_handler->send(dataToSend);

  // Log to Table
  txCount += dataToSend.size();
  updateCounters(rxCount, txCount);
  addPacketToTable(true, dataToSend);

  writeLog(true, dataToSend);
}

QByteArray ConnectionTab::getPacketData() {
  QString text = ui->txtPayload->text();

  if (rbInputHex->isChecked()) {
    QString raw = text;
    raw.remove(' ');
    raw.remove("0x", Qt::CaseInsensitive);

    if (!isHexString(raw)) {
      return QByteArray();
    }

    return QByteArray::fromHex(raw.toLatin1());
  } else if (rbInputBinary->isChecked()) {
    QString raw = text;
    raw.remove(' ');

    for (QChar c : raw) {
      if (c != '0' && c != '1')
        return QByteArray();
    }

    QByteArray result;
    for (int i = 0; i < raw.length(); i += 8) {
      QString chunk = raw.mid(i, 8);
      if (chunk.length() < 8)
        chunk = chunk.leftJustified(8, '0'); // Pad if needed
      bool ok;
      char byte = static_cast<char>(chunk.toInt(&ok, 2));
      if (ok)
        result.append(byte);
    }
    return result;
  } else {
    return text.toUtf8();
  }
}

/**
 * @brief Converts QByteArray to binary string with space separators.
 * @param data Input data
 * @return Binary string representation
 */
QString byteArrayToBinaryString(const QByteArray &data) {
  QString result;
  for (int i = 0; i < data.size(); ++i) {
    unsigned char byte = static_cast<unsigned char>(data.at(i));
    result += QString("%1").arg(byte, 8, 2, QChar('0'));
    if (i < data.size() - 1)
      result += ' ';
  }
  return result;
}

void ConnectionTab::convertInputFormat(int toFormat) {
  QString currentText = ui->txtPayload->text();
  if (currentText.isEmpty())
    return;

  QByteArray rawData;
  QString trimmed = currentText.simplified();
  static QRegularExpression binaryRegex("^[01\\s]+$");
  static QRegularExpression hexRegex("^[0-9A-Fa-f\\s]+$");

  QString cleanText = currentText;
  cleanText.remove(' ');

  if (binaryRegex.match(currentText).hasMatch() && cleanText.length() >= 8) {
    // Likely binary - decode from binary
    QString raw = cleanText;
    for (int i = 0; i < raw.length(); i += 8) {
      QString chunk = raw.mid(i, 8);
      if (chunk.length() < 8)
        chunk = chunk.leftJustified(8, '0');
      bool ok;
      char byte = static_cast<char>(chunk.toInt(&ok, 2));
      if (ok)
        rawData.append(byte);
    }
  } else if (hexRegex.match(currentText).hasMatch() &&
             cleanText.length() % 2 == 0 && cleanText.length() >= 2) {
    // Likely hex - decode from hex
    rawData = QByteArray::fromHex(cleanText.toLatin1());
  } else {
    // Assume ASCII
    rawData = currentText.toUtf8();
  }

  if (rawData.isEmpty())
    return;

  // Convert to the target format
  switch (toFormat) {
  case 0: { // ASCII
    QString result;
    for (char c : rawData) {
      if (QChar(c).isPrint())
        result.append(c);
      else
        result.append('.');
    }
    ui->txtPayload->setText(result);
    break;
  }
  case 1: { // HEX
    QString hex = rawData.toHex(' ').toUpper();
    ui->txtPayload->setText(hex);
    break;
  }
  case 2: { // BINARY
    QString binary = byteArrayToBinaryString(rawData);
    ui->txtPayload->setText(binary);
    break;
  }
  }
}

/**
 * @brief Handles incoming data from the communication handler.
 * @param data Received data bytes
 */
void ConnectionTab::onDataReceived(QByteArray data) {
  writeLog(false, data);
  rxCount += data.size();
  updateCounters(rxCount, txCount);
  addPacketToTable(false, data);
  processAutoTriggers(data);
}

/**
 * @brief Adds a packet to the traffic log table with all format views.
 * @param isTx true for transmitted packets, false for received
 * @param data Packet data
 */
void ConnectionTab::addPacketToTable(bool isTx, const QByteArray &data) {
  int row = ui->tablePackets->rowCount();
  ui->tablePackets->insertRow(row);

  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  ui->tablePackets->setItem(row, 0, new QTableWidgetItem(timeStr));

  QTableWidgetItem *itemDir = new QTableWidgetItem(isTx ? "TX" : "RX");
  itemDir->setForeground(isTx ? QBrush(QColor("#2196F3"))
                              : QBrush(QColor("#F44336")));
  ui->tablePackets->setItem(row, 1, itemDir);

  QString hex = data.toHex(' ').toUpper();
  QTableWidgetItem *itemHex = new QTableWidgetItem(hex);
  itemHex->setData(Qt::UserRole, data);
  ui->tablePackets->setItem(row, 2, itemHex);

  QString decimal;
  for (int i = 0; i < data.size(); ++i) {
    if (i > 0)
      decimal += ' ';
    decimal += QString::number(static_cast<unsigned char>(data.at(i)));
  }
  ui->tablePackets->setItem(row, 3, new QTableWidgetItem(decimal));

  QString ascii = formatAsciiWithMnemonics(data);
  ui->tablePackets->setItem(row, 4, new QTableWidgetItem(ascii));

  ui->tablePackets->scrollToBottom();
}

void ConnectionTab::on_btnSendFile_clicked() {
  if (!isConnected) {
    QMessageBox::warning(this, "Not Connected", "Please connect first.");
    return;
  }

  QString fileName = QFileDialog::getOpenFileName(this, "Send File");
  if (fileName.isEmpty())
    return;

  QFile file(fileName);
  if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    m_handler->send(data);
    txCount += data.size();
    updateCounters(rxCount, txCount);
    QMessageBox::information(
        this, "Sent", QString("Sent %1 bytes form file.").arg(data.size()));
  }
}

void ConnectionTab::updateCounters(int rx, int tx) {
  ui->lblRxCount->setText(QString("Rx: %1").arg(rx));
  ui->lblTxCount->setText(QString("Tx: %1").arg(tx));
}

void ConnectionTab::on_btnClearRx_clicked() {
  ui->tablePackets->setRowCount(0);
  rxCount = 0;
  txCount = 0;
  updateCounters(rxCount, txCount);
}

void ConnectionTab::onAutoSendTimerTimeout() {
  if (m_isHighPerformanceMode) {
    if (!isConnected || !m_handler || m_cachedSendData.isEmpty())
      return;

    m_handler->send(m_cachedSendData);

    {
      QMutexLocker lock(&m_bufferMutex);
      BufferedPacket pkt;
      pkt.isTx = true;
      pkt.data = m_cachedSendData;
      pkt.timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
      m_packetBuffer.append(pkt);
    }

    // Update counters (lightweight)
    txCount += m_cachedSendData.size();
    m_perfPacketCount++;

    // Update counter display less frequently (every 10 packets)
    if (m_perfPacketCount % 10 == 0) {
      updateCounters(rxCount, txCount);
    }

    // Write to log file (lightweight, no UI)
    writeLog(true, m_cachedSendData);
  } else {
    // Standard path
    sendPacket();
  }
}

// Window dragging removed - handled by MainWindow title bar only

void ConnectionTab::showCustomMessage(const QString &title, const QString &text,
                                      bool isError) {
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

void ConnectionTab::onError(int err) {
  // Re-enable button if we were stuck in connecting state
  if (ui->btnConnect->text() == "CONNECTING...") {
    ui->btnConnect->setEnabled(true);
    ui->btnConnect->setText("CONNECT");
    ui->btnConnect->setStyleSheet("");
  }

  // Map socket errors to human readable string if possible, or just show code
  QString errStr = "Unknown Error";
  if (err == 0)
    errStr = "Connection Refused (0)";
  else if (err == 2)
    errStr = "Host Not Found (2)";
  else if (err == 5)
    errStr = "Operation Timed Out (5)";
  else
    errStr = "Error Code: " + QString::number(err);

  showCustomMessage("Connection Error", "An error occurred.\n" + errStr, true);

  // Disconnect logic cleans up
  on_btnDisconnect_clicked();
}

void ConnectionTab::onConnected() {
  isConnected = true;
  ui->btnConnect->setChecked(true);
  ui->btnConnect->setText("CONNECTED");
  ui->btnConnect->setStyleSheet(
      "background-color: #006400; border: 1px solid #008000;");
  ui->btnDisconnect->setEnabled(true);

  // Disable Settings Tabs
  ui->tabSettings->setEnabled(false);

  // Enable Transmit & Macros
  ui->grpTransmit->setEnabled(true);
  ui->grpMacros->setEnabled(true);
}

void ConnectionTab::onDisconnected() {
  isConnected = false;
  ui->btnConnect->setChecked(false);
  ui->btnConnect->setText("CONNECT");
  ui->btnConnect->setStyleSheet("");
  ui->btnConnect->setEnabled(true); // CRITICAL: Re-enable Connect button
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

void ConnectionTab::closeEvent(QCloseEvent *event) {
  QSettings settings("PacketForge", "PacketTransmitter");
  // settings.setValue("geometry", saveGeometry()); // Handled by MainWindow

  // Save Macros
  for (auto it = m_macros.begin(); it != m_macros.end(); ++it) {
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

void ConnectionTab::configureMacro(int index) {
  MacroSettings current = m_macros[index];
  MacroDialog dlg(current, this);
  if (dlg.exec() == QDialog::Accepted) {
    m_macros[index] = dlg.getSettings();

    // Update Button Text
    QPushButton *btn =
        this->findChild<QPushButton *>("btnM" + QString::number(index));
    if (btn)
      btn->setText(m_macros[index].name);
  }
}

void ConnectionTab::onMacroClicked(int index) {
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
        if (toSend.isEmpty() && !s.data.isEmpty())
          toSend = s.data.toUtf8();

        sendPacket(toSend);
      });
    }

    QTimer *timer = m_macroTimers[index];
    QPushButton *btn =
        this->findChild<QPushButton *>("btnM" + QString::number(index));

    if (timer->isActive()) {
      timer->stop();
      // Visual indication stopped
      if (btn) {
        btn->setStyleSheet(
            "QPushButton { border-top-right-radius: 0px; "
            "border-bottom-right-radius: 0px; border-right: 0px; margin-right: "
            "0px; }"); // Revert to default split style
      }
      ui->grpTransmit->setEnabled(true); // Re-enable manual transmit
    } else {
      timer->start(s.intervalMs);
      // Visual indication started
      if (btn) {
        btn->setStyleSheet(
            "QPushButton { background-color: #2e7d32; color: white; border: "
            "1px solid #4caf50; border-top-right-radius: 0px; "
            "border-bottom-right-radius: 0px; border-right: 0px; margin-right: "
            "0px; }");
      }
      ui->grpTransmit->setEnabled(
          false); // Disable manual transmit to prevent conflict
    }

  } else {
    // Send Macro Data (One-shot)
    QByteArray dataToSend;

    // Smart Detect: If contains ONLY hex chars and spaces, treat as hex
    // Ignoring packetMode 1 check in favor of Smart Detect for better UX on
    // "Hello"
    static QRegularExpression hexRegex("^[0-9A-Fa-f\\s]*$");
    if (hexRegex.match(s.data).hasMatch()) {
      // It's pure hex
      if (s.packetMode == 0) { // Structured
        QString payloadHex = s.data;
        payloadHex.remove(QRegularExpression("[^0-9A-Fa-f]"));
        QString sofHex = s.sof;
        sofHex.remove(QRegularExpression("[^0-9A-Fa-f]"));
        QString eofHex = s.eof;
        eofHex.remove(QRegularExpression("[^0-9A-Fa-f]"));

        if (!sofHex.isEmpty())
          dataToSend.append(QByteArray::fromHex(sofHex.toLatin1()));
        dataToSend.append(QByteArray::fromHex(payloadHex.toLatin1()));
        if (!eofHex.isEmpty())
          dataToSend.append(QByteArray::fromHex(eofHex.toLatin1()));
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

void ConnectionTab::on_chkLogToFile_toggled(bool checked) {
  if (checked) {
    QString logsPath = LOG_FOLDER_PATH;
    QDir dir(logsPath);
    if (!dir.exists()) {
      dir.mkpath(".");
    }

    QString timestamp =
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    bool htmlMode = ui->chkHtmlLog->isChecked();
    QString ext = htmlMode ? ".html" : ".txt";
    QString fileName =
        QString("%1/PacketLog_%2%3").arg(logsPath, timestamp, ext);

    m_logFile.setFileName(fileName);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
      m_logStream.setDevice(&m_logFile);

      // Write HTML header if in HTML mode
      if (htmlMode) {
        m_logStream << R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>PacketForge Log</title>
<style>
body { font-family: 'Consolas', monospace; background-color: #1e1e1e; color: #f0f0f0; padding: 20px; }
.tx { color: #2196F3; }
.rx { color: #F44336; }
.time { color: #9E9E9E; }
.hex { color: #4CAF50; }
pre { margin: 2px 0; }
</style>
</head>
<body>
<h2>PacketForge Traffic Log</h2>
<p>Started: )" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
                    << R"(</p>
<hr>
)";
      }

      ui->chkLogToFile->setText(
          tr("Logging to %1").arg(QFileInfo(fileName).fileName()));
    } else {
      showCustomMessage("Logging Error", "Could not create log file.", true);
      ui->chkLogToFile->setChecked(false); // Revert
    }
  } else {
    if (m_logFile.isOpen()) {
      // Write HTML footer if in HTML mode
      if (ui->chkHtmlLog->isChecked()) {
        m_logStream << R"(
<hr>
<p>Ended: )" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
                    << R"(</p>
</body>
</html>)";
      }
      m_logFile.close();
      ui->chkLogToFile->setText("Log to File");
    }
  }
}

void ConnectionTab::writeLog(bool isTx, const QByteArray &data) {
  emit logData(isTx, data);

  if (!m_logFile.isOpen())
    return;

  QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  QString direction = isTx ? "TX" : "RX";

  // Format: Timestamp [TX] HEX_DATA (ASCII)
  QString hex = data.toHex(' ').toUpper();

  // Create a safe ASCII representation with mnemonics
  QString ascii = formatAsciiWithMnemonics(data);

  if (ui->chkHtmlLog->isChecked()) {
    // HTML format with colors
    QString dirClass = isTx ? "tx" : "rx";
    m_logStream << QString("<pre><span class=\"time\">[%1]</span> <span "
                           "class=\"%2\">[%3]</span> <span "
                           "class=\"hex\">%4</span>  (%5)</pre>\n")
                       .arg(timestamp, dirClass, direction, hex.toHtmlEscaped(),
                            ascii.toHtmlEscaped());
  } else {
    // Plain text format
    m_logStream << "[" << timestamp << "] [" << direction << "] " << hex
                << "  (" << ascii << ")\n";
  }
  m_logStream.flush();
}

void ConnectionTab::onTableDoubleClicked(int row, int column) {
  QTableWidgetItem *item = ui->tablePackets->item(row, column);
  if (!item)
    return;

  // Try to get UserRole data (original raw bytes)
  QByteArray data = item->data(Qt::UserRole).toByteArray();

  // If empty (e.g. user clicked Timestamp or Dir column), try to find the Hex
  // column (index 2)
  if (data.isEmpty()) {
    QTableWidgetItem *hexItem = ui->tablePackets->item(row, 2);
    if (hexItem) {
      data = hexItem->data(Qt::UserRole).toByteArray();
    }
  }

  if (data.isEmpty())
    return;

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
  // Provide raw latin1 view to preserve 1-byte-per-char visual as much as
  // possible
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

// --- High Performance Mode Helpers ---

QString ConnectionTab::formatAsciiWithMnemonics(const QByteArray &data) {
  QString result;
  result.reserve(data.size() * 2); // Pre-allocate for efficiency

  for (int i = 0; i < data.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(data.at(i));

    switch (c) {
    case 0x00:
      result += "<NUL>";
      break;
    case 0x01:
      result += "<SOH>";
      break;
    case 0x02:
      result += "<STX>";
      break;
    case 0x03:
      result += "<ETX>";
      break;
    case 0x04:
      result += "<EOT>";
      break;
    case 0x05:
      result += "<ENQ>";
      break;
    case 0x06:
      result += "<ACK>";
      break;
    case 0x07:
      result += "<BEL>";
      break;
    case 0x08:
      result += "<BS>";
      break;
    case 0x09:
      result += "<TAB>";
      break;
    case 0x0A:
      result += "<LF>";
      break;
    case 0x0B:
      result += "<VT>";
      break;
    case 0x0C:
      result += "<FF>";
      break;
    case 0x0D:
      result += "<CR>";
      break;
    case 0x0E:
      result += "<SO>";
      break;
    case 0x0F:
      result += "<SI>";
      break;
    case 0x10:
      result += "<DLE>";
      break;
    case 0x11:
      result += "<DC1>";
      break;
    case 0x12:
      result += "<DC2>";
      break;
    case 0x13:
      result += "<DC3>";
      break;
    case 0x14:
      result += "<DC4>";
      break;
    case 0x15:
      result += "<NAK>";
      break;
    case 0x16:
      result += "<SYN>";
      break;
    case 0x17:
      result += "<ETB>";
      break;
    case 0x18:
      result += "<CAN>";
      break;
    case 0x19:
      result += "<EM>";
      break;
    case 0x1A:
      result += "<SUB>";
      break;
    case 0x1B:
      result += "<ESC>";
      break;
    case 0x1C:
      result += "<FS>";
      break;
    case 0x1D:
      result += "<GS>";
      break;
    case 0x1E:
      result += "<RS>";
      break;
    case 0x1F:
      result += "<US>";
      break;
    case 0x7F:
      result += "<DEL>";
      break;
    default:
      if (c >= 32 && c <= 126) {
        result += QChar(c);
      } else {
        // Non-printable high bytes: show as hex
        result += QString("<%1>").arg(c, 2, 16, QChar('0')).toUpper();
      }
      break;
    }
  }
  return result;
}

void ConnectionTab::flushPacketBufferToTable() {
  QVector<BufferedPacket> localBuffer;

  // Move packets from shared buffer to local copy (minimize lock time)
  {
    QMutexLocker lock(&m_bufferMutex);
    localBuffer = std::move(m_packetBuffer);
    m_packetBuffer.clear();
    m_packetBuffer.reserve(100); // Pre-allocate for next batch
  }

  if (localBuffer.isEmpty())
    return;

  // Limit display to avoid UI overload (show last N packets)
  const int MAX_DISPLAY = 50;
  int startIdx = qMax(0, localBuffer.size() - MAX_DISPLAY);

  // Batch insert rows for efficiency
  ui->tablePackets->setUpdatesEnabled(false);

  for (int i = startIdx; i < localBuffer.size(); ++i) {
    const BufferedPacket &pkt = localBuffer[i];

    int row = ui->tablePackets->rowCount();
    ui->tablePackets->insertRow(row);

    // 1. Time
    ui->tablePackets->setItem(row, 0, new QTableWidgetItem(pkt.timestamp));

    // 2. Dir (TX = Blue, RX = Red - Docklight style)
    QTableWidgetItem *itemDir = new QTableWidgetItem(pkt.isTx ? "TX" : "RX");
    itemDir->setForeground(pkt.isTx ? QBrush(QColor("#2196F3"))
                                    : QBrush(QColor("#F44336")));
    ui->tablePackets->setItem(row, 1, itemDir);

    // 3. HEX
    QString hex = pkt.data.toHex(' ').toUpper();
    QTableWidgetItem *itemHex = new QTableWidgetItem(hex);
    itemHex->setData(Qt::UserRole, pkt.data);
    ui->tablePackets->setItem(row, 2, itemHex);

    // 4. Decimal
    QString decimal;
    for (int j = 0; j < pkt.data.size(); ++j) {
      if (j > 0)
        decimal += ' ';
      decimal += QString::number(static_cast<unsigned char>(pkt.data.at(j)));
    }
    ui->tablePackets->setItem(row, 3, new QTableWidgetItem(decimal));

    // 5. ASCII with mnemonics
    QString ascii = formatAsciiWithMnemonics(pkt.data);
    ui->tablePackets->setItem(row, 4, new QTableWidgetItem(ascii));
  }

  ui->tablePackets->setUpdatesEnabled(true);
  ui->tablePackets->scrollToBottom();

  // Update counters after batch
  updateCounters(rxCount, txCount);
}

// --- Auto-Answer Triggers Implementation ---

void ConnectionTab::processAutoTriggers(const QByteArray &data) {
  if (m_autoTriggers.isEmpty())
    return;

  // Accumulate received data for pattern matching
  m_rxAccumulator.append(data);

  // Limit accumulator size to prevent memory bloat (keep last 4KB)
  const int MAX_ACCUMULATOR_SIZE = 4096;
  if (m_rxAccumulator.size() > MAX_ACCUMULATOR_SIZE) {
    m_rxAccumulator = m_rxAccumulator.right(MAX_ACCUMULATOR_SIZE / 2);
  }

  // Check each trigger
  for (const AutoTrigger &trigger : m_autoTriggers) {
    if (!trigger.enabled || trigger.pattern.isEmpty())
      continue;

    int idx = m_rxAccumulator.indexOf(trigger.pattern);
    if (idx != -1) {
      // Pattern matched! Send response
      qDebug() << "[AutoTrigger] Pattern matched:" << trigger.name;

      // Remove matched portion to prevent re-triggering
      m_rxAccumulator = m_rxAccumulator.mid(idx + trigger.pattern.size());

      // Send response (with optional delay)
      if (trigger.delayMs > 0) {
        QTimer::singleShot(trigger.delayMs, this, [this, trigger]() {
          if (isConnected && m_handler) {
            sendPacket(trigger.response);
          }
        });
      } else {
        sendPacket(trigger.response);
      }
    }
  }
}

void ConnectionTab::openTriggerConfigDialog() {
  QDialog *dlg = new QDialog(this);
  dlg->setWindowTitle("Auto-Answer Triggers (Receive Sequences)");
  dlg->resize(700, 500);

  QVBoxLayout *mainLayout = new QVBoxLayout(dlg);

  // Info Label
  QLabel *lblInfo =
      new QLabel("Configure patterns to watch for. When a pattern is detected, "
                 "the response will be sent automatically.",
                 dlg);
  lblInfo->setWordWrap(true);
  mainLayout->addWidget(lblInfo);

  // Trigger List
  QTableWidget *tblTriggers = new QTableWidget(0, 5, dlg);
  tblTriggers->setHorizontalHeaderLabels(
      {"Enabled", "Name", "Pattern (Hex)", "Response (Hex)", "Delay (ms)"});
  tblTriggers->horizontalHeader()->setStretchLastSection(true);
  tblTriggers->setColumnWidth(0, 60);
  tblTriggers->setColumnWidth(1, 100);
  tblTriggers->setColumnWidth(2, 180);
  tblTriggers->setColumnWidth(3, 180);
  mainLayout->addWidget(tblTriggers);

  // Populate existing triggers
  for (const AutoTrigger &t : m_autoTriggers) {
    int row = tblTriggers->rowCount();
    tblTriggers->insertRow(row);

    QCheckBox *chk = new QCheckBox();
    chk->setChecked(t.enabled);
    tblTriggers->setCellWidget(row, 0, chk);

    tblTriggers->setItem(row, 1, new QTableWidgetItem(t.name));
    tblTriggers->setItem(row, 2,
                         new QTableWidgetItem(t.pattern.toHex(' ').toUpper()));
    tblTriggers->setItem(row, 3,
                         new QTableWidgetItem(t.response.toHex(' ').toUpper()));
    tblTriggers->setItem(row, 4,
                         new QTableWidgetItem(QString::number(t.delayMs)));
  }

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  QPushButton *btnAdd = new QPushButton("Add Trigger", dlg);
  QPushButton *btnRemove = new QPushButton("Remove Selected", dlg);
  QPushButton *btnSave = new QPushButton("Save & Close", dlg);
  btnLayout->addWidget(btnAdd);
  btnLayout->addWidget(btnRemove);
  btnLayout->addStretch();
  btnLayout->addWidget(btnSave);
  mainLayout->addLayout(btnLayout);

  // Add Trigger
  connect(btnAdd, &QPushButton::clicked, [tblTriggers]() {
    int row = tblTriggers->rowCount();
    tblTriggers->insertRow(row);

    QCheckBox *chk = new QCheckBox();
    chk->setChecked(true);
    tblTriggers->setCellWidget(row, 0, chk);

    tblTriggers->setItem(row, 1, new QTableWidgetItem("New Trigger"));
    tblTriggers->setItem(row, 2, new QTableWidgetItem(""));
    tblTriggers->setItem(row, 3, new QTableWidgetItem(""));
    tblTriggers->setItem(row, 4, new QTableWidgetItem("0"));
  });

  // Remove Selected
  connect(btnRemove, &QPushButton::clicked, [tblTriggers]() {
    int row = tblTriggers->currentRow();
    if (row >= 0)
      tblTriggers->removeRow(row);
  });

  // Save & Close
  connect(btnSave, &QPushButton::clicked, [this, dlg, tblTriggers]() {
    m_autoTriggers.clear();

    for (int row = 0; row < tblTriggers->rowCount(); ++row) {
      AutoTrigger t;

      QCheckBox *chk =
          qobject_cast<QCheckBox *>(tblTriggers->cellWidget(row, 0));
      t.enabled = chk ? chk->isChecked() : true;

      t.name =
          tblTriggers->item(row, 1) ? tblTriggers->item(row, 1)->text() : "";

      QString patternHex =
          tblTriggers->item(row, 2) ? tblTriggers->item(row, 2)->text() : "";
      patternHex.remove(' ');
      t.pattern = QByteArray::fromHex(patternHex.toLatin1());

      QString responseHex =
          tblTriggers->item(row, 3) ? tblTriggers->item(row, 3)->text() : "";
      responseHex.remove(' ');
      t.response = QByteArray::fromHex(responseHex.toLatin1());

      t.delayMs = tblTriggers->item(row, 4)
                      ? tblTriggers->item(row, 4)->text().toInt()
                      : 0;

      if (!t.pattern.isEmpty()) {
        m_autoTriggers.append(t);
      }
    }

    qDebug() << "[AutoTrigger] Saved" << m_autoTriggers.size() << "triggers";
    dlg->accept();
  });

  dlg->exec();
  delete dlg;
}

void ConnectionTab::openChecksumCalculator() {
  QDialog *dlg = new QDialog(this);
  dlg->setWindowTitle("Checksum Calculator");
  dlg->resize(500, 350);

  QVBoxLayout *layout = new QVBoxLayout(dlg);

  // Input
  QGroupBox *grpInput = new QGroupBox("Input Data (Hex)", dlg);
  QVBoxLayout *inputLayout = new QVBoxLayout(grpInput);
  QLineEdit *txtInput = new QLineEdit(grpInput);
  txtInput->setPlaceholderText("Enter hex data (e.g., 01 03 00 00 00 0A)");
  txtInput->setFont(QFont("Consolas", 10));
  inputLayout->addWidget(txtInput);
  layout->addWidget(grpInput);

  // Algorithm Selection
  QGroupBox *grpAlgo = new QGroupBox("Algorithm", dlg);
  QHBoxLayout *algoLayout = new QHBoxLayout(grpAlgo);
  QComboBox *cmbAlgo = new QComboBox(grpAlgo);
  cmbAlgo->addItems({"CRC16", "CRC32", "XOR", "LRC", "SUM8"});
  QPushButton *btnCalc = new QPushButton("Calculate", grpAlgo);
  algoLayout->addWidget(cmbAlgo);
  algoLayout->addWidget(btnCalc);
  layout->addWidget(grpAlgo);

  // Result
  QGroupBox *grpResult = new QGroupBox("Result", dlg);
  QVBoxLayout *resultLayout = new QVBoxLayout(grpResult);
  QLabel *lblResult = new QLabel("--", grpResult);
  lblResult->setFont(QFont("Consolas", 14, QFont::Bold));
  lblResult->setAlignment(Qt::AlignCenter);
  lblResult->setStyleSheet("color: #4CAF50; background-color: #1e1e1e; "
                           "padding: 15px; border-radius: 5px;");
  resultLayout->addWidget(lblResult);
  layout->addWidget(grpResult);

  // Calculate Logic
  connect(btnCalc, &QPushButton::clicked, [txtInput, cmbAlgo, lblResult]() {
    QString hexStr = txtInput->text();
    hexStr.remove(' ').remove("0x");
    QByteArray inputData = QByteArray::fromHex(hexStr.toLatin1());

    if (inputData.isEmpty()) {
      lblResult->setText("Invalid Hex Input");
      lblResult->setStyleSheet("color: #F44336; background-color: #1e1e1e; "
                               "padding: 15px; border-radius: 5px;");
      return;
    }

    QByteArray checksum =
        ConnectionTab::calculateChecksum(inputData, cmbAlgo->currentText());
    QString resultHex = checksum.toHex(' ').toUpper();
    lblResult->setText(resultHex);
    lblResult->setStyleSheet("color: #4CAF50; background-color: #1e1e1e; "
                             "padding: 15px; border-radius: 5px;");
  });

  // Close Button
  QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
  connect(btnBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
  layout->addWidget(btnBox);

  dlg->exec();
  delete dlg;
}

// --- Checksum Calculator Implementation ---

QByteArray ConnectionTab::calculateChecksum(const QByteArray &data,
                                            const QString &algorithm) {

  if (data.isEmpty())
    return QByteArray();

  if (algorithm == "XOR") {
    // Simple XOR of all bytes
    unsigned char xorResult = 0;
    for (char c : data) {
      xorResult ^= static_cast<unsigned char>(c);
    }
    return QByteArray(1, xorResult);
  } else if (algorithm == "SUM8") {
    // Sum of all bytes (mod 256)
    unsigned char sum = 0;
    for (char c : data) {
      sum += static_cast<unsigned char>(c);
    }
    return QByteArray(1, sum);
  } else if (algorithm == "LRC") {
    // Longitudinal Redundancy Check (2's complement of sum)
    unsigned char sum = 0;
    for (char c : data) {
      sum += static_cast<unsigned char>(c);
    }
    unsigned char lrc = (~sum) + 1;
    return QByteArray(1, lrc);
  } else if (algorithm == "CRC16") {
    // CRC-16/MODBUS (Polynomial 0x8005, Init 0xFFFF)
    quint16 crc = 0xFFFF;
    for (char c : data) {
      crc ^= static_cast<unsigned char>(c);
      for (int i = 0; i < 8; ++i) {
        if (crc & 0x0001) {
          crc = (crc >> 1) ^ 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    QByteArray result;
    result.append(
        static_cast<char>(crc & 0xFF)); // Low byte first (little-endian)
    result.append(static_cast<char>((crc >> 8) & 0xFF)); // High byte
    return result;
  } else if (algorithm == "CRC32") {
    // CRC-32 (Standard Ethernet/ZIP polynomial)
    quint32 crc = 0xFFFFFFFF;
    for (char c : data) {
      crc ^= static_cast<unsigned char>(c);
      for (int i = 0; i < 8; ++i) {
        if (crc & 1) {
          crc = (crc >> 1) ^ 0xEDB88320;
        } else {
          crc >>= 1;
        }
      }
    }
    crc ^= 0xFFFFFFFF;
    QByteArray result;
    result.append(static_cast<char>(crc & 0xFF));
    result.append(static_cast<char>((crc >> 8) & 0xFF));
    result.append(static_cast<char>((crc >> 16) & 0xFF));
    result.append(static_cast<char>((crc >> 24) & 0xFF));
    return result;
  }

  return QByteArray(); // Unknown algorithm
}
