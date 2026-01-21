#include "weatherclient.h"
#include <QDebug>

Client::Client(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected,
            this, &Client::onConnected);

    connect(socket, &QTcpSocket::readyRead,
            this, &Client::onReadyRead);

    connect(socket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &Client::onError);
}

void Client::requestWeather(const QString &ip, int port, const QString &cityCode)
{
    // 如果 socket 处于已连接状态，需要断开
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort();
    }

    // 保存要发送的城市编码
    socket->setProperty("cityCode", cityCode);

    socket->connectToHost(ip, port);
}

void Client::onConnected()
{
    QString cityCode = socket->property("cityCode").toString();
    socket->write(cityCode.toUtf8());  // 发给服务器
}

void Client::onReadyRead()
{
    QByteArray data = socket->readAll();
    QString json = QString::fromUtf8(data);

    emit weatherReceived(json);   // 发给 Weather 页面

    socket->disconnectFromHost();
}

void Client::onError(QAbstractSocket::SocketError err)
{
    emit errorOccurred(socket->errorString());
}
