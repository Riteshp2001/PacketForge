/**
 * @file SerialQTClass.cpp
 * @brief Serial port communication handler implementation using a worker thread.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "SerialQTClass.h"

SerialQT::SerialQT(QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::Serial_QT;
    workerThread = new QThread(this);
    worker = new SerialWorker();
    worker->moveToThread(workerThread);
    
    connect(this, &SerialQT::operateInit, worker, &SerialWorker::initialize);
    connect(this, &SerialQT::operateSend, worker, &SerialWorker::sendData);
    connect(this, &SerialQT::operateClose, worker, &SerialWorker::closePort);
    connect(this, &SerialQT::operateSetDtr, worker, &SerialWorker::setDtr);
    connect(this, &SerialQT::operateSetRts, worker, &SerialWorker::setRts);

    connect(worker, &SerialWorker::connected, this, &SerialQT::onWorkerConnected);
    connect(worker, &SerialWorker::disconnected, this, &SerialQT::onWorkerDisconnected);
    connect(worker, &SerialWorker::error, this, &SerialQT::onWorkerError);
    connect(worker, &SerialWorker::dataReceived, this, &SerialQT::onWorkerDataReceived);
    connect(worker, &SerialWorker::bytesWritten, this, &SerialQT::onWorkerBytesWritten);
    connect(worker, &SerialWorker::pinStatusChanged, this, &SerialQT::onWorkerPinStatusChanged);

    workerThread->start();
}

SerialQT::SerialQT(QString portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl, QObject *parent) 
    : SerialQT(parent)
{
    initialize(portName, baudRate, dataBits, parity, stopBits, flowControl);
}

SerialQT::~SerialQT()
{
    emit operateClose();
    
    if (worker) {
        QMetaObject::invokeMethod(worker, "deleteLater", Qt::QueuedConnection);
    }

    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
        delete workerThread;
    }
}

/**
 * @brief Initializes the serial port with the specified parameters.
 * @param portName Serial port name (e.g., "COM1" or "/dev/ttyUSB0")
 * @param baudRate Baud rate
 * @param dataBits Data bits (5-8)
 * @param parity Parity mode
 * @param stopBits Stop bits
 * @param flowControl Flow control mode
 * @return true (initialization is async via worker thread)
 */
bool SerialQT::initialize(QString portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl)
{
    emit operateInit(portName, baudRate, dataBits, parity, stopBits, flowControl);
    return true;
}

/**
 * @brief Closes the serial port.
 */
void SerialQT::close()
{
    emit operateClose();
}

/**
 * @brief Sends data through the serial port.
 * @param d Data to send
 */
void SerialQT::send(QByteArray d)
{
    if (isConnected()) {
        if (dataSendingRule != nullptr) dataSendingRule(d);
        emit operateSend(d);
    }
}

/**
 * @brief Handles successful connection signal from worker.
 */
void SerialQT::onWorkerConnected()
{
    connection = true;
    emit connected();
}

/**
 * @brief Handles disconnection signal from worker.
 */
void SerialQT::onWorkerDisconnected()
{
    connection = false;
    emit disconnected();
}

/**
 * @brief Handles error signal from worker.
 * @param err Error code
 */
void SerialQT::onWorkerError(int err)
{
    emit error(err);
}

/**
 * @brief Handles bytes written signal from worker.
 * @param bytes Number of bytes written
 */
void SerialQT::onWorkerBytesWritten(qint64 bytes)
{
    emit bytesWritten(bytes);
}

/**
 * @brief Processes received data from the worker thread.
 * @param d Received data
 */
void SerialQT::onWorkerDataReceived(QByteArray d)
{
    if (dataReceivingRule != nullptr) {
        for (int i = 0; i < d.length(); i++) {
            if (dataReceivingRule(buffer, d[i])) {
                emit receivedData(buffer);
                if (receivingQueue != nullptr) {
                    receivingQueue->enqueue(buffer);
                }
                buffer.clear();
            }
        }
    } else {
        emit receivedData(d);
        if (receivingQueue != nullptr) {
            receivingQueue->enqueue(d);
        }
    }
}
