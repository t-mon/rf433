#ifndef CORE_H
#define CORE_H

#include <QObject>

#include <radio433.h>

class Core : public QObject
{
    Q_OBJECT
public:
    explicit Core(QObject *parent = 0);
    ~Core();

    Radio433 *m_radio433;

signals:

public slots:

};

#endif // CORE_H
