#include "socketrequest.h"
#include <QXmlStreamReader>
#include <QString>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QHostInfo>

SocketRequest::SocketRequest(QByteArray data, QByteArray address, quint16 port)
{
    this->commandText = "no-no-no-no-command";
    this->containsError = false;
    this->address = address;
    this->port = port;

    //Parse the XML string
    this->ParseMessageXML( data );

    //Sanity check
    if (commandText == "" || commandText == "no-no-no-no-command")
        this->containsError = true;
}

SocketRequest::~SocketRequest()
{
    this->containsError = true;
    this->commandText = "";
    this->address = "";
    this->parameters.clear();
}

bool SocketRequest::hasError()
{
    return containsError;
}

QByteArray SocketRequest::getCommand()
{
    return this->commandText;
}

QByteArray SocketRequest::getAddress()
{
    return this->address;
}

QByteArray SocketRequest::getLocalAddress()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();

    //Get own IP addresses
    for (int i = 0; i < list.size(); i++)
        if (list[i].protocol() == QAbstractSocket::IPv4Protocol && !list[i].toString().startsWith("127") && !list[i].toString().startsWith("169"))
            return list[i].toString().toLatin1();
    return "";
}

quint16 SocketRequest::getPort()
{
    return this->port;
}

QByteArray SocketRequest::getParameter(QByteArray paramName)
{
    return parameters.value(paramName);
}

QMap<QByteArray,QByteArray>& SocketRequest::getParameters()
{
    return parameters;
}

int SocketRequest::getParametersCount()
{
    return parameters.count();
}

void SocketRequest::ParseMessageXML(const char* data)
{
    QXmlStreamReader* xml = new QXmlStreamReader();
    xml->addData(data);

    //Input format is similar to this https://internal.chumby.com/wiki/index.php/JavaScript/HTML_-_Hardware_Bridge_protocol
    //Example: <cmd>PlayWidget</cmd><data><value>1234567890</value></data>

    QString currentTag;
    bool isFirstElement = true;

    while (!xml->atEnd())
    {
        xml->readNext();

        if (xml->isStartElement() && isFirstElement)
        {
            isFirstElement = false;
        }
        else if (xml->isStartElement())
        {
            currentTag = xml->name().toString().trimmed();

            if (currentTag == "cmd")
                commandText = xml->readElementText().toLatin1();

            else if (currentTag != "data")
                parameters.insert(currentTag.toLatin1(), xml->readElementText().toLatin1());
        }
    }
    currentTag = "";
    delete xml;
}
