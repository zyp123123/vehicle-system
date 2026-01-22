#include "settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

Settings::Settings(QWidget *parent)
    : QWidget(parent)
{
    this->setFixedSize(800, 480);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("系统设置", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold;");
    layout->addWidget(title);

    // ============ 驱动控制区域 ============

    // ———— DHT11 ————
    QHBoxLayout *h1 = new QHBoxLayout;
    h1->addWidget(new QLabel("DHT11 温湿度传感器："));
    btnLoadDHT11 = new QPushButton("加载驱动");
    btnUnloadDHT11 = new QPushButton("卸载驱动");
    h1->addWidget(btnLoadDHT11);
    h1->addWidget(btnUnloadDHT11);
    layout->addLayout(h1);

    // ———— HCSR04 ————
    QHBoxLayout *h2 = new QHBoxLayout;
    h2->addWidget(new QLabel("HCSR04 超声波传感器："));
    btnLoadHCSR04 = new QPushButton("加载驱动");
    btnUnloadHCSR04 = new QPushButton("卸载驱动");
    h2->addWidget(btnLoadHCSR04);
    h2->addWidget(btnUnloadHCSR04);
    layout->addLayout(h2);

    // ———— SR501 ————
    QHBoxLayout *h3 = new QHBoxLayout;
    h3->addWidget(new QLabel("HCSR501 人体红外："));
    btnLoadSR501 = new QPushButton("加载驱动");
    btnUnloadSR501 = new QPushButton("卸载驱动");
    h3->addWidget(btnLoadSR501);
    h3->addWidget(btnUnloadSR501);
    layout->addLayout(h3);

    layout->addStretch();

    // ============ 退出程序 ============

    exitBtn = new QPushButton("⚠ 退出程序");
    exitBtn->setFixedHeight(90);
    exitBtn->setStyleSheet(
        "QPushButton { font-size: 28px; border-radius: 20px; background: #FF6666; color: white; }"
        "QPushButton:pressed { background: #CC3333; }"
    );
    layout->addWidget(exitBtn);

    // ============ 返回按钮 ============

    backBtn = new QPushButton(this);
    backBtn->setFixedSize(50, 50);
    backBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 25px;"
        "border-image: url(:/images/icons/back.png); }"
    );
    backBtn->move(10, 10);
    backBtn->raise();

    // 信号槽绑定
    connect(backBtn, &QPushButton::clicked, this, &Settings::onBackClicked);
    connect(exitBtn, &QPushButton::clicked, this, &Settings::onExitClicked);

    connect(btnLoadDHT11, &QPushButton::clicked, this, &Settings::loadDHT11);
    connect(btnUnloadDHT11, &QPushButton::clicked, this, &Settings::unloadDHT11);

    connect(btnLoadHCSR04, &QPushButton::clicked, this, &Settings::loadHCSR04);
    connect(btnUnloadHCSR04, &QPushButton::clicked, this, &Settings::unloadHCSR04);

    connect(btnLoadSR501, &QPushButton::clicked, this, &Settings::loadSR501);
    connect(btnUnloadSR501, &QPushButton::clicked, this, &Settings::unloadSR501);
}

bool Settings::runCommand(const QString &cmd, QString &output)
{
    QProcess process;
    process.start(cmd);
    process.waitForFinished();

    output = process.readAllStandardOutput() + process.readAllStandardError();
    return (process.exitCode() == 0);
}

void Settings::loadDHT11()
{
    QString out;
    runCommand("insmod /home/root/driver/dht11/dht11.ko", out);
    QMessageBox::information(this, "DHT11", out);
}

void Settings::unloadDHT11()
{
    QString out;
    runCommand("rmmod dht11", out);
    QMessageBox::information(this, "DHT11", out);
}

void Settings::loadHCSR04()
{
    QString out;
    runCommand("insmod /home/root/driver/hcsr04/hcsr04.ko", out);
    QMessageBox::information(this, "HCSR04", out);
}

void Settings::unloadHCSR04()
{
    QString out;
    runCommand("rmmod hcsr04", out);
    QMessageBox::information(this, "HCSR04", out);
}

void Settings::loadSR501()
{
    QString out;
    runCommand("insmod /home/root/driver/hcsr501/sr501.ko", out);
    QMessageBox::information(this, "SR501", out);
}

void Settings::unloadSR501()
{
    QString out;
    runCommand("rmmod sr501", out);
    QMessageBox::information(this, "SR501", out);
}

void Settings::onBackClicked()
{
    emit requestClose();
}

void Settings::onExitClicked()
{
    emit requestExitApp();
}
