#include "appthreadpool.h"

QThreadPool* AppThreadPool::instance()
{
    static QThreadPool pool;
    static bool initialized = false;

    if (!initialized) {
        // 车机 / 嵌入式推荐 2~4
        pool.setMaxThreadCount(2);
        initialized = true;
    }

    return &pool;
}
