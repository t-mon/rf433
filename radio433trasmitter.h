#ifndef RADIO433TRASMITTER_H
#define RADIO433TRASMITTER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QDebug>

#include "gpio.h"


class Radio433Trasmitter : public QThread
{
    Q_OBJECT
public:
    explicit Radio433Trasmitter(QObject *parent = 0, int gpio = 22);
    ~Radio433Trasmitter();

    void startTransmitter();
    void stopTransmitter();

    void sendData(QList<int> rawData);

private:
    int m_gpioPin;
    Gpio *m_gpio;

    QMutex m_mutex;
    bool m_enabled;

    QMutex m_allowSendingMutex;
    bool m_allowSending;

    void run();
    bool setUpGpio();

    QMutex m_queueMutex;
    QQueue<QList<int> > m_rawDataQueue;

signals:

public slots:
    void allowSending(bool sending);

};

#endif // RADIO433TRASMITTER_H
