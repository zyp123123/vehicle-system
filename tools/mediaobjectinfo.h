#ifndef MEDIAOBJECTINFO_H
#define MEDIAOBJECTINFO_H

#include <QString>

struct MediaObjectInfo
{
    QString filePath;
    QString title;
    QString artist;
    qint64  duration = 0;
};

#endif
