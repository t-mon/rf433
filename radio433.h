#ifndef RADIO433_H
#define RADIO433_H

#include <QObject>

#include "radio433receiver.h"
#include "radio433trasmitter.h"

class Radio433 : public QObject
{
    Q_OBJECT
public:
    explicit Radio433(QObject *parent = 0);
    ~Radio433();

    Radio433Receiver *m_receiver;
    Radio433Trasmitter *m_transmitter;

signals:

private slots:
    void readingChanged(bool reading);

public slots:

};

#endif // RADIO433_H
