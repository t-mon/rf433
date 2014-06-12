#include "core.h"

Core::Core(QObject *parent) :
    QObject(parent)
{
    m_receiver = new Radio433Receiver(this,27);
    m_receiver->run();
}

Core::~Core()
{
    m_receiver->quit();
    m_receiver->deleteLater();
}
