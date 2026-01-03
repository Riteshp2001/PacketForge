#include "TcpServer_SingleClientClass.h"

#include <QTimer>

TcpServer_SingleClient::TcpServer_SingleClient(QObject *parent):AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Server;
    server = new QTcpServer;
}

TcpServer_SingleClient::TcpServer_SingleClient(int p, QObject *parent):AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Server;
    server = new QTcpServer;
    initialize(p);
}

TcpServer_SingleClient::~TcpServer_SingleClient()
{
    close();
}

bool TcpServer_SingleClient::initialize(int p)
{
    this->port = p;
    if(!server->listen(QHostAddress::Any,port)){
        static int tryCount = 1;
        if(tryCount<=5){
            COMM_HANDLER_DEBUG && DEBUG("Unable to Start Server on Port : "<<port<<"Retrying .......")
            tryCount+=1;
            QTimer::singleShot(1000,this,[&](){
                initialize(port);
            });
            return false;
        }else{
            tryCount = 0;
            COMM_HANDLER_DEBUG && DEBUG("Could Not Start Server on Port : "<<port)
            return false;
        }
    }else{
        COMM_HANDLER_DEBUG && DEBUG("Listing on Port :"<<port)
        connect(server,SIGNAL(newConnection()),this,SLOT(acceptClient()));
        return true;
    }
}

void TcpServer_SingleClient::close()
{
    connection = false;
    emit disconnected();
    socket->close();
    socket->disconnect();
    delete socket;
    server->close();
    server->disconnect();
    delete server;
}

void TcpServer_SingleClient::acceptClient()
{
    socket = server->nextPendingConnection();
    emit connected();
    connection = true;
    server->pauseAccepting();
//    connect(socket,SIGNAL(connected()),this,SIGNAL(connected()));
    QObject::connect(socket,&QTcpSocket::disconnected,[&](){
        connection = false;
        emit disconnected();
        socket->deleteLater();
        server->resumeAccepting();
    });
    connect(socket,SIGNAL(bytesWritten(qint64)),this,SIGNAL(bytesWritten(qint64)));
    connect(socket,SIGNAL(readyRead()),this,SLOT(receive()));
}

void TcpServer_SingleClient::receive()
{
    QByteArray d = socket->readAll();
    if(dataReceivingRule!=nullptr){
        for(int i=0; i<d.length(); i++){
            if(dataReceivingRule(buffer,d[i])){
                emit receivedData(buffer);
                if(receivingQueue!=nullptr){
                    receivingQueue->enqueue(buffer);
                }
                RX_TX_DEBUG && DEBUG("Data Received : " << buffer)
                buffer.clear();
            }
        }
    }else{
        emit receivedData(d);
        if(receivingQueue!=nullptr){
            receivingQueue->enqueue(d);
        }
        RX_TX_DEBUG && DEBUG("Received Data : "<<d)
    }
}

void TcpServer_SingleClient::send(QByteArray d)
{
    if(dataSendingRule != nullptr) dataSendingRule(d);
    if(isConnected()){
        socket->write(d);
        RX_TX_DEBUG && DEBUG("Data Sent : " << d)
    }
}
