#include "mapserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QBuffer>
#include <QTimer>
#include <QCoreApplication>
#include <QDataStream>

MapServer::MapServer(MapPage *page, QObject *parent)
    : QObject(parent),
      tcpServer(new QTcpServer(this)),
      currentClient(nullptr),
      mapPage(page)
{
    connect(tcpServer, &QTcpServer::newConnection, this, &MapServer::onNewConnection);
}

bool MapServer::start(quint16 port)
{
    if(!tcpServer->listen(QHostAddress::Any, port)){
        emit logMessage(QString("地图服务监听端口 %1 失败").arg(port));
        return false;
    }
    emit logMessage(QString("地图服务已启动，端口：%1").arg(port));
    return true;
}

void MapServer::sendMapImage()
{
    if (!currentClient || currentClient->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "无有效客户端连接，无法发送截图";
        return;
    }

    // 1. 获取地图截图并转为 PNG 字节数组
    QImage img = mapPage->captureMapImage();
    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");

    quint32 length = pngBytes.size();

    // 2. 构造 4 字节长度头（大端序 Network Byte Order）
    QByteArray header;
    header.append(static_cast<char>((length >> 24) & 0xFF));
    header.append(static_cast<char>((length >> 16) & 0xFF));
    header.append(static_cast<char>((length >> 8) & 0xFF));
    header.append(static_cast<char>(length & 0xFF));

    // 3. 顺序写入：长度头 + 图片数据
    currentClient->write(header);
    currentClient->write(pngBytes);
    currentClient->flush();

    qDebug() << "已发送图片数据，PNG大小:" << length << "字节";
}

void MapServer::onNewConnection()
{
    while(tcpServer->hasPendingConnections()){
        QTcpSocket *client = tcpServer->nextPendingConnection();
        currentClient = client;
        emit logMessage("客户端连接：" + client->peerAddress().toString());

        connect(client, &QTcpSocket::readyRead, this, &MapServer::onClientReadyRead);

        // 客户端断开处理（建议添加）
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }
}

void MapServer::onClientReadyRead()
{
    currentClient = qobject_cast<QTcpSocket *>(sender());
    if (!currentClient) return;

    QByteArray data = currentClient->readAll().trimmed();
    emit logMessage("收到客户端数据：" + QString(data));

    // 解析 JSON 指令
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage("解析 JSON 错误");
        return;
    }

    QJsonObject obj = doc.object();
    QString city = obj.value("city").toString();
    QString start = obj.value("start").toString();
    QString end = obj.value("end").toString();

    emit logMessage(QString("城市: %1, 起点: %2, 终点: %3").arg(city, start, end));

    // --- 填充数据到 UI 并执行导航 ---
    if (mapPage) {
        mapPage->fillNavigationData(city, start, end);
    }

    // ★★★ 延迟 2 秒发送截图 ★★★
    // 目的：等待网页端路径渲染、动画加载完成后再进行截图
    QTimer::singleShot(2000, this, &MapServer::sendMapImage);
}
