#ifndef DTOTCPSERVER_SINGLECLIENT_H
#define DTOTCPSERVER_SINGLECLIENT_H

#include "Debugger.h"
#include "AbstractCommunicationHandlerClass.h"

#include <QTcpServer>
#include <QTcpSocket>

/**
 * @brief TCP Server (Single Client) Communication Handler
 * 
 * A simplified TCP Server that accepts only one client connection at a time.
 * If a new client connects, the behavior depends on implementation (usually rejects or replaces).
 */
class TcpServer_SingleClient : public AbstractCommunicationHandler
{
    Q_OBJECT

    int port;
    QTcpServer *server;
    QTcpSocket *socket;
public:
    explicit TcpServer_SingleClient(QObject *parent = nullptr);
    explicit TcpServer_SingleClient(int p, QObject *parent = nullptr);
    ~TcpServer_SingleClient();

    /**
     * @brief Starts the TCP server on the specified port.
     * @param p Port to listen on.
     * @return true if listening started.
     */
    bool initialize(int p);

    /**
     * @brief Stops listening and closes any active client connection.
     */
    void close() override;

private slots:
    void acceptClient();
    void receive();
public slots:
    void send(QByteArray) override;
};

#endif // DTOTCPSERVER_SINGLECLIENT_H
