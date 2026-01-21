#include "remote.h"

#include <QDebug>
#include <QTimer>

#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

/* ================= 构造 / 析构 ================= */

Remote::Remote(QObject *parent)
    : QObject(parent),
      m_client(new QMqttClient(this))
{
    /* ---------- MQTT 连接状态 ---------- */
    connect(m_client, &QMqttClient::connected, this, [=](){
        emit mqttLog(
            QString("Connected to %1:%2")
                .arg(m_client->hostname())
                .arg(m_client->port()));
        emit mqttConnected();

        /* 自动订阅控制主题 */
        subscribeTopic("zyp_mqtt/led");
        subscribeTopic("zyp_mqtt/buzzer");
    });

    connect(m_client, &QMqttClient::disconnected, this, [=](){
        emit mqttLog("Disconnected from broker");
        emit mqttDisconnected();
    });

    connect(m_client, &QMqttClient::messageReceived,
            this,
            [=](const QByteArray &message,
                const QMqttTopicName &topic)
    {
        QString t = topic.name();
        QString p = QString::fromUtf8(message);

        emit mqttLog(QString("[RECV] %1 : %2").arg(t, p));
        emit mqttMessageReceived(t, p);

        if (t == "zyp_mqtt/led")
            handleLedMessage(p);
        else if (t == "zyp_mqtt/buzzer")
            handleBuzzerMessage(p);
    });

    connect(m_client, &QMqttClient::errorChanged,
            this,
            [=](QMqttClient::ClientError e){
        emit mqttLog(QString("MQTT error: %1").arg(e));
    });

    /* ---------- 定时发布 ---------- */
    QTimer *tempTimer = new QTimer(this);
    connect(tempTimer, &QTimer::timeout,
            this, &Remote::publishTemperature);
    tempTimer->start(5000);

    QTimer *dhtTimer = new QTimer(this);
    connect(dhtTimer, &QTimer::timeout,
            this, &Remote::publishDht11);
    dhtTimer->start(5000);

    QTimer *hcsrTimer = new QTimer(this);
    connect(hcsrTimer, &QTimer::timeout,
            this, &Remote::publishHcsr04);
    hcsrTimer->start(3000);
}

Remote::~Remote()
{
    if (m_client->state() == QMqttClient::Connected)
        m_client->disconnectFromHost();
}

/* ================= MQTT 基本操作 ================= */

void Remote::connectToHost(const QString &host, quint16 port)
{
    m_client->setHostname(host);
    m_client->setPort(port);
    m_client->setUsername("mqtt1");
    m_client->setPassword("123456");

    emit mqttLog(QString("Connecting to %1:%2 ...").arg(host).arg(port));
    m_client->connectToHost();
}

void Remote::disconnectFromHost()
{
    if (m_client) {
        m_client->disconnectFromHost();
        emit mqttLog("Disconnecting MQTT...");
    }
}

void Remote::subscribeTopic(const QString &topic, quint8 qos)
{
    auto sub = m_client->subscribe(topic, qos);
    if (sub)
        emit mqttLog(QString("Subscribed: %1").arg(topic));
    else
        emit mqttLog(QString("Subscribe failed: %1").arg(topic));
}

void Remote::unsubscribeTopic(const QString &topic)
{
    m_client->unsubscribe(topic);
    emit mqttLog("Unsubscribed: " + topic);
}

void Remote::publishMessage(const QString &topic,
                            const QString &message,
                            quint8 qos,
                            bool retain)
{
    m_client->publish(topic, message.toUtf8(), qos, retain);
    emit mqttLog(QString("[PUB] %1 : %2").arg(topic, message));
}

/* ================= LED / 蜂鸣器 ================= */

void Remote::handleLedMessage(const QString &msg)
{
    setLed(msg);
}

void Remote::handleBuzzerMessage(const QString &msg)
{
    setBuzzer(msg);
}

void Remote::setLed(const QString &mode)
{
    if (mode == "0" || mode == "1") {
        int fdTrig = open("/sys/class/leds/sys-led/trigger", O_WRONLY);
        if (fdTrig >= 0) {
            write(fdTrig, "none", 4);
            close(fdTrig);
        }

        int fdBright = open("/sys/class/leds/sys-led/brightness", O_WRONLY);
        if (fdBright >= 0) {
            const char *v = (mode == "1") ? "255" : "0";
            write(fdBright, v, strlen(v));
            close(fdBright);
        }
    }
    else if (mode == "2") {
        int fdTrig = open("/sys/class/leds/sys-led/trigger", O_WRONLY);
        if (fdTrig >= 0) {
            write(fdTrig, "timer", 5);
            close(fdTrig);
        }

        int fdOn  = open("/sys/class/leds/sys-led/delay_on", O_WRONLY);
        int fdOff = open("/sys/class/leds/sys-led/delay_off", O_WRONLY);
        if (fdOn >= 0 && fdOff >= 0) {
            write(fdOn,  "500", 3);
            write(fdOff, "500", 3);
            close(fdOn);
            close(fdOff);
        }
    }
}

void Remote::setBuzzer(const QString &mode)
{
    int fd = open("/sys/class/leds/beep/brightness", O_WRONLY);
    if (fd < 0) return;

    write(fd, (mode == "1") ? "1" : "0", 1);
    close(fd);
}

/* ================= 传感器 ================= */

float Remote::readTemperature()
{
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return -1;

    int milli = 0;
    fscanf(fp, "%d", &milli);
    fclose(fp);
    return milli / 1000.0f;
}

void Remote::publishTemperature()
{
    float t = readTemperature();
    if (t < 0) return;

    publishMessage("zyp_mqtt/temperature",
                   QString::number(t, 'f', 1));
}

bool Remote::readDht11(float &humidity, float &temperature)
{
    FILE *fp = fopen("/sys/class/misc/dht11/value", "r");
    if (!fp) return false;

    fscanf(fp, "%f,%f", &humidity, &temperature);
    fclose(fp);
    return true;
}

void Remote::publishDht11()
{
    float h, t;
    if (!readDht11(h, t)) return;

    QString json =
        QString("{\"humidity\":%1,\"temperature\":%2}")
            .arg(h, 0, 'f', 1)
            .arg(t, 0, 'f', 1);

    publishMessage("zyp_mqtt/dht11", json);
}

bool Remote::readHcsr04(double &meters)
{
    int fd = open("/dev/hcsr04", O_RDWR);
    if (fd < 0) return false;

    uint32_t mm = 0;
    int ret = read(fd, &mm, sizeof(mm));
    close(fd);

    if (ret != sizeof(mm)) return false;

    meters = mm / 1000.0;
    return true;
}

void Remote::publishHcsr04()
{
    double m;
    if (!readHcsr04(m)) return;

    publishMessage("zyp_mqtt/hcsr04",
                   QString::number(m, 'f', 2));
}
