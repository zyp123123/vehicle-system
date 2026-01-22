#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

    /**
     * @brief 请求天气数据
     * @param ip 服务器IP
     * @param port 端口
     * @param cityCode 城市 adcode (如 110000)
     */
    void requestWeather(const QString &ip, int port, const QString &cityCode);

signals:
    void weatherReceived(QString json);  // 成功收到并转换后的 JSON 字符串
    void errorOccurred(QString err);     // 错误处理信号

private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);

private:
    QTcpSocket *socket;
};

#endif // CLIENT_H
