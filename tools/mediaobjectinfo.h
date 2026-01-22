#ifndef MEDIAOBJECTINFO_H
#define MEDIAOBJECTINFO_H

#include <QString>

struct MediaObjectInfo {
    QString filePath;
    QString fileName; // 添加这一行，或者将 musicplayer.cpp 里的 fileName 改为 title
    QString title;
    QString artist;
    qint64  duration = 0;
};
#endif
