/**
 * @file ModbusClientWidget.cpp
 * @brief Implementation of the Modbus client widget.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "ModbusClientWidget.h"
#include "ui_ModbusClientWidget.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QMessageBox>
#include <QVariant>
#include <QDebug>
#include <QTime>

ModbusClientWidget::ModbusClientWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModbusClientWidget),
    m_modbusDevice(nullptr),
    m_scanTimer(new QTimer(this))
{
    ui->setupUi(this);
    
    // Initialize ComboBoxes
    ui->comboBaud->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    ui->comboBaud->setCurrentText("115200");
    
    refreshSerialPorts();
    
    // Connect Signals
    connect(ui->btnConnect, &QPushButton::clicked, this, &ModbusClientWidget::on_btnConnect_clicked);
    connect(ui->btnRefreshPorts, &QPushButton::clicked, this, &ModbusClientWidget::on_btnRefreshPorts_clicked);
    connect(ui->btnRead, &QPushButton::clicked, this, &ModbusClientWidget::on_btnRead_clicked);
    connect(ui->btnWrite, &QPushButton::clicked, this, &ModbusClientWidget::on_btnWrite_clicked);
    connect(ui->chkAutoRead, &QCheckBox::toggled, this, &ModbusClientWidget::on_chkAutoRead_toggled);
    connect(ui->comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModbusClientWidget::on_comboMode_currentIndexChanged);
    connect(m_scanTimer, &QTimer::timeout, this, &ModbusClientWidget::on_btnRead_clicked);
    
    // Initial State
    on_comboMode_currentIndexChanged(0);
    ui->tableRegisters->horizontalHeader()->setStretchLastSection(true);
}

ModbusClientWidget::~ModbusClientWidget()
{
    if (m_modbusDevice)
        m_modbusDevice->disconnectDevice();
    delete m_modbusDevice;
    delete ui;
}

/**
 * @brief Updates the serial port list.
 */
void ModbusClientWidget::refreshSerialPorts()
{
    QString current = ui->comboPort->currentText();
    ui->comboPort->clear();
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        ui->comboPort->addItem(info.portName());
    }
    ui->comboPort->setCurrentText(current);
}

void ModbusClientWidget::on_btnRefreshPorts_clicked()
{
    refreshSerialPorts();
}

/**
 * @brief Handles mode selection changes (TCP vs RTU).
 * @param index 0 for TCP, 1 for RTU
 */
void ModbusClientWidget::on_comboMode_currentIndexChanged(int index)
{
    ui->stackSettings->setCurrentIndex(index);
}

/**
 * @brief Handles connection toggling.
 */
void ModbusClientWidget::on_btnConnect_clicked()
{
    if (m_modbusDevice) {
        m_modbusDevice->disconnectDevice();
        delete m_modbusDevice;
        m_modbusDevice = nullptr;
        ui->btnConnect->setText("🔗 Connect");
        ui->btnConnect->setChecked(false);
        ui->lblConnectionStatus->setText("⚪ Disconnected");
        logStatus("Disconnected");
        return;
    }

    if (ui->comboMode->currentIndex() == 0) {
        // TCP
        m_modbusDevice = new QModbusTcpClient(this);
        m_modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, ui->spinPort->value());
        m_modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ui->txtIp->text());
    } else {
        // RTU
        m_modbusDevice = new QModbusRtuSerialClient(this);
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, ui->comboPort->currentText());
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, ui->comboBaud->currentText().toInt());
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);
        
        // Parity
        QSerialPort::Parity parity = QSerialPort::NoParity;
        if (ui->comboParity->currentIndex() == 1) parity = QSerialPort::EvenParity;
        else if (ui->comboParity->currentIndex() == 2) parity = QSerialPort::OddParity;
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, parity);
    }

    m_modbusDevice->setTimeout(1000);
    m_modbusDevice->setNumberOfRetries(3);

    connect(m_modbusDevice, &QModbusClient::stateChanged, this, &ModbusClientWidget::onStateChanged);
    connect(m_modbusDevice, &QModbusClient::errorOccurred, this, &ModbusClientWidget::onErrorOccurred);

    if (!m_modbusDevice->connectDevice()) {
        logStatus("Connection failed: " + m_modbusDevice->errorString(), true);
        delete m_modbusDevice;
        m_modbusDevice = nullptr;
    } else {
         ui->lblConnectionStatus->setText("🟡 Connecting...");
    }
}

/**
 * @brief Handles Modbus device state changes.
 * @param state New state
 */
void ModbusClientWidget::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState) {
        ui->lblConnectionStatus->setText("🟢 Connected");
        ui->btnConnect->setText("❌ Disconnect");
        ui->btnConnect->setChecked(true);
        logStatus("Connected successfully");
    } else if (state == QModbusDevice::UnconnectedState) {
        ui->lblConnectionStatus->setText("⚪ Disconnected");
        ui->btnConnect->setText("🔗 Connect");
        ui->btnConnect->setChecked(false);
        m_scanTimer->stop();
        ui->chkAutoRead->setChecked(false);
    }
}

/**
 * @brief Handles Modbus errors.
 * @param error Error code
 */
void ModbusClientWidget::onErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError) return;
    logStatus(m_modbusDevice->errorString(), true);
}

/**
 * @brief Executes a read request.
 */
void ModbusClientWidget::on_btnRead_clicked()
{
    if (!m_modbusDevice || m_modbusDevice->state() != QModbusDevice::ConnectedState) {
        logStatus("Not connected", true);
        return;
    }

    QModbusDataUnit::RegisterType type;
    int index = ui->comboRegType->currentIndex();
    switch (index) {
        case 0: type = QModbusDataUnit::Coils; break;
        case 1: type = QModbusDataUnit::DiscreteInputs; break;
        case 2: type = QModbusDataUnit::HoldingRegisters; break;
        default: type = QModbusDataUnit::InputRegisters; break;
    }

    int startAddr = ui->spinStartAddr->value();
    int count = ui->spinCount->value();
    int slaveId = ui->spinSlaveId->value();

    if (auto *reply = m_modbusDevice->sendReadRequest(QModbusDataUnit(type, startAddr, count), slaveId)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusClientWidget::onReadReady);
        else
            delete reply;
            
        m_txCount++;
        updateCounters();
    } else {
        logStatus("Read Request Error: " + m_modbusDevice->errorString(), true);
    }
}

/**
 * @brief Handles completed read requests.
 */
void ModbusClientWidget::onReadReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        
        ui->tableRegisters->setRowCount(unit.valueCount());
        for (int i = 0; i < unit.valueCount(); i++) {
            int addr = unit.startAddress() + i;
            uint16_t val = unit.value(i);
            
            ui->tableRegisters->setItem(i, 0, new QTableWidgetItem(QString::number(addr)));
            ui->tableRegisters->setItem(i, 1, new QTableWidgetItem(QString::number(val)));
            ui->tableRegisters->setItem(i, 2, new QTableWidgetItem("0x" + QString::number(val, 16).toUpper().rightJustified(4, '0')));
            ui->tableRegisters->setItem(i, 3, new QTableWidgetItem(QString("%1").arg(val, 16, 2, QChar('0'))));
            
            QString ascii = (val >= 32 && val <= 126) ? QString(QChar(val)) : ".";
            ui->tableRegisters->setItem(i, 4, new QTableWidgetItem(ascii));
        }
        
        m_rxCount++;
        updateCounters();
        logStatus("Read OK");
    } else {
         logStatus("Read Error: " + reply->errorString(), true);
    }
    
    reply->deleteLater();
}

/**
 * @brief Executes a write request.
 */
void ModbusClientWidget::on_btnWrite_clicked()
{
    if (!m_modbusDevice || m_modbusDevice->state() != QModbusDevice::ConnectedState) return;

    QModbusDataUnit writeUnit;
    int index = ui->comboWriteType->currentIndex();
    int addr = ui->spinWriteAddr->value();
    
    // Parse value (auto-detect hex/dec)
    QString valStr = ui->txtWriteValue->text();
    bool ok;
    quint16 val;
    if (valStr.startsWith("0x", Qt::CaseInsensitive))
         val = (quint16)valStr.toUInt(&ok, 16);
    else 
         val = (quint16)valStr.toUInt(&ok, 10);
         
    if (!ok) {
        logStatus("Invalid write value", true);
        return;
    }

    if (index == 0) { // Single Coil
        writeUnit = QModbusDataUnit(QModbusDataUnit::Coils, addr, 1);
        writeUnit.setValue(0, val ? 1 : 0);
    } else if (index == 1) { // Single Register
        writeUnit = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, addr, 1);
        writeUnit.setValue(0, val);
    } 
    // TODO: Implement Multiple Writes if needed

    int slaveId = ui->spinSlaveId->value();
    if (auto *reply = m_modbusDevice->sendWriteRequest(writeUnit, slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusClientWidget::onWriteReady);
        } else {
            reply->deleteLater();
        }
        m_txCount++;
        updateCounters();
    } else {
        logStatus("Write Request Error: " + m_modbusDevice->errorString(), true);
    }
}

/**
 * @brief Handles completed write requests.
 */
void ModbusClientWidget::onWriteReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        logStatus("Write OK");
        m_rxCount++;
        updateCounters();
    } else {
        logStatus("Write Error: " + reply->errorString(), true);
    }
    reply->deleteLater();
}

void ModbusClientWidget::on_chkAutoRead_toggled(bool checked)
{
    if (checked) m_scanTimer->start(ui->spinScanRate->value());
    else m_scanTimer->stop();
}

void ModbusClientWidget::logStatus(const QString &msg, bool isError)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    ui->lblStatus->setText(timestamp + " - " + msg);
    if (isError) ui->lblStatus->setStyleSheet("color: red;");
    else ui->lblStatus->setStyleSheet("color: green;");
}

void ModbusClientWidget::updateCounters()
{
    ui->lblTransactions->setText(QString("TX: %1 | RX: %2").arg(m_txCount).arg(m_rxCount));
}
