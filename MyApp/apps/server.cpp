#include "server.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

Server::Server(QObject *parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    networkManager = new QNetworkAccessManager(this);
    currentClient = nullptr;

    connect(tcpServer, &QTcpServer::newConnection,
            this, &Server::onNewConnection);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &Server::onNetworkReplyFinished);
}

bool Server::start(quint16 port)
{
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        emit logMessage(QString("监听端口 %1 失败").arg(port));
        return false;
    }

    emit logMessage(QString("天气服务器已启动，端口：%1").arg(port));
    return true;
}

void Server::onNewConnection()
{
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket *client = tcpServer->nextPendingConnection();
        emit logMessage("客户端连接：" + client->peerAddress().toString());

        connect(client, &QTcpSocket::readyRead,
                this, &Server::onClientReadyRead);
        connect(client, &QTcpSocket::disconnected,
                client, &QTcpSocket::deleteLater);
    }
}

void Server::onClientReadyRead()
{
    currentClient = qobject_cast<QTcpSocket *>(sender());
    if (!currentClient) return;

    cityRequest = currentClient->readAll().trimmed();
    emit logMessage("收到城市代码：" + QString(cityRequest));

    // 注意：实际应用中应将API key放在配置文件中或使用环境变量
    QString key = "d6aab55a0d7d60f05151a9a0dbe73ae3";
    QString urlStr = QString(
        "http://restapi.amap.com/v3/weather/weatherInfo?"
        "key=%1&city=%2&extensions=base"
    ).arg(key, QString(cityRequest));

    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(urlStr)));

    // 为每个回复单独连接信号，确保回复与客户端对应
    connect(reply, &QNetworkReply::finished,
            this, &Server::onNetworkReplyFinished);
}

void Server::onNetworkReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        emit logMessage("网络错误：" + reply->errorString());
        reply->deleteLater();
        return;
    }

    // 读取返回数据
    QByteArray data = reply->readAll();
    emit logMessage("返回天气 JSON：" + QString::fromUtf8(data));

    // 回写给客户端
    if (currentClient) {
        currentClient->write(data);
        currentClient->flush();
        currentClient->disconnectFromHost();
    }

    reply->deleteLater();
    currentClient = nullptr;
}
