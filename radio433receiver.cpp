#include "radio433receiver.h"
#include <QFile>
#include <QFileSystemWatcher>
#include <sys/time.h>

Radio433Receiver::Radio433Receiver(QObject *parent, int gpio) :
    QThread(parent),m_gpioPin(gpio)
{
    stopReceiver();

    connect(this,SIGNAL(timingReady(int)),this,SLOT(handleTiming(int)),Qt::DirectConnection);
}

Radio433Receiver::~Radio433Receiver()
{
//    stopReceiver();
//    quit();
}

void Radio433Receiver::startReceiver()
{
    m_mutex.lock();
    m_enabled = true;
    m_mutex.unlock();

    m_timings.clear();

    run();
}

void Radio433Receiver::stopReceiver()
{
    m_mutex.lock();
    m_enabled = false;
    m_mutex.unlock();
}

void Radio433Receiver::run()
{
    // init the gpio
    setUpGpio();

    struct pollfd fdset[2];
    int gpio_fd = m_gpio->openGpio();
    char buf[1];

    bool enabled = true;

    m_mutex.lock();
    m_enabled = true;
    m_mutex.unlock();

    int lastTime = micros();
    // poll the gpio file, if something changes...emit the signal wit the current pulse length
    while(enabled){
        memset((void*)fdset, 0, sizeof(fdset));
        fdset[0].fd = STDIN_FILENO;
        fdset[0].events = POLLIN;

        fdset[1].fd = gpio_fd;
        fdset[1].events = POLLPRI;
        int rc = poll(fdset, 2, 3000);

        if (rc < 0) {
            qDebug() << "ERROR: poll failed";
            return;
        }
        if(rc == 0){
            //timeout
        }
        if (fdset[1].revents & POLLPRI){
            read(fdset[1].fd, buf, 1);
            int currentTime = micros();
            int duration = currentTime - lastTime;
            lastTime = currentTime;
            emit timingReady(duration);
        }

        m_mutex.lock();
        enabled = m_enabled;
        m_mutex.unlock();
    }
}

bool Radio433Receiver::setUpGpio()
{
    m_gpio = new Gpio(this,m_gpioPin);
    m_gpio->setDirection(INPUT);
    m_gpio->setEdgeInterrupt(EDGE_BOTH);

    return true;
}

int Radio433Receiver::micros()
{
    struct timeval tv ;
    int now ;
    gettimeofday (&tv, NULL) ;
    now  = (int)tv.tv_sec * (int)1000000 + (int)tv.tv_usec ;

    return (int)(now - m_epochMicro) ;
}

bool Radio433Receiver::valueInTolerance(int value, int sollValue)
{
    // in in range of +- 35% of sollValue return true, eles return false
    if(value >= (double)sollValue*0.7 && value <= (double)sollValue*1.3){
        return true;
    }
    return false;
}

bool Radio433Receiver::checkValue(int value)
{
    if(valueInTolerance(value,m_pulseProtocolOne) || valueInTolerance(value,2*m_pulseProtocolOne) || valueInTolerance(value,3*m_pulseProtocolOne) || valueInTolerance(value,4*m_pulseProtocolOne) || valueInTolerance(value,8*m_pulseProtocolOne)){
        return true;
    }
    if(valueInTolerance(value,m_pulseProtocolTwo) || valueInTolerance(value,2*m_pulseProtocolTwo) || valueInTolerance(value,3*m_pulseProtocolTwo) || valueInTolerance(value,4*m_pulseProtocolTwo) || valueInTolerance(value,8*m_pulseProtocolTwo)){
        return true;
    }
    return false;
}

bool Radio433Receiver::checkValues(Protocol protocol)
{
    switch (protocol) {
    case Protocol48:
        for(int i = 1; i < m_timings.count(); i++){
            if(!(valueInTolerance(m_timings.at(i),m_pulseProtocolOne) || valueInTolerance(m_timings.at(i),2*m_pulseProtocolOne) || valueInTolerance(m_timings.at(i),3*m_pulseProtocolOne) || valueInTolerance(m_timings.at(i),4*m_pulseProtocolOne) || valueInTolerance(m_timings.at(i),8*m_pulseProtocolOne))){
                break;
            }
        }
        return true;
    case Protocol64:
        for(int i = 1; i < m_timings.count(); i++){
            if(!(valueInTolerance(m_timings.at(i),m_pulseProtocolTwo) || valueInTolerance(m_timings.at(i),2*m_pulseProtocolTwo) || valueInTolerance(m_timings.at(i),3*m_pulseProtocolTwo) || valueInTolerance(m_timings.at(i),4*m_pulseProtocolTwo) || valueInTolerance(m_timings.at(i),8*m_pulseProtocolTwo))){
                break;
            }
        }
        return true;
    default:
        break;
    }
    return false;
}

void Radio433Receiver::changeReading(bool reading)
{
    if(reading != m_reading){
        m_reading = reading;
        qDebug() << "reading" << reading;
        emit readingChanged(reading);
    }
}

void Radio433Receiver::handleTiming(int duration)
{

    // to short...
    if(duration < 60){
        changeReading(false);
        m_timings.clear();
        return;
    }

    // could by a sync signal...
    bool sync = false;
    if(duration > 2400 && duration < 14000){
        changeReading(false);
        sync = true;
    }

    // got sync signal and list is not empty...
    if(!m_timings.isEmpty() && sync){
        // 1 sync bit + 48 data bit
        if(m_timings.count() == 49 && checkValues(Protocol48)){
            qDebug() << "48 bit ->" << m_timings << "\n--------------------------";
            changeReading(false);
            emit dataReceived(m_timings);
        }

        // 1 sync bit + 64 data bit
        if(m_timings.count() == 65 && checkValues(Protocol64)){
            qDebug() << "64 bit ->" << m_timings << "\n--------------------------";
            changeReading(false);
            emit dataReceived(m_timings);
        }
        m_timings.clear();
        m_pulseProtocolOne = 0;
        m_pulseProtocolTwo = 0;
        changeReading(false);
    }

    // got sync signal and list is empty...
    if(m_timings.isEmpty() && sync){
        m_timings.append(duration);
        m_pulseProtocolOne = (int)((double)m_timings.first()/31);
        m_pulseProtocolTwo = (int)((double)m_timings.first()/10);
        changeReading(false);
        return;
    }

    // list not empty and this is a possible value
    if(!m_timings.isEmpty() && checkValue(duration)){
        m_timings.append(duration);

        // check if it could be a signal -> if we have a sync and 15 valid values
        // set reading true to prevent a collision from own transmitter
        if(m_timings.count() == 20 && (checkValues(Protocol48) || checkValues(Protocol64))){
            changeReading(true);
        }

        // check if we have allready a vallid protocol
        // 1 sync bit + 48 data bit
        if(m_timings.count() == 49 && checkValues(Protocol48)){
            qDebug() << "48 bit -> " << m_timings << "\n--------------------------";
            emit dataReceived(m_timings);
        }

        // 1 sync bit + 64 data bit
        if(m_timings.count() == 65 && checkValues(Protocol64)){
            qDebug() << "64 bit -> " << m_timings << "\n--------------------------";
            changeReading(false);
            emit dataReceived(m_timings);
        }
    }
}
