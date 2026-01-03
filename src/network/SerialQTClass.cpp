#include "SerialQTClass.h"

SerialQT::SerialQT(QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::Serial_QT;
    workerThread = new QThread(this);
    worker = new SerialWorker();
    worker->moveToThread(workerThread);
    
    // Wiring signals
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
    : SerialQT(parent) // Delegate to default constructor to setup thread
{
    initialize(portName, baudRate, dataBits, parity, stopBits, flowControl);
}

SerialQT::~SerialQT(){
    close();
    workerThread->quit();
    workerThread->wait();
    delete worker;
}

bool SerialQT::initialize(QString portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl)
{
    emit operateInit(portName, baudRate, dataBits, parity, stopBits, flowControl);
    return true;
}

void SerialQT::close()
{
    emit operateClose();
}

void SerialQT::send(QByteArray d){
    if(isConnected()){
        if(dataSendingRule != nullptr) dataSendingRule(d);
        emit operateSend(d);
    }
}

// Slots handling Worker signals

void SerialQT::onWorkerConnected()
{
    connection = true;
    emit connected();
    RX_TX_DEBUG && DEBUG("Port Connected via Worker")
}

void SerialQT::onWorkerDisconnected()
{
    connection = false;
    emit disconnected();
    RX_TX_DEBUG && DEBUG("Port Disconnected via Worker")
}

void SerialQT::onWorkerError(int err)
{
    emit error(err);
    RX_TX_DEBUG && DEBUG("Worker Port Error: " << err)
}

void SerialQT::onWorkerBytesWritten(qint64 bytes)
{
    emit bytesWritten(bytes);
}

void SerialQT::onWorkerDataReceived(QByteArray d)
{
    if(dataReceivingRule != nullptr){
        for(int i=0; i<d.length(); i++){
            if(dataReceivingRule(buffer, d[i])){
                emit receivedData(buffer);
                if(receivingQueue!=nullptr){
                    receivingQueue->enqueue(buffer);
                }
                buffer.clear();
            }
        }
    } else {
        // No rule, just pass through
        emit receivedData(d);
        if(receivingQueue!=nullptr){
            receivingQueue->enqueue(d);
        }
    }
    
    // Debug logging
    // RX_TX_DEBUG && DEBUG("Received Data : "<<d)
}
