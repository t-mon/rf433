#include "radio433.h"

Radio433::Radio433(QObject *parent) :
    QObject(parent)
{
    m_receiver = new Radio433Receiver();
    m_transmitter = new Radio433Trasmitter();

    connect(m_receiver,SIGNAL(readingChanged(bool)),this,SLOT(readingChanged(bool)));

    m_receiver->startReceiver();
    m_transmitter->startTransmitter();
}

Radio433::~Radio433()
{
    m_receiver->stopReceiver();
    m_receiver->quit();
    m_receiver->deleteLater();

    m_transmitter->stopTransmitter();
    m_transmitter->quit();
    m_transmitter->deleteLater();
}

void Radio433::readingChanged(bool reading)
{
    if(reading){
        m_transmitter->allowSending(false);
    }else{
        m_transmitter->allowSending(true);
    }
}
