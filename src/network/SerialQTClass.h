#ifndef DTOSERIAL_H
#define DTOSERIAL_H

#include "AbstractCommunicationHandlerClass.h"
#include "Debugger.h"

#include <QSerialPort>
#include <QThread>
#include <QMutex>
#include <QTimer>

/**
 * @brief background worker for Serial operations.
 * 
 * Handles the actual QSerialPort interactions in a separate thread
 * to avoid blocking the GUI.
 */
class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr) : QObject(parent), p(nullptr) {}
    ~SerialWorker() {
        if(p) {
            if(p->isOpen()) p->close();
            delete p;
        }
    }

public slots:
    void setDtr(bool set) { if(p) p->setDataTerminalReady(set); }
    void setRts(bool set) { if(p) p->setRequestToSend(set); }

    /**
     * @brief Initializes and opens the serial port.
     */
    void initialize(QString portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl) {
        if(!p) p = new QSerialPort(this);
        if(p->isOpen()) p->close();

        p->setPortName(portName);
        p->setBaudRate(baudRate);
        p->setDataBits((QSerialPort::DataBits)dataBits);
        p->setParity((QSerialPort::Parity)parity);
        p->setStopBits((QSerialPort::StopBits)stopBits);
        p->setFlowControl((QSerialPort::FlowControl)flowControl);

        if(p->open(QIODevice::ReadWrite)) {
            connect(p, &QSerialPort::readyRead, this, &SerialWorker::onReadyRead);
            
            // Start Pin Monitor
            if(!monitorTimer) {
                monitorTimer = new QTimer(this);
                connect(monitorTimer, &QTimer::timeout, this, &SerialWorker::monitorPins);
            }
            monitorTimer->start(100);
            
            emit connected();
        } else {
            emit error(0); // 0 = generic open error
        }
    }

    /**
     * @brief Closes the serial port.
     */
    void closePort() {
        if(monitorTimer) monitorTimer->stop();
        if(p && p->isOpen()) {
            p->close();
            emit disconnected();
        }
    }

    /**
     * @brief Writes data to the serial port.
     */
    void sendData(QByteArray d) {
        if(p && p->isOpen()) {
            qint64 bytes = p->write(d);
            if(bytes > 0) emit bytesWritten(bytes);
        }
    }

    void updateSettings(QString portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl) {
        // Support dynamic re-config if needed, for now just re-init
        initialize(portName, baudRate, dataBits, parity, stopBits, flowControl);
    }

private slots:
    void onReadyRead() {
        if(p) emit dataReceived(p->readAll());
    }
    
    void monitorPins() {
        if(p && p->isOpen()) {
            emit pinStatusChanged((int)p->pinoutSignals());
        }
    }

signals:
    void connected();
    void disconnected();
    void error(int);
    void dataReceived(QByteArray);
    void bytesWritten(qint64);
    void pinStatusChanged(int);

private:
    QSerialPort *p;
    QTimer *monitorTimer = nullptr;
};


/**
 * @brief Serial Port Communication Handler (Qt Version)
 * 
 * Implements serial communication using QSerialPort moved to a worker thread.
 * This ensures non-blocking I/O operations.
 */
class SerialQT : public AbstractCommunicationHandler
{
    Q_OBJECT

    QThread *workerThread;
    SerialWorker *worker;

public:
    explicit SerialQT(QObject *parent = nullptr);
    explicit SerialQT(QString, int baudRate = 9600, int dataBits = 8, int parity = 0, int stopBits = 1, int flowControl = 0, QObject *parent = nullptr);
    ~SerialQT();
    
    /**
     * @brief Starts the worker thread and initializes the port.
     * @return true (always returns true as init is async).
     */
    bool initialize(QString, int baudRate = 9600, int dataBits = 8, int parity = 0, int stopBits = 1, int flowControl = 0 );
    
    /**
     * @brief Stops the thread and closes the port.
     */
    void close() override;

public slots:
    /**
     * @brief Sends data to the serial port asynchronously.
     * @param data Data to send.
     */
    void send(QByteArray data) override;
    
    void setDtr(bool set) override { emit operateSetDtr(set); }
    void setRts(bool set) override { emit operateSetRts(set); }
    int getPinStatus() override { return m_cachedPinStatus; }

signals:
    // Internal signals to communicate with worker
    void operateInit(QString, int, int, int, int, int);
    void operateSend(QByteArray);
    void operateClose();
    void operateSetDtr(bool);
    void operateSetRts(bool);

private slots:
    // Slots to handle signals FROM worker (run in Main Thread)
    void onWorkerConnected();
    void onWorkerDisconnected();
    void onWorkerError(int);
    void onWorkerDataReceived(QByteArray);
    void onWorkerBytesWritten(qint64);
    void onWorkerPinStatusChanged(int status) { m_cachedPinStatus = status; }
    
private:
    int m_cachedPinStatus = 0;
};

#endif // DTOSERIAL_H
