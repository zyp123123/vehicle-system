#include "weather.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QLabel>
#include <QFrame>

Weather::Weather(QWidget *parent)
    : QWidget(parent)
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // 卡片背景
    QFrame *card = new QFrame(this);
    card->setStyleSheet(
        "background:white;"
        "border-radius:15px;"
        "padding:20px;"
        "border: 1px solid #DDDDDD;"
    );
    QVBoxLayout *cardLayout = new QVBoxLayout(card);

    // 标题
    QLabel *title = new QLabel("天气服务器日志", card);
    title->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #333333;"
    );
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    // 日志框
    logText = new QTextEdit(this);
    logText->setReadOnly(true);
    logText->setStyleSheet(
        "font-size:16px;"
        "background:#F7F7F7;"
        "border:1px solid #CCCCCC;"
        "border-radius:8px;"
    );

    cardLayout->addWidget(logText);

    // 卡片加入主布局
    mainLayout->addWidget(card);

    // 创建 server
    server = new Server(this);

    connect(server, &Server::logMessage,
            this, &Weather::appendLog);

    // 启动服务器
    if (!server->start(8888)) {
        appendLog("服务器启动失败！");
    } else {
        appendLog("服务器启动成功，端口 8888");
    }
}

Weather::~Weather()
{
}

void Weather::appendLog(const QString &msg)
{
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logText->append(QString("[%1] %2").arg(time, msg));
}
