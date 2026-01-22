#ifndef REMOTEWIDGET_H
#define REMOTEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>

#include "remote.h"

class RemoteWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RemoteWidget(QWidget *parent = nullptr);

signals:
    void requestClose();   // 发射给父窗口，用于切换回主界面

private slots:
    // UI 按钮触发的槽
    void onConnectClicked();
    void onSubscribeClicked();
    void onPublishClicked();
    void onUnsubscribeClicked();
    void onSubItemClicked();

    // 接收来自 Remote 逻辑层的信号
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttMessage(const QString &topic, const QString &payload);
    void appendLog(const QString &msg);

private:
    // --- 连接区域 ---
    QLineEdit   *m_hostEdit;
    QLineEdit   *m_portEdit;
    QPushButton *m_connectBtn;
    QLabel      *m_statusLabel;

    // --- 订阅区域 ---
    QLineEdit   *m_subTopicEdit;
    QComboBox   *m_subQosBox;
    QPushButton *m_subBtn;
    QListWidget *m_subList;
    QPushButton *m_unsubBtn;

    // --- 发布区域 ---
    QLineEdit   *m_pubTopicEdit;
    QLineEdit   *m_pubMsgEdit;
    QComboBox   *m_pubQosBox;
    QPushButton *m_pubBtn;

    // --- 日志区域 ---
    QTextEdit   *m_logEdit;

    // --- 逻辑处理 ---
    Remote      *m_remote;
};

#endif // REMOTEWIDGET_H
