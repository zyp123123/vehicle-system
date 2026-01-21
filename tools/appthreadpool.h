#ifndef APPTHREADPOOL_H
#define APPTHREADPOOL_H

#include <QThreadPool>

class AppThreadPool
{
public:
    static QThreadPool* instance();
};

#endif // APPTHREADPOOL_H
