#include "socketresponse.h"
#include <QXmlStreamWriter>
#include <QBuffer>
#include <QTcpSocket>
#include <QUdpSocket>

SocketResponse::SocketResponse(QAbstractSocket* socket, QByteArray address, quint16 port)
{
    this->socket = socket;
    this->statusText = "0";     //0:unimplemented, 1:success, 2:general error
    this->address = address;
    this->port = port;
}

SocketResponse::~SocketResponse()
{
    this->socket = NULL;
    this->statusText = "";
    this->commandText = "";
    this->parameters.clear();
}

void SocketResponse::setBroadcast()
{
    this->address = "";
}

void SocketResponse::setStatus(QByteArray statusName)
{
    this->statusText = statusName;
}

void SocketResponse::setStatus(int statusCode)
{
    this->statusText = QByteArray::number(statusCode);
}

void SocketResponse::setCommand(QByteArray commandName)
{
    this->commandText = commandName;
}

void SocketResponse::setParameter(QByteArray paramName, int paramValue)
{
    setParameter(paramName, QByteArray::number(paramValue));
}

void SocketResponse::setParameter(QByteArray paramName, float paramValue)
{
    setParameter(paramName, QByteArray::number(paramValue));
}

void SocketResponse::setParameter(QByteArray paramName, QByteArray paramValue)
{
    parameters.insert(paramName, paramValue);
}

QMap<QByteArray,QByteArray>& SocketResponse::getParameters()
{
    return parameters;
}

int SocketResponse::getParametersCount()
{
    return parameters.count();
}

void SocketResponse::write()
{
    //This is a fake socket response (to handle status bar message)
    if (this->socket == NULL || this->port == 0 || this->address.length() < 1)
        return;

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    //Output format follows this https://internal.chumby.com/wiki/index.php/JavaScript/HTML_-_Hardware_Bridge_protocol

    QXmlStreamWriter *xmlfile = new QXmlStreamWriter();
    xmlfile->setDevice(&buffer);

    xmlfile->writeStartElement("xml");
    if (commandText != "")          xmlfile->writeTextElement("cmd", commandText);
    else if (statusText != "")      xmlfile->writeTextElement("status", statusText);

    if (parameters.count() == 1)
    {
        //this allow printing multiple key/value pairs from external scripts
        if (parameters.contains("data"))
        {
            buffer.write("<data>");
            buffer.write(parameters.value("data"));
            buffer.write("</data>");

            //Don't want this. This will escape any XML syntax already in data
            //xmlfile->writeTextElement("data", parameters.value("data"));
        }

        //this standardize printing 1 single parameter of any name
        else
        {
            xmlfile->writeStartElement( "data" );
            QMapIterator<QByteArray, QByteArray> i(parameters);
            while (i.hasNext())
            {
                 i.next();
                 xmlfile->writeTextElement("value", i.value());
            }
            xmlfile->writeEndElement();
        }
    }
    else if (parameters.count() > 1)
    {
        xmlfile->writeStartElement( "data" );
        QMapIterator<QByteArray, QByteArray> i(parameters);
        while (i.hasNext())
        {
             i.next();
             xmlfile->writeTextElement(i.key(), i.value());
        }
        xmlfile->writeEndElement();
    }
    xmlfile->writeEndElement();

    delete xmlfile;

    buffer.close();
    writeToSocket(byteArray);

    byteArray.clear();
}

void SocketResponse::writeToSocket(QByteArray data)
{
    int remaining = data.size();
    char* ptr=data.data();

    QTcpSocket *tcpsocket = qobject_cast<QTcpSocket*>(socket);
    QUdpSocket *udpsocket = qobject_cast<QUdpSocket*>(socket);

    if (tcpsocket)
    {
        while (socket->isOpen() && remaining>0)
        {
            int written = socket->write(data);
            ptr += written;
            remaining -= written;
        }
    }
    else if (udpsocket)
    {

        if (address == "")
            udpsocket->writeDatagram(data, QHostAddress(QHostAddress::Broadcast), port);
        else
            udpsocket->writeDatagram(data, QHostAddress(QString(address)), port);
    }
}
