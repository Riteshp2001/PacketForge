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
    initialize(a,p);
}

Udp::~Udp()
{
    close();
}

bool Udp::initialize(QString a, int p){

    bool status = true;
    addr.setAddress(a);
    port=p; //Initializing port number

    //checking socket is binding to the specific add and port
    if(!socket.bind(QHostAddress::LocalHost,static_cast<quint16>(port)))
    {
        static int tryCount = 1;
        //trying for 5 times to make socket to bind incase of bind fail
        if(tryCount<=5)
        {
            tryCount+=1;
            QTimer::singleShot(1000,this,[&](){
                initialize(addr.toString(),port);
            });
            status= false;
        }
        else
        {
            tryCount = 0;
            status= false;
        }
    }
    else
    {
        connect(&socket,SIGNAL(readyRead()),this,SLOT(receive()));
        connect(&socket,&QUdpSocket::disconnected,[&](){initialize(addr.toString(),port);});
    }
    return status;
}

void Udp::close()
{
    connection = false;
    emit disconnected();
    socket.close();
    socket.disconnect();
    addr.clear();
}

void Udp::receive()
{
    while (socket.hasPendingDatagrams())// Checking for datagrams available or not
    {
        QByteArray datagram;
        datagram.resize(static_cast<int>(socket.pendingDatagramSize()));//resizing the datagram as per pending Datagram Size
        socket.readDatagram(datagram.data(), datagram.size());

        if(dataReceivingRule!=nullptr){
            for(int i=0; i<datagram.length(); i++){
                if(dataReceivingRule(buffer,datagram[i])){
                    emit receivedData(buffer);
                    if(receivingQueue!=nullptr){
                        receivingQueue->enqueue(buffer);
                    }
                    RX_TX_DEBUG && DEBUG("Data Received : " << buffer.data())
                            buffer.clear();
                }
            }
        }else{
            emit receivedData(datagram);
            if(receivingQueue!=nullptr){
                receivingQueue->enqueue(buffer);
            }
            RX_TX_DEBUG && DEBUG("Data Received : " << datagram.data())
        }
    }
}

void Udp::send(QByteArray d)
{
    if(dataSendingRule != nullptr) dataSendingRule(d);
    emit bytesWritten(socket.writeDatagram(d,addr,static_cast<quint16>(port)));
    RX_TX_DEBUG && DEBUG("Data Sent : " << d.data())
}
