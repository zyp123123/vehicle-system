#include "maptcpclient.h"
#include <QDebug>

MapTcpClient::MapTcpClient(QObject *parent)
    : QObject(parent),
      m_socket(new QTcpSocket(this))
{
    // 信号槽连接
    connect(m_socket, &QTcpSocket::readyRead, this, &MapTcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MapTcpClient::onDisconnected);

    // 注意：Qt 5.15+ 建议使用新版连接方式，这里保留您的兼容性写法
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onErrorOccurred(QAbstractSocket::SocketError)));
}

MapTcpClient::~MapTcpClient()
{
    if (m_socket->isOpen()) {
        m_socket->close();
    }
}

void MapTcpClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void MapTcpClient::sendNavigationData(const QString &city, const QString &start, const QString &end)
{
    if (!m_socket->isOpen() || m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Socket 未连接，无法发送数据");
        return;
    }

    // 构建指令 JSON
    QJsonObject obj;
    obj["city"] = city;
    obj["start"] = start;
    obj["end"] = end;
    QJsonDocument doc(obj);

    m_socket->write(doc.toJson(QJsonDocument::Compact));
    m_socket->flush();
}

void MapTcpClient::onReadyRead()
{
    // 将新到达的数据追加到缓冲区
    m_buffer.append(m_socket->readAll());

    while (true)
    {
        // 步骤 1：解析 4 字节大端序长度头
        if (m_waitingHeader) {
            if (m_buffer.size() < 4)
                return; // 数据不足 4 字节，继续等待

            m_expectedLength =
                ((quint8)m_buffer[0] << 24) |
                ((quint8)m_buffer[1] << 16) |
                ((quint8)m_buffer[2] << 8)  |
                (quint8)m_buffer[3];

            // 从缓冲区移除已读取的头
            m_buffer.remove(0, 4);
            m_waitingHeader = false;

            qDebug() << "解析到图片长度头：" << m_expectedLength;
        }

        // 步骤 2：根据长度解析图片主体数据
        if (!m_waitingHeader) {
            if ((quint32)m_buffer.size() < m_expectedLength)
                return; // 图片数据尚未接收完整，继续等待

            // 提取图片字节并从缓冲区移除
            QByteArray imgBytes = m_buffer.left(m_expectedLength);
            m_buffer.remove(0, m_expectedLength);

            // 重置状态机，准备接收下一帧（如果有）
            m_waitingHeader = true;

            qDebug() << "图片数据接收完整，字节数：" << imgBytes.size();

            // 尝试加载图片
            QImage img;
            if (!img.loadFromData(imgBytes, "PNG")) {
                qDebug() << "图片解析失败（格式不正确）";
            } else {
                qDebug() << "图片解析成功！发射信号...";
                emit mapImageReceived(img);
            }
        }
    }
}

void MapTcpClient::onDisconnected()
{
    qDebug() << "与服务器断开连接";
    // 断开后重置缓冲区和状态机
    m_buffer.clear();
    m_waitingHeader = true;
}

void MapTcpClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit errorOccurred(m_socket->errorString());
}
