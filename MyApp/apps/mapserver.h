#ifndef MAPSERVER_H
#define MAPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include "mappage.h"

class MapServer : public QObject
{
    Q_OBJECT
public:
    explicit MapServer(MapPage *page, QObject *parent=nullptr);
    bool start(quint16 port);
    void sendMapImage();   // 发送地图截图到客户端（二进制流格式）

signals:
    void logMessage(const QString &msg);
    void navigationCommand(const QString &city, const QString &start, const QString &end);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QTcpServer *tcpServer;
    QTcpSocket *currentClient;
    MapPage    *mapPage; // 指向 MapPage 实例
};

#endif // MAPSERVER_H
