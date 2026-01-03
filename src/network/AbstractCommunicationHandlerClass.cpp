#include "AbstractCommunicationHandlerClass.h"

#include "SerialQTClass.h"
#include "TcpServer_SingleClientClass.h"
#include "TcpClientClass.h"
#include "UdpClass.h"

AbstractCommunicationHandler::AbstractCommunicationHandler(QObject *parent)
    :QObject(parent),
    connection(false),
    receivingQueue(nullptr),
    dataReceivingRule(nullptr),
    dataSendingRule(nullptr)
{}

//AbstractCommunicationHandler::~AbstractCommunicationHandler(){}

void AbstractCommunicationHandler::setReceivingQueue(QQueue<QByteArray> *receivingQ){this->receivingQueue = receivingQ;}
void AbstractCommunicationHandler::setDataReceivingRule(DRR f){dataReceivingRule = f;}
void AbstractCommunicationHandler::setDataSendingRule(DSR f){dataSendingRule = f;}
bool AbstractCommunicationHandler::isConnected(){return connection;}

AbstractCommunicationHandler::Type AbstractCommunicationHandler::getCommHandlerType(QString chType)
{
    chType = chType.trimmed().toUpper();
    if(chType == SERIAL_QT) return AbstractCommunicationHandler::Type::Serial_QT;
    else if(chType == SERIAL_WIN32) return AbstractCommunicationHandler::Type::Serial_Win32;
    else if(chType == TCP_SERVER) return AbstractCommunicationHandler::Type::TCP_Server;
    else if(chType == TCP_CLIENT) return AbstractCommunicationHandler::Type::TCP_Client;
    else if(chType == UPD) return AbstractCommunicationHandler::Type::UDP;
    else return AbstractCommunicationHandler::Type::InvalidCommHandlerType;
}

AbstractCommunicationHandler *AbstractCommunicationHandler::MakeCommunicationHandler(const DeviceCommParams &commparam)
{
    AbstractCommunicationHandler *ptrCommHandler;
    switch (commparam.commHandlertype) {
    case AbstractCommunicationHandler::Serial_QT:
        ptrCommHandler = new SerialQT(
                    commparam.port,
                    commparam.baudrate!=-1?commparam.baudrate:9600,
                    commparam.dataBits!=-1?commparam.dataBits:8,
                    commparam.parity!=-1?commparam.parity:0,
                    commparam.stopBits!=-1?commparam.stopBits:1,
                    commparam.flowControl!=-1?commparam.flowControl:0);
        break;
    // case AbstractCommunicationHandler::Serial_Win32:
    //     ptrCommHandler = new SerialWin32(
    //                 commparam.port,
    //                 commparam.baudrate!=-1?commparam.baudrate:9600,
    //                 commparam.dataBits!=-1?commparam.dataBits:8,
    //                 commparam.parity!=-1?commparam.parity:0);
    //     break;
    case AbstractCommunicationHandler::TCP_Server:
        ptrCommHandler = new TcpServer_SingleClient(commparam.port.toInt());
        break;
    case AbstractCommunicationHandler::TCP_Client:
        ptrCommHandler = new TcpClient(commparam.address,commparam.port.toInt());
        break;
    case AbstractCommunicationHandler::UDP:
        ptrCommHandler = new Udp(commparam.address,commparam.port.toInt());
        break;
    case AbstractCommunicationHandler::InvalidCommHandlerType:
        ptrCommHandler = nullptr;
        break;
    }
    return ptrCommHandler;
}

QString AbstractCommunicationHandler::getCommHandlerType(int chType)
{
    switch (static_cast<Type>(chType)) {
    case AbstractCommunicationHandler::Serial_QT:return SERIAL_QT;
    case AbstractCommunicationHandler::Serial_Win32:return SERIAL_WIN32;
    case AbstractCommunicationHandler::TCP_Server:return TCP_SERVER;
    case AbstractCommunicationHandler::TCP_Client:return TCP_CLIENT;
    case AbstractCommunicationHandler::UDP:return UPD;
    default:return "";
    }
}

QString AbstractCommunicationHandler::getCommHandlerType(AbstractCommunicationHandler::Type chType)
{
    switch (chType) {
    case AbstractCommunicationHandler::Serial_QT:return SERIAL_QT;
    case AbstractCommunicationHandler::Serial_Win32:return SERIAL_WIN32;
    case AbstractCommunicationHandler::TCP_Server:return TCP_SERVER;
    case AbstractCommunicationHandler::TCP_Client:return TCP_CLIENT;
    case AbstractCommunicationHandler::UDP:return UPD;
    default:return "";
    }
}

DeviceCommParams::DeviceCommParams(){
    commHandlertype = AbstractCommunicationHandler::Type::InvalidCommHandlerType;
    port.clear();
    address.clear();
    baudrate = -1;
    parity = -1;
    dataBits = -1;
    stopBits = -1;
    flowControl = -1;
    commHandlerModelLink.clear();
}

DeviceInterfaceDetail::DeviceInterfaceDetail(){
    id=-1;
    name.clear();
}
