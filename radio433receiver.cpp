#include "radio433receiver.h"
#include <sys/time.h>

Radio433Receiver::Radio433Receiver(QObject *parent, int gpio) :
    QThread(parent),m_gpio(gpio)
{
    // Set up receiver
    setUpGpio();
}

Radio433Receiver::~Radio433Receiver()
{
    stop();
}

void Radio433Receiver::run()
{
    struct pollfd fdset[2];

    char b[64];
    snprintf(b, sizeof(b), "/sys/class/gpio/gpio%d/value", m_gpio);

    int fd = open(b, O_RDONLY | O_NONBLOCK );
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/direction";
    }

    int gpio_fd = fd;
    int nfds = 2;
    int rc;
    int timeout = 3000;
    char buf[64];

    bool enabled = true;

    m_mutex.lock();
    m_enabled = true;
    m_mutex.unlock();

    while(enabled){
        memset((void*)fdset, 0, sizeof(fdset));
        fdset[0].fd = STDIN_FILENO;
        fdset[0].events = POLLIN;

        fdset[1].fd = gpio_fd;
        fdset[1].events = POLLPRI;

        rc = poll(fdset, nfds, timeout);

        if (rc < 0) {
            qDebug() << "ERROR: poll failed";
            return;
        }
        if(rc == 0){
            //timeout
        }
        if (fdset[1].revents & POLLPRI) {
            read(fdset[1].fd, buf, 64);
            handleInterrupt();
        }
        m_mutex.lock();
        enabled = m_enabled;
        m_mutex.unlock();
    }
}

void Radio433Receiver::stop()
{
    m_mutex.lock();
    m_enabled = false;
    m_mutex.unlock();
}

bool Radio433Receiver::setUpGpio()
{

    // export gpio
    char buf[64];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio/export";
        return false;
    }
    int len = snprintf(buf, sizeof(buf), "%d", m_gpio);
    write(fd, buf, len);
    close(fd);

    // set direction
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", m_gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/direction";
        return false;
    }
    write(fd, "in", 3);
    close(fd);

    // set edge interrupt both
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", m_gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/edge";
        return false;
    }
    write(fd, "both", 5);
    close(fd);
    qDebug() << "gpio setup ok";
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
    // in in range of +- 20% of sollValue return true, eles return false
    if((value <= (double)sollValue+300 && value >= (double)sollValue-300)){
        return true;
    }
    return false;
}

bool Radio433Receiver::valueIsValid(int value)
{
    int pulseProtocolOne = (int)((double)m_timings.first()/31);
    int pulseProtocolTwo = (int)((double)m_timings.first()/10);

    if(valueInTolerance(value,pulseProtocolOne) || valueInTolerance(value,2*pulseProtocolOne) || valueInTolerance(value,3*pulseProtocolOne) || valueInTolerance(value,4*pulseProtocolOne) || valueInTolerance(value,8*pulseProtocolOne)){
        return true;
    }

    if(valueInTolerance(value,pulseProtocolTwo) || valueInTolerance(value,2*pulseProtocolTwo) || valueInTolerance(value,3*pulseProtocolTwo) || valueInTolerance(value,4*pulseProtocolTwo) || valueInTolerance(value,8*pulseProtocolTwo)){
        return true;
    }

    return false;
}

void Radio433Receiver::handleInterrupt()
{
    int currentTime = micros();
    int duration = currentTime - m_lastTime;

    if(duration < 100){
        m_timings.clear();
        m_lastTime = currentTime;
        return;
    }

    bool sync = false;
    if(duration > 2000 && duration < 13000){
        sync = true;
    }

    // sync signal and list not empty
    if(!m_timings.isEmpty() && sync){
        if(m_timings.count() == 49){
            qDebug() << m_timings << "\n--------------------------";
            emit dataReceived(m_timings);
        }
        if(m_timings.count() == 65){
            qDebug() << m_timings << "\n--------------------------";
            emit dataReceived(m_timings);
        }
        m_timings.clear();
    }

    // if this is a sync signal and our timigs list is empty...
    if(m_timings.isEmpty() && sync){
        m_timings.append(duration);
        m_lastTime = currentTime;
        return;
    }

    // if we have allready a sync signal and this is a possible value
    if(!m_timings.isEmpty() && valueIsValid(duration)){
        m_timings.append(duration);
    }

    m_lastTime = currentTime;
}
