#include "weatherclient.h"
#include <QDebug>

Client::Client(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);

    // 兼容 Qt5 的错误处理信号写法
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));
}

void Client::requestWeather(const QString &ip, int port, const QString &cityCode)
{
    // 如果当前正在连接或已连接，先中断
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort();
    }

    // 使用动态属性暂存城市代码，待连接成功后发送
    socket->setProperty("cityCode", cityCode);
    socket->connectToHost(ip, port);
}

void Client::onConnected()
{
    QString cityCode = socket->property("cityCode").toString();
    socket->write(cityCode.toUtf8());  // 发送城市代码给服务端
    qDebug() << "TCP Connected, sent cityCode:" << cityCode;
}

void Client::onReadyRead()
{
    QByteArray data = socket->readAll();
    QString json = QString::fromUtf8(data);

    emit weatherReceived(json);   // 通知 UI 界面解析数据

    // 天气请求通常是短连接，收到数据后断开
    socket->disconnectFromHost();
}

void Client::onError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err);
    emit errorOccurred(socket->errorString());
}
