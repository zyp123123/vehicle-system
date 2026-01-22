#include "settings.h"
#include "tools/returnbutton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QApplication>

/* ================= 工具 ================= */

static void updateStatus(QLabel *label, bool loaded)
{
    label->setText(QString("驱动状态：%1").arg(loaded ? "已加载" : "未加载"));
    label->setStyleSheet(QString(
        "font-size:16px;"
        "font-weight:bold;"
        "color:%1;"
    ).arg(loaded ? "#2E7D32" : "#C62828"));
}

static QString driverBtnStyle()
{
    return
        "QPushButton {"
        "  font-size:16px;"
        "  height:38px;"
        "  border-radius:14px;"
        "  background:#F5F5F5;"
        "  border:1px solid rgba(0,0,0,40);"
        "}"
        "QPushButton:pressed {"
        "  background:#E0E0E0;"
        "}";
}

/* ================= ctor ================= */

Settings::Settings(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(800, 480);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background:#FAFAFA;");

    /* ===== 主布局 ===== */
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    /* ===== 标题 ===== */
    QLabel *title = new QLabel("系统设置");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:36px;font-weight:bold;");
    mainLayout->addWidget(title);

    /* ===== 驱动卡片 ===== */
    QHBoxLayout *cardsLayout = new QHBoxLayout;
    cardsLayout->setSpacing(24);

    auto createCard = [&](const QString &name, QLabel *&statusLabel,
                          QPushButton *&btnLoad, QPushButton *&btnUnload) {

        QFrame *card = new QFrame;
        card->setFixedSize(220, 260);
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setStyleSheet(
            "QFrame {"
            " background:#FFFFFF;"
            " border-radius:24px;"
            " border:1px solid rgba(0,0,0,35);"
            "}"
        );

        QLabel *t = new QLabel(name);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("font-size:20px;font-weight:bold;");

        statusLabel = new QLabel;
        statusLabel->setAlignment(Qt::AlignCenter);

        btnLoad = new QPushButton("加载驱动");
        btnUnload = new QPushButton("卸载驱动");
        btnLoad->setStyleSheet(driverBtnStyle());
        btnUnload->setStyleSheet(driverBtnStyle());

        QVBoxLayout *v = new QVBoxLayout(card);
        v->setContentsMargins(18, 18, 18, 18);
        v->setSpacing(8);
        v->addWidget(t);
        v->addWidget(statusLabel);
        v->addWidget(btnLoad);
        v->addWidget(btnUnload);

        cardsLayout->addWidget(card);
    };

    QPushButton *btnLoadDHT11, *btnUnloadDHT11;
    QPushButton *btnLoadHCSR04, *btnUnloadHCSR04;
    QPushButton *btnLoadSR501, *btnUnloadSR501;

    createCard("DHT11 温湿度", dht11Status, btnLoadDHT11, btnUnloadDHT11);
    createCard("HCSR04 超声波", hcsr04Status, btnLoadHCSR04, btnUnloadHCSR04);
    createCard("SR501 人体红外", sr501Status, btnLoadSR501, btnUnloadSR501);

    mainLayout->addLayout(cardsLayout);
    mainLayout->addStretch();

    /* ===== 退出程序 ===== */
    exitBtn = new QPushButton("⚠ 结束程序");
    exitBtn->setFixedHeight(80);
    exitBtn->setStyleSheet(
        "QPushButton {"
        " font-size:26px;"
        " border-radius:22px;"
        " background:#FF6666;"
        " color:white;"
        "}"
        "QPushButton:pressed { background:#CC3333; }"
    );
    mainLayout->addWidget(exitBtn);

    /* ===== 信号 ===== */
    connect(exitBtn, &QPushButton::clicked,
            this, &Settings::onExitClicked);

    connect(btnLoadDHT11, &QPushButton::clicked, this, &Settings::loadDHT11);
    connect(btnUnloadDHT11, &QPushButton::clicked, this, &Settings::unloadDHT11);

    connect(btnLoadHCSR04, &QPushButton::clicked, this, &Settings::loadHCSR04);
    connect(btnUnloadHCSR04, &QPushButton::clicked, this, &Settings::unloadHCSR04);

    connect(btnLoadSR501, &QPushButton::clicked, this, &Settings::loadSR501);
    connect(btnUnloadSR501, &QPushButton::clicked, this, &Settings::unloadSR501);

    /* ===== 初始化状态（只检测）===== */
    updateStatus(dht11Status,  isModuleLoaded("dht11"));
    updateStatus(hcsr04Status, isModuleLoaded("hcsr04"));
    updateStatus(sr501Status,  isModuleLoaded("sr501"));

    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose,
            this, &Settings::requestClose);
}

/* ================= 命令 ================= */

bool Settings::runCommand(const QString &cmd, QString &output)
{
    QProcess p;
    p.start("sh", {"-c", cmd});
    p.waitForFinished();
    output = p.readAllStandardOutput() + p.readAllStandardError();
    return p.exitCode() == 0;
}

bool Settings::isModuleLoaded(const QString &module)
{
    QProcess p;
    p.start("sh", {"-c", "lsmod | grep " + module});
    p.waitForFinished();
    return !p.readAllStandardOutput().isEmpty();
}

/* ================= 驱动 ================= */

void Settings::loadDHT11()
{
    QString out;
    runCommand("insmod /home/root/driver/dht11/dht11.ko", out);
    updateStatus(dht11Status, isModuleLoaded("dht11"));
}

void Settings::unloadDHT11()
{
    QString out;
    runCommand("rmmod dht11", out);
    updateStatus(dht11Status, isModuleLoaded("dht11"));
}

void Settings::loadHCSR04()
{
    QString out;
    runCommand("insmod /home/root/driver/hcsr04/hcsr04.ko", out);
    updateStatus(hcsr04Status, isModuleLoaded("hcsr04"));
}

void Settings::unloadHCSR04()
{
    QString out;
    runCommand("rmmod hcsr04", out);
    updateStatus(hcsr04Status, isModuleLoaded("hcsr04"));
}

void Settings::loadSR501()
{
    QString out;
    runCommand("insmod /home/root/driver/hcsr501/sr501.ko", out);
    updateStatus(sr501Status, isModuleLoaded("sr501"));
}

void Settings::unloadSR501()
{
    QString out;
    runCommand("rmmod sr501", out);
    updateStatus(sr501Status, isModuleLoaded("sr501"));
}

void Settings::onExitClicked()
{
    emit requestExitApp();
    qApp->quit();   // ★真正退出程序
}
