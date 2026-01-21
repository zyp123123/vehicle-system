#include "mapserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

MapServer::MapServer(QObject *parent)
    : QObject(parent),
      tcpServer(new QTcpServer(this)),
      currentClient(nullptr)
{
    connect(tcpServer, &QTcpServer::newConnection,
            this, &MapServer::onNewConnection);
}

bool MapServer::start(quint16 port)
{
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        emit logMessage(QString("地图服务监听端口 %1 失败").arg(port));
        return false;
    }

    emit logMessage(QString("地图服务已启动，端口：%1").arg(port));
    return true;
}

void MapServer::onNewConnection()
{
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket *client = tcpServer->nextPendingConnection();
        emit logMessage("客户端连接：" + client->peerAddress().toString());

        connect(client, &QTcpSocket::readyRead,
                this, &MapServer::onClientReadyRead);
    }
}

void MapServer::onClientReadyRead()
{
    currentClient = qobject_cast<QTcpSocket *>(sender());
    if (!currentClient) return;

    QByteArray data = currentClient->readAll().trimmed();
    emit logMessage("收到客户端数据：" + QString(data));

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage("解析 JSON 错误");
        return;
    }

    QJsonObject obj = doc.object();

    // 更新成员变量
    city = obj.value("city").toString();
    startLocation = obj.value("start").toString();   // 修改名称
    endLocation = obj.value("end").toString();       // 修改名称

    emit logMessage(QString("城市: %1, 起点: %2, 终点: %3")
                    .arg(city, startLocation, endLocation));

    // TODO: 这里可以调用 MapPage 的函数填充 UI 并开始导航
    // 例如：
    // mapPage->setCity(city);
    // mapPage->setStart(startLocation);
    // mapPage->setEnd(endLocation);
    // mapPage->startNavigation();

    // 发送成功响应给客户端
    if (currentClient) {
        currentClient->write("ok");
        currentClient->flush();
        currentClient->disconnectFromHost();
    }

    currentClient = nullptr;
}
