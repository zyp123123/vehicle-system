#ifndef REMOTE_H
#define REMOTE_H

#include <QObject>
#include <QtMqtt/QMqttClient>

class Remote : public QObject
{
    Q_OBJECT
public:
    explicit Remote(QObject *parent = nullptr);
    ~Remote();

    /* ---------- 基本连接 ---------- */
    void connectToHost(const QString &host, quint16 port);
    void disconnectFromHost();

    /* ---------- 订阅 / 发布 ---------- */
    void subscribeTopic(const QString &topic, quint8 qos = 0);
    void unsubscribeTopic(const QString &topic);
    void publishMessage(const QString &topic,
                        const QString &message,
                        quint8 qos = 0,
                        bool retain = false);

    /* ---------- 控制处理 ---------- */
    void handleLedMessage(const QString &msg);
    void handleBuzzerMessage(const QString &msg);

signals:
    void mqttConnected();
    void mqttDisconnected();
    void mqttLog(const QString &text);
    void mqttMessageReceived(const QString &topic,
                             const QString &payload);

private:
    QMqttClient *m_client;

    /* ---------- 硬件控制 ---------- */
    void setLed(const QString &mode);     // "0" off, "1" on, "2" breath
    void setBuzzer(const QString &mode);  // "0" off, "1" on

    /* ---------- 传感器 ---------- */
    float readTemperature();
    bool readDht11(float &humidity, float &temperature);
    bool readHcsr04(double &meters);

    void publishTemperature();
    void publishDht11();
    void publishHcsr04();
};

#endif // REMOTE_H
