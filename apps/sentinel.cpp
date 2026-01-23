#include "sentinel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <fcntl.h>
#include <unistd.h>
#include <QThread>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include "tools/returnbutton.h"
#include "tools/appthreadpool.h"

Sentinel::Sentinel(QWidget *parent)
    : QWidget(parent)
{
    this->setFixedSize(800, 480);

    QVBoxLayout *main = new QVBoxLayout(this);

    // 顶部提示文本
    statusLabel = new QLabel("等待动作感应...", this);
    statusLabel->setStyleSheet("font-size:32px; color:white; background:black;");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setFixedHeight(120);
    main->addWidget(statusLabel);

    // 历史记录
    historyBox = new QTextEdit(this);
    historyBox->setReadOnly(true);
    historyBox->setStyleSheet("font-size:22px; background:#202020; color:#eeeeee;");
    main->addWidget(historyBox);

    // 查看相册按钮
    viewAlbumBtn = new QPushButton("查看相册", this);
    viewAlbumBtn->setStyleSheet(
        "QPushButton { font-size:26px; background:#444; color:white; padding:10px; border-radius:8px; }"
        "QPushButton:hover { background:#666; }"
    );
    connect(viewAlbumBtn, &QPushButton::clicked, this, &Sentinel::onViewAlbumClicked);
    main->addWidget(viewAlbumBtn);

    // 创建返回按钮
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    back->setStyleSheet(back->styleSheet() + "z-index: 9999;");
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(back, &ReturnButton::requestClose, this, [=](){
        emit requestClose();
    });

    // 打开 SR501 传感器
    sensorFd = ::open("/dev/sr501", O_RDONLY);
    if (sensorFd < 0) {
        statusLabel->setText("无法打开 /dev/sr501");
    }

    // 定时检测
    checkTimer = new QTimer(this);
    connect(checkTimer, &QTimer::timeout, this, &Sentinel::checkSensor);
    checkTimer->start(80);   // 80ms 一次
}

Sentinel::~Sentinel()
{
    if (sensorFd >= 0)
        ::close(sensorFd);
}

void Sentinel::checkSensor()
{
    if (sensorFd < 0) return;

    char val;
    if (::read(sensorFd, &val, 1) <= 0)
        return;

    if (locked) {
        if (lockTimer.elapsed() > 5000) {
            locked = false;
            statusLabel->setText("等待动作感应...");
        }
        return;
    }

    if (val == '1') {
        locked = true;
        lockTimer.restart();

        statusLabel->setText("⚠ 有人靠近！正在抓拍...");

        // 投递线程池任务
        QtConcurrent::run(
            AppThreadPool::instance(),
            [this]() {
                captureEvidence();
            }
        );

        historyBox->append(
            QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            + "  有人经过，开始抓拍"
        );
    }
}

void Sentinel::captureEvidence()
{
#ifdef Q_PROCESSOR_ARM
    QString device = "/dev/video2";
#else
    QString device = "/dev/video0";
#endif

    V4L2Capture camera;   // 局部对象

    if (!camera.openDevice(device.toStdString(), 640, 480)) {
        qDebug() << "[Sentinel] 摄像头打开失败";
        return;
    }

    cv::Mat frame;
    QImage resultImg;

    for (int i = 0; i < 15; ++i) {
        if (camera.readFrame(frame) && !frame.empty() && i >= 4) {
            cv::Mat rgb;
            cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
            resultImg = QImage(
                rgb.data, rgb.cols, rgb.rows, rgb.step,
                QImage::Format_RGB888
            ).copy();
            break;
        }
        QThread::msleep(50);
    }

    camera.closeDevice();

    if (resultImg.isNull()) {
        QMetaObject::invokeMethod(this, [=]() {
            historyBox->append(" -> 抓拍失败");
        });
        return;
    }

    QString dir = QCoreApplication::applicationDirPath() + "/myAlbum";
    QDir().mkpath(dir);
    QString file = dir + "/sentinel_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";

    bool ok = resultImg.save(file);

    QMetaObject::invokeMethod(this, [=]() {
        historyBox->append(ok ? " -> 抓拍成功" : " -> 保存失败");
    });
}

void Sentinel::onBackClicked()
{
    emit requestClose();
}

void Sentinel::onViewAlbumClicked()
{
    emit openAlbum();
}
