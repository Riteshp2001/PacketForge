/**
 * @file TcpClientClass.cpp
 * @brief TCP Client communication handler implementation.
 * 
 * @project PacketForge
 * @license MIT License
 */

#include "TcpClientClass.h"

/**
 * @brief Constructs a TcpClient object.
 * @param parent Parent object
 */
TcpClient::TcpClient(QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Client;
    socket = new QTcpSocket(this);
    connectSigSlot();
}

/**
 * @brief Constructs a TcpClient and initializes connection.
 * @param addr Host address
 * @param p Port number
 * @param parent Parent object
 */
TcpClient::TcpClient(QString addr, int p, QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Client;
    socket = new QTcpSocket(this);
    connectSigSlot();
    initialize(addr, p);
}

/**
 * @brief Destroys the TcpClient object and closes connection.
 */
TcpClient::~TcpClient()
{
    connection = false;
    if (socket) {
        socket->disconnect();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->abort();
        }
    }
}

/**
 * @brief Connects internal signals and slots for socket events.
 */
void TcpClient::connectSigSlot()
{
    connect(socket, &QTcpSocket::connected, this, [this]() {
        connection = true;
        emit connected();
    });
    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        connection = false;
        emit disconnected();
    });
    connect(socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError socketError) {
        emit error(static_cast<int>(socketError));
    });
    connect(socket, &QTcpSocket::bytesWritten, this, &TcpClient::bytesWritten);
    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::receive);
}

/**
 * @brief Initiates an asynchronous connection to the specified host.
 * @param addr Hostname or IP address
 * @param p Port number
 * @return true (connection is async, success determined by signals)
 */
bool TcpClient::initialize(QString addr, int p)
{
    address = addr;
    port = p;
    socket->connectToHost(address, port);
    return true;
}

/**
 * @brief Closes the TCP connection.
 */
void TcpClient::close()
{
    bool wasConnected = connection;
    connection = false;
    
    if (socket) {
        socket->disconnect();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->abort();
        }
        connectSigSlot();
    }
    address.clear();
    
    if (wasConnected) {
        emit disconnected();
    }
}

/**
 * @brief Receives incoming data from the socket.
 */
void TcpClient::receive()
{
    QByteArray d = socket->readAll();
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

/**
 * @brief Sends data over the TCP socket.
 * @param d Data to send
 */
void TcpClient::send(QByteArray d)
{
    if (dataSendingRule != nullptr) dataSendingRule(d);
    
    if (socket && socket->isOpen() && isConnected()) {
        socket->write(d);
    }
}
