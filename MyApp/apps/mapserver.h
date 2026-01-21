#ifndef MAPSERVER_H
#define MAPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>

class MapServer : public QObject
{
    Q_OBJECT
public:
    explicit MapServer(QObject *parent = nullptr);

    // 启动 TCP 服务
    bool start(quint16 port);

signals:
    void logMessage(const QString &msg);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QTcpServer *tcpServer;
    QTcpSocket *currentClient;

    // 保存客户端发送的数据
    QString city;
    QString startLocation;   // 修改名称，避免和 start() 函数冲突
    QString endLocation;     // 修改名称
};

#endif // MAPSERVER_H
