#include "sentinel.h"
#include "tools/returnbutton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#include <fcntl.h>
#include <unistd.h>

/* ================= ctor / dtor ================= */

Sentinel::Sentinel(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(800, 480);
    setStyleSheet("background:#111; color:white;");

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(20, 20, 20, 20);
    main->setSpacing(15);

    /* ---------- 标题 ---------- */
    QLabel *title = new QLabel("安防哨兵");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:34px; font-weight:bold;");
    main->addWidget(title);

    /* ---------- 状态 ---------- */
    statusLabel = new QLabel("状态：安全");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "font-size:36px;"
        "background:#222;"
        "border-radius:12px;"
        "padding:20px;"
    );
    main->addWidget(statusLabel);

    /* ---------- 历史 ---------- */
    historyBox = new QTextEdit;
    historyBox->setReadOnly(true);
    historyBox->setStyleSheet(
        "background:#000;"
        "border-radius:10px;"
        "font-size:18px;"
    );
    main->addWidget(historyBox, 1);

    /* ---------- 按钮 ---------- */
    QHBoxLayout *btns = new QHBoxLayout;
    viewAlbumBtn = new QPushButton("查看抓拍记录");
    viewAlbumBtn->setFixedHeight(60);
    viewAlbumBtn->setStyleSheet(
        "font-size:22px;"
        "background:#3b82f6;"
        "border-radius:12px;"
    );
    btns->addStretch();
    btns->addWidget(viewAlbumBtn);
    btns->addStretch();
    main->addLayout(btns);

    connect(viewAlbumBtn, &QPushButton::clicked,
            this, &Sentinel::onViewAlbumClicked);

    /* ---------- 返回按钮 ---------- */
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose,
            this, &Sentinel::onBackClicked);

    /* ---------- 打开 PIR ---------- */
    sensorFd = ::open("/dev/sr501", O_RDONLY | O_NONBLOCK);
    if (sensorFd < 0)
        qWarning() << "无法打开 /dev/sr501";

    /* ---------- 打开摄像头 ---------- */
#ifdef Q_PROCESSOR_ARM
    camera.openDevice("/dev/video2", 640, 480);
#else
    camera.openDevice("/dev/video0", 640, 480);
#endif

    /* ---------- 定时检测 ---------- */
    checkTimer = new QTimer(this);
    connect(checkTimer, &QTimer::timeout,
            this, &Sentinel::checkSensor);
    checkTimer->start(200);
}

Sentinel::~Sentinel()
{
    if (checkTimer)
        checkTimer->stop();

    if (sensorFd >= 0)
        ::close(sensorFd);

    camera.closeDevice();
}

/* ================= PIR 检测 ================= */

void Sentinel::checkSensor()
{
    if (sensorFd < 0)
        return;

    char buf;
    int ret = ::read(sensorFd, &buf, 1);
    if (ret <= 0)
        return;

    /* 防抖：3 秒内只触发一次 */
    if (locked && lockTimer.elapsed() < 3000)
        return;

    locked = true;
    lockTimer.restart();

    QString now = QDateTime::currentDateTime()
                      .toString("yyyy-MM-dd HH:mm:ss");

    statusLabel->setText("⚠ 有人靠近！");
    statusLabel->setStyleSheet(
        "font-size:36px;"
        "background:#b91c1c;"
        "border-radius:12px;"
        "padding:20px;"
    );

    historyBox->append("[" + now + "] 检测到人体活动");

    captureEvidence();

    /* 3 秒后恢复 */
    QTimer::singleShot(3000, this, [=](){
        statusLabel->setText("状态：安全");
        statusLabel->setStyleSheet(
            "font-size:36px;"
            "background:#222;"
            "border-radius:12px;"
            "padding:20px;"
        );
        locked = false;
    });
}

/* ================= 拍照存证 ================= */

void Sentinel::captureEvidence()
{
    cv::Mat frame;
    if (!camera.readFrame(frame))
        return;

    QString dir = QCoreApplication::applicationDirPath() + "/myAlbum";
    QDir().mkpath(dir);

    QString file =
        dir + "/sentinel_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
        ".jpg";

    cv::imwrite(file.toStdString(), frame);

    historyBox->append("  └─ 已抓拍：" + QFileInfo(file).fileName());
}

/* ================= UI ================= */

void Sentinel::onViewAlbumClicked()
{
    emit openAlbum();
}

void Sentinel::onBackClicked()
{
    emit requestClose();
}
