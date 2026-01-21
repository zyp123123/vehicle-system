#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

    void requestWeather(const QString &ip, int port, const QString &cityCode);

signals:
    void weatherReceived(QString json);  // 收到 JSON
    void errorOccurred(QString err);     // 可选：异常信息

private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError);

private:
    QTcpSocket *socket;
};

#endif // CLIENT_H
