#include "maptcpclient.h"
#include <QBuffer>
#include <QDebug>

MapTcpClient::MapTcpClient(QObject *parent)
    : QObject(parent),
      m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead, this, &MapTcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MapTcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MapTcpClient::onErrorOccurred);
}

MapTcpClient::~MapTcpClient() { }

void MapTcpClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void MapTcpClient::sendNavigationData(const QString &city, const QString &start, const QString &end)
{
    if (!m_socket->isOpen()) return;
    QByteArray data = city.toUtf8() + "\n" + start.toUtf8() + "\n" + end.toUtf8();
    m_socket->write(data);
    m_socket->flush();
}

void MapTcpClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    // 假设服务端发送的是完整的PNG或JPEG图片
    QImage img;
    if (img.loadFromData(m_buffer)) {
        emit navigationImageReceived(img);
        m_buffer.clear();
    }
}

void MapTcpClient::onDisconnected() { qDebug() << "Disconnected from server"; }
void MapTcpClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit errorOccurred(m_socket->errorString());
}
