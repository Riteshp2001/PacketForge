/**
 * @file UdpClass.cpp
 * @brief UDP communication handler implementation.
 * 
 * @project PacketForge
 * @license MIT License
 */

#include "UdpClass.h"
#include <QTimer>

Udp::Udp(QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::UDP;
}

Udp::Udp(QString a, int p, QObject *parent) : AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::UDP;
    addr = QHostAddress(a);
    initialize(a, p);
}

Udp::~Udp()
{
    close();
}

/**
 * @brief Initializes the UDP socket and binds to the specified port.
 * @param a IP address for sending
 * @param p Port number to bind
 * @return true if bind successful, false otherwise
 */
bool Udp::initialize(QString a, int p)
{
    addr.setAddress(a);
    port = p; 

    if (!socket.bind(QHostAddress::Any, static_cast<quint16>(port))) {
        return false;
    }
    
    connect(&socket, SIGNAL(readyRead()), this, SLOT(receive()));
    emit connected(); 
    connection = true;
    return true;
}

/**
 * @brief Closes the UDP socket.
 */
void Udp::close()
{
    connection = false;
    emit disconnected();
    socket.close();
    socket.disconnect();
    addr.clear();
}

/**
 * @brief Receives incoming UDP datagrams.
 */
void Udp::receive()
{
    while (socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(socket.pendingDatagramSize()));
        socket.readDatagram(datagram.data(), datagram.size());

        if (dataReceivingRule != nullptr) {
            for (int i = 0; i < datagram.length(); i++) {
                if (dataReceivingRule(buffer, datagram[i])) {
                    emit receivedData(buffer);
                    if (receivingQueue != nullptr) {
                        receivingQueue->enqueue(buffer);
                    }
                    buffer.clear();
                }
            }
        } else {
            emit receivedData(datagram);
            if (receivingQueue != nullptr) {
                receivingQueue->enqueue(buffer);
            }
        }
    }
}

/**
 * @brief Sends data via UDP datagram.
 * @param d Data to send
 */
void Udp::send(QByteArray d)
{
    if (dataSendingRule != nullptr) dataSendingRule(d);
    emit bytesWritten(socket.writeDatagram(d, addr, static_cast<quint16>(port)));
}
