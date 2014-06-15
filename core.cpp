#include "core.h"

Core::Core(QObject *parent) :
    QObject(parent)
{
    m_radio433 = new Radio433(this);
}

Core::~Core()
{

}
