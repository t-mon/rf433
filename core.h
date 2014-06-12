#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <radio433receiver.h>

class Core : public QObject
{
    Q_OBJECT
public:
    explicit Core(QObject *parent = 0);
    ~Core();

    Radio433Receiver *m_receiver;

signals:

public slots:

};

#endif // CORE_H
