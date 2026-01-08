/**
 * @file ModbusClientWidget.h
 * @brief Modbus RTU/TCP client widget with read/write capabilities.
 * 
 * Provides a professional interface for Modbus communication. Supports:
 * - Modbus TCP and RTU modes
 * - Read Coils, Discrete Inputs, Holding Registers, Input Registers
 * - Write Single/Multiple Coils and Registers
 * - Configurable Slave ID, Baud Rate, Parity, Stop Bits
 * - Transaction logging and formatted data display (Dec, Hex, Bin, ASCII)
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef MODBUSCLIENTWIDGET_H
#define MODBUSCLIENTWIDGET_H

#include <QWidget>
#include <QModbusClient>
#include <QModbusTcpClient>
#include <QModbusRtuSerialClient>
#include <QTimer>
#include <QElapsedTimer>

namespace Ui {
class ModbusClientWidget;
}

/**
 * @brief Main widget for Modbus client functionality.
 */
class ModbusClientWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the ModbusClientWidget.
     * @param parent Parent widget
     */
    explicit ModbusClientWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor.
     */
    ~ModbusClientWidget();

private slots:
    // UI Interactions
    void on_btnConnect_clicked();
    void on_btnRefreshPorts_clicked();
    void on_btnRead_clicked();
    void on_btnWrite_clicked();
    void on_chkAutoRead_toggled(bool checked);
    void on_comboMode_currentIndexChanged(int index);
    
    // Modbus Events
    void onStateChanged(QModbusDevice::State state);
    void onErrorOccurred(QModbusDevice::Error error);
    void onReadReady();
    void onWriteReady();

private:
    /**
     * @brief Refreshes the list of available serial ports.
     */
    void refreshSerialPorts();
    
    /**
     * @brief Adds a log entry to the transaction counter/status.
     * @param msg Message to log
     * @param isError true if the message is an error
     */
    void logStatus(const QString &msg, bool isError = false);
    
    /**
     * @brief Updates transaction counters in the UI.
     */
    void updateCounters();

    Ui::ModbusClientWidget *ui;
    QModbusClient *m_modbusDevice;
    QTimer *m_scanTimer;
    
    int m_txCount = 0;
    int m_rxCount = 0;
};

#endif // MODBUSCLIENTWIDGET_H
