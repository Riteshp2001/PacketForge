/**
 * @file TcpClientClass.h
 * @brief TCP client communication handler.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef DTOTCPCLIENT_H
#define DTOTCPCLIENT_H

#include "AbstractCommunicationHandlerClass.h"
#include "Debugger.h"

#include <QTcpSocket>
#include <QHostAddress>

/**
 * @brief TCP Client Communication Handler
 * 
 * Implements a standard TCP client that connects to a remote server.
 * Uses non-blocking QTcpSocket signals.
 */
class TcpClient : public AbstractCommunicationHandler
{
    Q_OBJECT

    QString address;
    int port;
    QTcpSocket *socket;
    void connectSigSlot(void);
public:
    explicit TcpClient(QObject *parent = nullptr);
    explicit TcpClient(QString addr, int p, QObject *parent = nullptr);
    ~TcpClient();

    /**
     * @brief Connects to the specified TCP server.
     * @param addr IP Address (e.g., "192.168.1.1").
     * @param p Port number.
     * @return true.
     */
    bool initialize(QString addr, int p);
    
    /**
     * @brief Disconnects from the server.
     */
    void close() override;

private slots:
    void receive();
public slots:
    void send(QByteArray) override;
};

#endif // DTOTCPCLIENT_H
