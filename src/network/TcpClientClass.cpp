#include "TcpClientClass.h"
#include <thread>

TcpClient::TcpClient(QObject *parent):AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Client;
    socket = new QTcpSocket(this);
    connectSigSlot();
}

TcpClient::TcpClient(QString addr, int p, QObject *parent):AbstractCommunicationHandler(parent)
{
    commHandlerType = AbstractCommunicationHandler::Type::TCP_Client;
    socket = new QTcpSocket(this);
    connectSigSlot();
    initialize(addr,p);
}

TcpClient::~TcpClient()
{
    close();
}

void TcpClient::connectSigSlot()
{
    QObject::connect(socket,&QTcpSocket::connected,[&](){
        connection = true;
        emit connected();
    });
    QObject::connect(socket,&QTcpSocket::disconnected,[&](){
        connection = false;
        emit disconnected();
        initialize(address.toString(),port);
    });
    connect(socket,SIGNAL(bytesWritten(qint64)),this,SIGNAL(bytesWritten(qint64)));
    connect(socket,SIGNAL(readyRead()),this,SLOT(receive()));
}

bool TcpClient::initialize(QString addr, int p)
{
    address.setAddress(addr);
    port = p;
    socket->connectToHost(address,port);
    if(!socket->waitForConnected(2000)){
        COMM_HANDLER_DEBUG && DEBUG("Error : "<<socket->errorString())
        initialize(address.toString(),port);
        emit error(0);
        return false;
    }
    return true;
}

void TcpClient::close()
{
    connection = false;
    emit disconnected();
    address.clear();
    socket->close();
    socket->disconnect();
    delete socket;
}

void TcpClient::receive()
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

void TcpClient::send(QByteArray d)
{
    if(dataSendingRule != nullptr) dataSendingRule(d);
    if(isConnected()){
        socket->write(d);
        RX_TX_DEBUG && DEBUG("Data Sent : " << d)
    }
}
