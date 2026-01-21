#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    bool start(quint16 port);

signals:
    void logMessage(const QString &msg);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onNetworkReplyFinished();

private:
    QTcpServer *tcpServer;
    QTcpSocket *currentClient;
    QNetworkAccessManager *networkManager;
    QByteArray cityRequest;
};

#endif // SERVER_H
