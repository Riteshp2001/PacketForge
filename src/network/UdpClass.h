/**
 * @file UdpClass.h
 * @brief UDP communication handler.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef DTOUDP_H
#define DTOUDP_H

#include "AbstractCommunicationHandlerClass.h"
#include "Debugger.h"

#include <QUdpSocket>

/**
 * @brief UDP Communication Handler
 * 
 * Implements UDP "connectionless" communication. 
 * Even though UDP is connectionless, this class maintains a target IP/Port
 * to emulate a connected state for the application logic.
 */
class Udp : public AbstractCommunicationHandler
{
    Q_OBJECT

    QUdpSocket socket;
    QHostAddress addr;
    int port;
public:
    explicit Udp(QObject *parent = nullptr);
    explicit Udp(QString a,int p, QObject *parent = nullptr);
    ~Udp();

    /**
     * @brief Binds to the specified port and sets target address.
     * @param a Target IP address.
     * @param p Local/Target port.
     * @return true.
     */
    bool initialize(QString a,int p);

    /**
     * @brief Closes the UDP socket.
     */
    void close() override;

private slots:
    void receive();
public slots:
    void send(QByteArray) override;
};

#endif // DTOUDP_H
