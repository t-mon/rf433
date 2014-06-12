#include "gpio.h"
#include <QDebug>

/*! Constructs a \l{Gpio} object to represent a GPIO with the given \a gpio number and the \a parent. */
Gpio::Gpio(QObject *parent, int gpio) :
    QThread(parent),m_gpio(gpio)
{
    exportGpio();
}

/*! Destroys the Gpio object and unexports the GPIO. */
Gpio::~Gpio()
{
    unexportGpio();
}

/*! The starting point for the thread. After calling start(), this thread calls this function and starts
 *  to poll the GPIO file. If an interrupt happens to this GPIO pin, the signal pinInterrupt() gets
 *  emited.
 */
void Gpio::run()
{
    struct pollfd fdset[2];
    int gpio_fd = openGpio();
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
            emit pinInterrupt();
        }
        m_mutex.lock();
        enabled = m_enabled;
        m_mutex.unlock();
    }
}
/*! Returns true if the GPIO could be exported in the system file "/sys/class/gpio/export".*/
bool Gpio::exportGpio()
{
    char buf[64];

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio/export";
        return false;
    }

    int len = snprintf(buf, sizeof(buf), "%d", m_gpio);
    write(fd, buf, len);
    close(fd);

    return true;
}

/*! Returns true if the GPIO could be unexported in the system file "/sys/class/gpio/unexport".*/
bool Gpio::unexportGpio()
{
    char buf[64];

    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio/unexport";
        return false;
    }
    int len = snprintf(buf, sizeof(buf), "%d", m_gpio);
    write(fd, buf, len);
    close(fd);

    return true;
}

/*! Returns true if the GPIO file could be opend.*/
int Gpio::openGpio()
{
    char buf[64];

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);

    int fd = open(buf, O_RDONLY | O_NONBLOCK );
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/direction";
        return fd;
    }
    return fd;
}

/*! Returns true, if the direction \a dir of the GPIO could be set correctly.
 *
 * Possible directions are:
 *
 * \table
 * \header
 *      \li {2,1} Pin directions
 * \row
 *      \li 0
 *      \li INPUT
 * \row
 *      \li 1
 *      \li OUTPUT
 * \endtable
 */
bool Gpio::setDirection(int dir)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", m_gpio);

    int fd = open(buf, O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/direction";
        return false;
    }
    if(dir == INPUT){
        write(fd, "in", 3);
        m_dir = INPUT;
        close(fd);
        return true;
    }
    if(dir == OUTPUT){
        write(fd, "out", 4);
        m_dir = OUTPUT;
        close(fd);
        return true;
    }
    close(fd);
    return false;
}
/*! Returns true, if the digital \a value of the GPIO could be set correctly.
 *
 * Possible \a value 's are:
 *
 * \table
 * \header
 *      \li {2,1} Pin value
 * \row
 *      \li 0
 *      \li LOW
 * \row
 *      \li 1
 *      \li HIGH
 * \endtable
 */
bool Gpio::setValue(unsigned int value)
{
    // check if gpio is a output
    if( m_dir == OUTPUT){
        char buf[64];
        snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);

        int fd = open(buf, O_WRONLY);
        if (fd < 0) {
            qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/value";
            return false;
        }

        if(value == LOW){
            write(fd, "0", 2);
            close(fd);
            return true;
        }
        if(value == HIGH){
            write(fd, "1", 2);
            close(fd);
            return true;
        }
        close(fd);
        return false;
    }else{
        qDebug() << "ERROR: Gpio" << m_gpio << "is not a OUTPUT.";
        return false;
    }
}
/*! Returns the current digital value of the GPIO.
 *
 * Possible values are:
 *
 * \table
 * \header
 *      \li {2,1} Pin directions
 * \row
 *      \li 0
 *      \li LOW
 * \row
 *      \li 1
 *      \li HIGH
 * \endtable
 */
int Gpio::getValue()
{
    char buf[64];

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);

    int fd = open(buf, O_RDONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/value";
        return -1;
    }
    char ch;
    int value = -1;
    read(fd, &ch, 1);

    if (ch != '0') {
        value = 1;
    }else{
        value = 0;
    }
    close(fd);

    qDebug() << "gpio" << m_gpio << "value = " << value;
    return value;
}

/*! Returns true, if the \a edge of the GPIO could be set correctly. The \a edge parameter specifies,
 *  when an interrupt occurs.
 *
 * Possible values are:
 *
 * \table
 * \header
 *      \li {2,1} Edge possibilitys
 * \row
 *      \li 0
 *      \li EDGE_FALLING
 * \row
 *      \li 1
 *      \li EDGE_RISING
 * \row
 *      \li 2
 *      \li EDGE_BOTH
 * \endtable
 */
bool Gpio::setEdgeInterrupt(int edge)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", m_gpio);

    int fd = open(buf, O_WRONLY);
    if (fd < 0) {
        qDebug() << "ERROR: could not open /sys/class/gpio" << m_gpio << "/edge";
        return false;
    }

    if(edge == EDGE_FALLING){
        write(fd, "falling", 8);
        close(fd);
        return true;
    }
    if(edge == EDGE_RISING){
        write(fd, "rising", 7);
        close(fd);
        return true;
    }
    if(edge == EDGE_BOTH){
        write(fd, "both", 5);
        close(fd);
        return true;
    }
    close(fd);
    return false;
}

/*! Stop the polling of the \l{run()} method.*/
void Gpio::stop()
{
    m_mutex.lock();
    m_enabled = false;
    m_mutex.unlock();
}
