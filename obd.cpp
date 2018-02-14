#include "obd.h"
#include "serialport.h"
#include "dashboard.h"
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFile>
#include <QTextStream>
#include <QByteArrayMatcher>
#include <QProcess>



int reqquestInd = 0; //ID for requested data type Power FC
int ExpectedBytes;
QByteArray checksumh;
QByteArray recvchecksumh;



OBD::OBD(QObject *parent)
    : QObject(parent)
    , m_dashboard(Q_NULLPTR)

{

}
OBD::OBD(DashBoard *dashboard, QObject *parent)
    : QObject(parent),
    m_dashboard(dashboard),
    m_serial(Q_NULLPTR),
    m_bytesWritten(0)
{
}


void OBD::initSerialPort()
{


    if (m_serial)
        delete m_serial;
    m_serial = new SerialPort(this);
    connect(this->m_serial,SIGNAL(readyRead()),this,SLOT(readyToRead()));
    connect(m_serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &OBD::handleError);
    connect(m_serial, &QSerialPort::bytesWritten, this, &OBD::handleBytesWritten);
    connect(&m_timer, &QTimer::timeout, this, &OBD::handleTimeout);
    m_readData.clear();
    qDebug() <<("Initialized");


}

//function for flushing all serial buffers
void OBD::clear() const
{
    m_serial->clear();
}


//function to open serial port
void OBD::openConnection(const QString &portName)
{

    qDebug() <<("Opening Port")<<portName;

    initSerialPort();

    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud38400); //initial standard baud for ELM327 is either 9600 or 384000
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);;

    if(m_serial->open(QIODevice::ReadWrite) == false)
    {
       qDebug() <<("not open");//m_dashBoard->setSerialStat(m_serial->errorString());
    }
    else
    {
        qDebug() <<("open");
       //m_dashBoard->setSerialStat(QString("Connected to Serialport"));
    }

    reqquestInd = 0;
    OBD::sendRequest(reqquestInd);

}

void OBD::closeConnection()

{
    m_serial->close();
}

void OBD::handleTimeout()
{
    m_timer.stop();
    qDebug() <<("timeou message") << m_buffer;
    if(reqquestInd <= 7){reqquestInd++;}
    m_readData.clear();
    m_buffer.clear();
    OBD::sendRequest(reqquestInd);
}

void OBD::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        QString fileName = "Errors.txt";
        QFile mFile(fileName);
        if(!mFile.open(QFile::Append | QFile::Text)){
        }
        QTextStream out(&mFile);
        out << "Serial Error " << (m_serial->errorString()) <<endl;
        mFile.close();
        //m_dashBoard->setSerialStat(m_serial->errorString());

    }
}


void OBD::readyToRead()
{

    m_readData = m_serial->readAll();
    qDebug() <<("reading") << m_readData;
    OBD::messageconstructor(m_readData);

}

void OBD::messageconstructor(const QByteArray &buffer)
{
    m_timer.start(5000);
    m_buffer.append(buffer);
    qDebug() <<("constructed")<<m_buffer;
    QByteArray msgEnd = (QByteArray::fromStdString(">"));
    if (m_buffer.contains(msgEnd))
    {
         m_timer.stop();
        m_message = m_buffer;
        int end = m_message.indexOf(msgEnd);
        qDebug() <<("found MSG end at pos")<<end;
        qDebug() <<("raw msg")<<m_message;
        m_message.remove(end+1,m_message.length()-end);
        qDebug() <<("final msg")<<m_message;
        m_buffer.remove(0,end+1);
        qDebug() <<("buffer - message")<<m_buffer;
        //m_dashBoard->setSerialStat(m_message);
        readData(m_message);
        reqquestInd++;
        OBD::sendRequest(reqquestInd);
    }




    //just for testing

     //m_timer.start(15000);

    /*
    if (ExpectedBytes != m_buffer.length())
    {
        m_timer.start(5000);
    }

    m_buffer.append(buffer);

    QByteArray startpattern = m_writeData.left(1);
    QByteArrayMatcher startmatcher(startpattern);

    int pos = 0;
    while((pos = startmatcher.indexIn(m_buffer, pos)) != -1)
    {
        if (pos !=0)
        {
            m_buffer.remove(0, pos);
            if (m_buffer.length() > ExpectedBytes)
            {
                m_buffer.remove(ExpectedBytes,m_buffer.length() );
            }
        }

        if (pos == 0 )
        {
            break;
        }


    }

    if (m_buffer.length() == ExpectedBytes)
    {
        m_apexiMsg =  m_buffer;
        m_buffer.clear();
        m_timer.stop();
        if(reqquestInd <= 5){reqquestInd++;}
        else{reqquestInd = 2;}
        readData(m_apexiMsg);
        m_apexiMsg.clear();
        OBD::sendRequest(reqquestInd);
    }
*/
}




void OBD::readData(QByteArray serialdata)
{

    if( serialdata.length() )
    {
        //Power FC Decode
        quint8 requesttype = serialdata[0];

        //Write all OK Serial Messages to a file
        if(serialdata[1] + 1 == serialdata.length())
        {
/*
            switch (requesttype) {
            case APEXI::DATA::Advance:
                m_decoderapexi->decodeAdv(serialdata);
                break;
            default:
                break;
            }
*/


         //   if(requesttype == APEXI::DATA::Advance) {m_decoderapexi->decodeAdv(serialdata);}


       }
        serialdata.clear();

    }


}
void OBD::handleBytesWritten(qint64 bytes)
{
    m_bytesWritten += bytes;
    if (m_bytesWritten == m_writeData.size()) {
        m_bytesWritten = 0;
        //qDebug() <<("Data successfully sent to port") << (m_serial->portName());

    }
}
// Serial requests are send via Serial
void OBD::writeRequest(QByteArray p_request)
{
    qDebug() << "Sending Request" << p_request;
    m_writeData = p_request;
    qint64 bytesWritten = m_serial->write(p_request);
    ////m_dashBoard->setSerialStat(QString("Sending Request " + p_request.toHex()));

    //Action to be implemented
    if (bytesWritten == -1) {
     //   //m_dashBoard->setSerialStat(m_serial->errorString());
        //qDebug() << "Write request to port failed" << (m_serial->errorString());
    } else if (bytesWritten != m_writeData.size()) {
     //   //m_dashBoard->setSerialStat(m_serial->errorString());
        //qDebug() << "could not write complete request to port" << (m_serial->errorString());
    }

}



void OBD::sendRequest(int reqquestInd)
{
    switch (reqquestInd){

    // Setup the OBD ELM Device
       case 0:
           // Reset Adapter ELM OBD Adapter
           OBD::writeRequest(QByteArray::fromStdString("ATZ\r"));
           qDebug() <<("ELM Device reset ");
           break;
       case 1:
           // Reset Adapter ELM OBD Adapter
           OBD::writeRequest(QByteArray::fromStdString("ATI\r"));
           qDebug() <<("Request name of ELM Device");
           break;

       case 2:
           // disable extended response
           OBD::writeRequest(QByteArray::fromStdString("ATE0\r"));
           qDebug() <<("Disable  extended response");
           break;

       case 3:
           // Disable extended response
           OBD::writeRequest(QByteArray::fromStdString("ATL0\r"));
           qDebug() <<("Disable  extended response");
           break;

       case 4:
           // Enable header for response
           OBD::writeRequest(QByteArray::fromStdString("ATH1\r"));
           qDebug() <<("Enable Response Header");
           break;

       case 5:
           // Autodetect protocol
           OBD::writeRequest(QByteArray::fromStdString("ATSP00\r"));
           qDebug() <<("Autodetect Protocol");
           break;

       case 6:
           // Test communication
           OBD::writeRequest(QByteArray::fromStdString("0001\r"));
           qDebug() <<("Check if ELM communication is OK ");
           break;
       case 7:
           // Test communication
           OBD::writeRequest(QByteArray::fromStdString("0100\r"));
           qDebug() <<("Request supported pids ");
           break;
       case 8:
           // Test RPM
           OBD::writeRequest(QByteArray::fromStdString("010C\r"));
           qDebug() <<("Request RPM ");
           break;
// PID requests

        break;
    }
}
