#ifndef RADIO433RECEIVER_H
#define RADIO433RECEIVER_H

#include <QThread>
#include <QDebug>
#include <QMutex>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

class Radio433Receiver : public QThread
{
    Q_OBJECT
public:
    explicit Radio433Receiver(QObject *parent = 0, int gpio = 0);
    ~Radio433Receiver();

    void run() override;
    void stop();

private:
    int m_gpio;
    unsigned int m_epochMicro;
    unsigned long m_lastTime;

    QList<uint> m_timings;

    QMutex m_mutex;
    bool m_enabled;

    bool setUpGpio();
    int micros();
    bool valueInTolerance(int value, int sollValue);
    bool valueIsValid(int value);

private slots:
    void handleInterrupt();

signals:
    void dataReceived(QList<uint> rawData);

public slots:

};

#endif // RADIO433RECEIVER_H
