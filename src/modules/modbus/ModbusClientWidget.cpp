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
    
    refreshSerialPorts();
    
    // Populate Bauds
    ui->comboBaud->addItems({"9600", "19200", "38400", "57600", "115200"});
    ui->comboBaud->setCurrentText("115200");

    ui->spinCount->setValue(10);
    ui->tableRegisters->horizontalHeader()->setStretchLastSection(true);
    
    connect(m_scanTimer, &QTimer::timeout, this, &ModbusClientWidget::on_btnRead_clicked);
}

ModbusClientWidget::~ModbusClientWidget()
{
    if (m_modbusDevice)
        m_modbusDevice->disconnectDevice();
    delete m_modbusDevice;
    delete ui;
}

void ModbusClientWidget::refreshSerialPorts()
{
    ui->comboPort->clear();
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        ui->comboPort->addItem(info.portName());
    }
}

void ModbusClientWidget::on_comboMode_currentIndexChanged(int index)
{
    ui->stackSettings->setCurrentIndex(index);
}

void ModbusClientWidget::on_btnConnect_clicked()
{
    if (m_modbusDevice && m_modbusDevice->state() == QModbusDevice::ConnectedState) {
        m_modbusDevice->disconnectDevice();
        ui->btnConnect->setText("Connect");
        ui->btnConnect->setChecked(false);
        return;
    }

    if (m_modbusDevice) {
        delete m_modbusDevice;
        m_modbusDevice = nullptr;
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
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, ui->comboBaud->currentText().toInt());
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        m_modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);
    }

    connect(m_modbusDevice, &QModbusClient::stateChanged, this, &ModbusClientWidget::onStateChanged);
    connect(m_modbusDevice, &QModbusClient::errorOccurred, this, &ModbusClientWidget::onErrorOccurred);

    if (!m_modbusDevice->connectDevice()) {
        ui->lblStatus->setText("Connection error: " + m_modbusDevice->errorString());
    }
}

void ModbusClientWidget::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState) {
        ui->lblStatus->setText("State: Connected");
        ui->btnConnect->setText("Disconnect");
        ui->btnConnect->setChecked(true);
    } else if (state == QModbusDevice::UnconnectedState) {
        ui->lblStatus->setText("State: Disconnected");
        ui->btnConnect->setText("Connect");
        ui->btnConnect->setChecked(false);
        m_scanTimer->stop();
        ui->chkAutoRead->setChecked(false);
    }
}

void ModbusClientWidget::onErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError) return;
    ui->lblStatus->setText("Error: " + m_modbusDevice->errorString());
}

void ModbusClientWidget::on_btnRead_clicked()
{
    if (!m_modbusDevice || m_modbusDevice->state() != QModbusDevice::ConnectedState) return;

    QModbusDataUnit::RegisterType type;
    int index = ui->comboRegType->currentIndex();
    if (index == 0) type = QModbusDataUnit::Coils;
    else if (index == 1) type = QModbusDataUnit::DiscreteInputs;
    else if (index == 2) type = QModbusDataUnit::HoldingRegisters;
    else type = QModbusDataUnit::InputRegisters;

    int startAddr = ui->spinStartAddr->value();
    int count = ui->spinCount->value();

    if (auto *reply = m_modbusDevice->sendReadRequest(QModbusDataUnit(type, startAddr, count), 1)) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusClientWidget::onReadReady);
        else
            delete reply; // Finished immediately (broadcast?)
    } else {
        ui->lblStatus->setText("Read error: " + m_modbusDevice->errorString());
    }
}

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
            ui->tableRegisters->setItem(i, 2, new QTableWidgetItem("0x" + QString::number(val, 16).toUpper()));
        }
        ui->lblStatus->setText("Read OK: " + QTime::currentTime().toString());
    } else {
         ui->lblStatus->setText("Read Error: " + reply->errorString());
    }

    reply->deleteLater();
}

void ModbusClientWidget::on_chkAutoRead_toggled(bool checked)
{
    if (checked) m_scanTimer->start(1000); // 1s
    else m_scanTimer->stop();
}
