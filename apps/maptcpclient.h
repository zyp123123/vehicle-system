#ifndef MAPTCPCLIENT_H
#define MAPTCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QImage>

class MapTcpClient : public QObject
{
    Q_OBJECT
public:
    explicit MapTcpClient(QObject *parent = nullptr);
    ~MapTcpClient();

    void connectToServer(const QString &host, quint16 port);
    void sendNavigationData(const QString &city, const QString &start, const QString &end);

signals:
    void navigationImageReceived(const QImage &image);
    void errorOccurred(const QString &msg);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer; // 用于接收图片数据
};

#endif // MAPTCPCLIENT_H
