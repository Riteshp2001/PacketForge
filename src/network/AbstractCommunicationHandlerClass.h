#ifndef COMMUNICATIONHANDLER_H
#define COMMUNICATIONHANDLER_H

#include <QObject>
#include <QQueue>

// Communication Handler Type Definitions
#define SERIAL_WIN32    "SERIAL_WIN32"
#define SERIAL_QT       "SERIAL_QT"
#define TCP_SERVER      "TCP_SERVER"
#define TCP_CLIENT      "TCP_CLIENT"
#define UPD             "UDP"

struct DeviceCommParams;
struct DeviceInterfaceDetail;

/**
 * @brief Data Receiving Rule Callback
 * Function pointer type for determining if a received byte completes a packet.
 */
typedef bool(*DRR)(QByteArray&, const char);

/**
 * @brief Data Sending Rule Callback
 * Function pointer type for formatting data before sending.
 */
typedef bool(*DSR)(QByteArray&);

/**
 * @brief The AbstractCommunicationHandler class
 * 
 * Abstract base class for all communication handlers (Serial, TCP, UDP).
 * Defines the common interface for connecting, sending, and receiving data.
 */
class AbstractCommunicationHandler : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Supported Communication Types
     */
    enum Type {
        Serial_QT = 1,
        Serial_Win32 = 2,
        TCP_Server = 3,
        TCP_Client = 4,
        UDP = 5,
        InvalidCommHandlerType = 6
    };

    explicit AbstractCommunicationHandler(QObject *parent = nullptr);
    // virtual ~AbstractCommunicationHandler();

    /**
     * @brief Sets the queue where received data packets will be stored.
     * @param receivingQ Pointer to the QQueue<QByteArray>.
     */
    void setReceivingQueue(QQueue<QByteArray>* receivingQ);

    /**
     * @brief Sets the rule for parsing incoming data streams.
     * @param rule Function pointer to the parsing logic.
     */
    void setDataReceivingRule(DRR rule);

    /**
     * @brief Sets the rule for formatting outgoing data.
     * @param rule Function pointer to the formatting logic.
     */
    void setDataSendingRule(DSR rule);

    /**
     * @brief Checks if the handler is currently connected.
     * @return true if connected, false otherwise.
     */
    bool isConnected(void);

    // --- Static Helpers for Type Conversion ---
    static QString getCommHandlerType(int chType);
    static QString getCommHandlerType(Type chType);
    static Type getCommHandlerType(QString chType);

    /**
     * @brief Factory method to create a communication handler instance.
     * @param commparam Parameters defining the type and settings of the connection.
     * @return Pointer to the created handler, or nullptr on failure.
     */
    static AbstractCommunicationHandler *MakeCommunicationHandler(const DeviceCommParams &commparam);

    /**
     * @brief Closes the connection. Must be implemented by subclasses.
     */
    virtual void close() = 0;

protected:
    QByteArray buffer;                  ///< Internal buffer for incoming data
    bool connection;                    ///< Connection state status
    QQueue<QByteArray>* receivingQueue; ///< Pointer to external receive queue
    DRR dataReceivingRule;              ///< Callback for data parsing
    DSR dataSendingRule;                ///< Callback for data formatting
    Type commHandlerType;               ///< Type of this handler instance

public slots:
    /**
     * @brief Sends data via the communication channel.
     * @param data The byte array to send.
     */
    virtual void send(QByteArray data) = 0;

    // Hardware Pin Control (Virtual - Default No-Op)
    virtual void setDtr(bool set) {}
    virtual void setRts(bool set) {}
    
    /**
     * @brief Gets the status of input pins (CTS, DSR, DCD, RI).
     * @return Bitmask: 0x1=CTS, 0x2=DSR, 0x4=DCD, 0x8=RI
     */
    virtual int getPinStatus() { return 0; }

signals:
    void receivedData(QByteArray data); ///< Emitted when new data is received
    void connected(void);               ///< Emitted on successful connection
    void disconnected(void);            ///< Emitted on disconnection
    void bytesWritten(qint64 bytes);    ///< Emitted when bytes are written to the interface
    void error(int code);               ///< Emitted when an error occurs
};

/**
 * @brief Configuration parameters for creating a communication handler.
 */
struct DeviceCommParams{
    AbstractCommunicationHandler::Type commHandlertype;
    QString port;       ///< Serial port name or Identifier
    int baudrate;       ///< Baud rate for serial
    int parity;         ///< Parity setting
    int dataBits;       ///< Data bits
    int stopBits;       ///< Stop bits
    int flowControl;    ///< Flow control setting
    QString address;    ///< IP Address for Network modes
    QString commHandlerModelLink;

    DeviceCommParams();
};

/**
 * @brief High-level device interface definition.
 * Used to group connection parameters with a name and ID.
 */
struct DeviceInterfaceDetail
{
    int id;
    QString name;
    DeviceCommParams CommParams;

    DeviceInterfaceDetail();
};

#endif // COMMUNICATIONHANDLER_H
