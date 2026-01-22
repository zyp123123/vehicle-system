#ifndef MAPTCPCLIENT_H
#define MAPTCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

class MapTcpClient : public QObject
{
    Q_OBJECT
public:
    explicit MapTcpClient(QObject *parent = nullptr);
    ~MapTcpClient();

    // 发起连接
    void connectToServer(const QString &host, quint16 port);

    // 发送导航 JSON 指令
    void sendNavigationData(const QString &city, const QString &start, const QString &end);

signals:
    void mapImageReceived(const QImage &img);                // 成功解析图片时发射
    void errorOccurred(const QString &msg);                 // 发生错误时发射
    void navigationCommandReceived(const QString &city,
                                   const QString &start,
                                   const QString &end);    // (预留)

private slots:
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_socket;
    QByteArray  m_buffer;       // 数据缓冲区
    bool        m_waitingHeader = true;   // 状态机标志：是否正在等待长度头
    quint32     m_expectedLength = 0;     // 预期接收的图片字节数
};

#endif // MAPTCPCLIENT_H
