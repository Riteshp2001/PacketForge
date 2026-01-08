/**
 * @file TcpServer_SingleClientClass.cpp
 * @brief TCP Server (single client) communication handler implementation.
 * 
 * @project PacketForge
 * @license MIT License
 */

#include "TcpServer_SingleClientClass.h"
#include <QTimer>

/**
 * @brief Constructs a TcpServer_SingleClient object.
 * @param parent Parent object
 */
TcpServer_SingleClient::TcpServer_SingleClient(QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Server;
    server = new QTcpServer;
}

/**
 * @brief Constructs a TcpServer_SingleClient and starts listening.
 * @param p Port number
 * @param parent Parent object
 */
TcpServer_SingleClient::TcpServer_SingleClient(int p, QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Server;
    server = new QTcpServer;
    initialize(p);
}

/**
 * @brief Destroys the server and closes connections.
 */
TcpServer_SingleClient::~TcpServer_SingleClient()
{
    connection = false;
    if (socket) {
        if (socket->isOpen()) socket->close();
        socket->disconnect();
    }
    if (server) {
        server->close();
        delete server;
        server = nullptr;
    }
}

/**
 * @brief Starts listening on the specified port.
 * @param p Port number to listen on
 * @return true if server started successfully, false otherwise
 */
bool TcpServer_SingleClient::initialize(int p)
{
    this->port = p;
    if (!server) server = new QTcpServer(this);

    if (!server->listen(QHostAddress::Any, port)) {
        return false;
    }
    
    connect(server, SIGNAL(newConnection()), this, SLOT(acceptClient()));
    emit connected();
    return true;
}

/**
 * @brief Closes the server and disconnects any connected client.
 */
void TcpServer_SingleClient::close()
{
    connection = false;
    emit disconnected();
    
    if (socket) {
        if (socket->isOpen()) socket->close();
        socket->disconnect(); 
        socket->deleteLater(); 
        socket = nullptr;
    }
    
    if (server) {
        server->close();
    }
}

/**
 * @brief Accepts a new incoming client connection.
 */
void TcpServer_SingleClient::acceptClient()
{
    socket = server->nextPendingConnection();
    connection = true;
    server->pauseAccepting();
    
    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        connection = false;
        if (socket) {
            socket->deleteLater();
            socket = nullptr;
        }
        if (server) {
            server->resumeAccepting();
        }
    });
    
    connect(socket, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()), this, SLOT(receive()));
}

/**
 * @brief Receives data from the connected client.
 */
void TcpServer_SingleClient::receive()
{
    if (!socket) return;
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
 * @brief Sends data to the connected client.
 * @param d Data to send
 */
void TcpServer_SingleClient::send(QByteArray d)
{
    if (dataSendingRule != nullptr) dataSendingRule(d);
    
    if (socket && socket->isOpen()) {
        socket->write(d);
    }
}
