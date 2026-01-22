#include "remotewidget.h"
#include "tools/returnbutton.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QTime>

RemoteWidget::RemoteWidget(QWidget *parent)
    : QWidget(parent)
{
    // 实例化 MQTT 逻辑处理类
    m_remote = new Remote(this);

    // 1. --- 连接区域布局 ---
    m_hostEdit = new QLineEdit("192.168.2.222");
    m_portEdit = new QLineEdit("1883");
    m_connectBtn = new QPushButton("连接");
    m_statusLabel = new QLabel("● Disconnected");
    m_statusLabel->setStyleSheet("color: red; font-weight: bold;");

    QHBoxLayout *connLayout = new QHBoxLayout();
    connLayout->addWidget(new QLabel("服务器:"));
    connLayout->addWidget(m_hostEdit);
    connLayout->addWidget(new QLabel("端口:"));
    connLayout->addWidget(m_portEdit, 0); // 端口占位设为0
    connLayout->addWidget(m_connectBtn);
    connLayout->addWidget(m_statusLabel);


    // 2. --- 订阅区域布局 ---
    QGroupBox *subGroup = new QGroupBox("订阅管理");
    QVBoxLayout *subGroupLayout = new QVBoxLayout(subGroup);

    m_subTopicEdit = new QLineEdit("zyp_mqtt/temperature");
    m_subQosBox = new QComboBox();
    m_subQosBox->addItems({"0", "1", "2"});
    m_subBtn = new QPushButton("订阅主题");
    m_unsubBtn = new QPushButton("取消订阅");

    m_subList = new QListWidget();
    m_subList->setFixedHeight(80); // 限制列表高度

    QHBoxLayout *subInputLayout = new QHBoxLayout();
    subInputLayout->addWidget(new QLabel("主题:"));
    subInputLayout->addWidget(m_subTopicEdit);
    subInputLayout->addWidget(new QLabel("QoS:"));
    subInputLayout->addWidget(m_subQosBox);
    subInputLayout->addWidget(m_subBtn);
    subInputLayout->addWidget(m_unsubBtn);

    subGroupLayout->addLayout(subInputLayout);
    subGroupLayout->addWidget(m_subList);


    // 3. --- 发布区域布局 ---
    QGroupBox *pubGroup = new QGroupBox("发布消息");
    QHBoxLayout *pubLayout = new QHBoxLayout(pubGroup);

    m_pubTopicEdit = new QLineEdit("zyp_mqtt/led");
    m_pubMsgEdit = new QLineEdit("1");
    m_pubQosBox = new QComboBox();
    m_pubQosBox->addItems({"0", "1", "2"});
    m_pubBtn = new QPushButton("立即发布");

    pubLayout->addWidget(new QLabel("主题:"));
    pubLayout->addWidget(m_pubTopicEdit);
    pubLayout->addWidget(new QLabel("消息:"));
    pubLayout->addWidget(m_pubMsgEdit);
    pubLayout->addWidget(new QLabel("QoS:"));
    pubLayout->addWidget(m_pubQosBox);
    pubLayout->addWidget(m_pubBtn);


    // 4. --- 日志区域 ---
    m_logEdit = new QTextEdit();
    m_logEdit->setReadOnly(true);
    m_logEdit->setStyleSheet("background-color: #f0f0f0; font-family: 'Consolas';");


    // 5. --- 主布局构建 ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(connLayout);
    mainLayout->addWidget(subGroup);
    mainLayout->addWidget(pubGroup);
    mainLayout->addWidget(new QLabel("运行日志:"));
    mainLayout->addWidget(m_logEdit);


    // 6. --- 信号槽连接 ---

    // UI -> Remote 逻辑
    connect(m_connectBtn, &QPushButton::clicked, this, &RemoteWidget::onConnectClicked);
    connect(m_subBtn, &QPushButton::clicked, this, &RemoteWidget::onSubscribeClicked);
    connect(m_pubBtn, &QPushButton::clicked, this, &RemoteWidget::onPublishClicked);
    connect(m_unsubBtn, &QPushButton::clicked, this, &RemoteWidget::onUnsubscribeClicked);
    connect(m_subList, &QListWidget::itemClicked, this, &RemoteWidget::onSubItemClicked);

    // Remote 逻辑 -> UI 反馈
    connect(m_remote, &Remote::mqttConnected, this, &RemoteWidget::onMqttConnected);
    connect(m_remote, &Remote::mqttDisconnected, this, &RemoteWidget::onMqttDisconnected);
    connect(m_remote, &Remote::mqttLog, this, &RemoteWidget::appendLog);
    connect(m_remote, &Remote::mqttMessageReceived, this, &RemoteWidget::onMqttMessage);


    // 7. --- 添加返回悬浮球 ---
    ReturnButton *backBtn = new ReturnButton(this);
    // 关键：确保悬浮球在布局之上且不被遮挡
    backBtn->raise();
    backBtn->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    connect(backBtn, &ReturnButton::requestClose, this, [=](){
        emit requestClose();
    });

    appendLog("系统就绪，等待连接服务器...");
}

// 槽函数：点击连接/断开按钮
void RemoteWidget::onConnectClicked()
{
    if (m_connectBtn->text() == "连接") {
        QString host = m_hostEdit->text().trimmed();
        quint16 port = m_portEdit->text().toUShort();

        if(host.isEmpty()) return;

        appendLog(QString("尝试连接至 %1:%2...").arg(host).arg(port));
        m_remote->connectToHost(host, port);
        m_connectBtn->setText("断开");
    } else {
        m_remote->disconnectFromHost();
        m_connectBtn->setText("连接");
    }
}

// 槽函数：点击订阅
void RemoteWidget::onSubscribeClicked()
{
    QString topic = m_subTopicEdit->text().trimmed();
    int qos = m_subQosBox->currentText().toInt();

    if(topic.isEmpty()) return;

    m_remote->subscribeTopic(topic, qos);
    m_subList->addItem(QString("%1 (QoS %2)").arg(topic).arg(qos));
    m_subList->scrollToBottom();
}

// 槽函数：点击发布
void RemoteWidget::onPublishClicked()
{
    QString topic = m_pubTopicEdit->text().trimmed();
    QString msg = m_pubMsgEdit->text();
    int qos = m_pubQosBox->currentText().toInt();

    if(topic.isEmpty()) return;

    m_remote->publishMessage(topic, msg, qos);
}

// 槽函数：取消订阅
void RemoteWidget::onUnsubscribeClicked()
{
    QListWidgetItem *item = m_subList->currentItem();
    if (!item) {
        appendLog("请先在列表中选择要取消订阅的主题");
        return;
    }

    QString text = item->text();
    int pos = text.indexOf("(QoS");
    if (pos > 0) {
        QString topic = text.left(pos).trimmed();
        m_remote->unsubscribeTopic(topic);
        delete m_subList->takeItem(m_subList->row(item));
    }
}

// 槽函数：点击列表项同步到输入框
void RemoteWidget::onSubItemClicked()
{
    QListWidgetItem *item = m_subList->currentItem();
    if (!item) return;

    QString text = item->text();
    int pos = text.indexOf("(QoS");
    if (pos > 0) {
        m_subTopicEdit->setText(text.left(pos).trimmed());
        QString qos = text.mid(pos + 4).remove(")").trimmed();
        m_subQosBox->setCurrentText(qos);
    }
}

// MQTT 连接成功的 UI 处理
void RemoteWidget::onMqttConnected()
{
    m_statusLabel->setText("● Connected");
    m_statusLabel->setStyleSheet("color: #2ecc71; font-weight: bold;"); // 绿色
    appendLog("状态：服务器已连接");
}

// MQTT 断开连接的 UI 处理
void RemoteWidget::onMqttDisconnected()
{
    m_statusLabel->setText("● Disconnected");
    m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;"); // 红色
    m_connectBtn->setText("连接");
    appendLog("状态：连接已断开");
}

// 收到 MQTT 消息
void RemoteWidget::onMqttMessage(const QString &topic, const QString &payload)
{
    // 这里只需记录日志，具体的硬件控制逻辑已经在 Remote 类中实现了
    // appendLog 已由 Remote::mqttLog 信号触发，此处可做额外 UI 更新
}

// 追加日志
void RemoteWidget::appendLog(const QString &msg)
{
    m_logEdit->append(QString("[%1] ► %2")
                      .arg(QTime::currentTime().toString("hh:mm:ss"))
                      .arg(msg));
}
