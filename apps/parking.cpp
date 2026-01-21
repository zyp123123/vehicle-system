#include "parking.h"

#include <QVBoxLayout>
#include <QDebug>
#include <QCoreApplication>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <fcntl.h>
#include <unistd.h>

#include "tools/returnbutton.h"

/* ================= HCSR04 Worker ================= */

HCSR04Worker::HCSR04Worker(QObject *parent)
    : QThread(parent)
{
    fd = ::open("/dev/hcsr04", O_RDWR);
    if (fd < 0)
        qWarning() << "Failed to open /dev/hcsr04";
}

HCSR04Worker::~HCSR04Worker()
{
    running = false;
    wait();

    if (fd >= 0)
        ::close(fd);
}

void HCSR04Worker::stop()
{
    running = false;
}

void HCSR04Worker::run()
{
    while (running) {
        if (fd < 0) {
            msleep(200);
            continue;
        }

        uint32_t mm = 0;
        int ret = ::read(fd, &mm, sizeof(mm));
        if (ret == sizeof(mm)) {
            double meters = mm / 1000.0;
            emit distanceUpdated(meters);
        }
        msleep(100);
    }
}

/* ================= Parking ================= */

Parking::Parking(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(800, 480);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    cameraView = new QLabel("摄像头加载中...");
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setStyleSheet("background:black;color:white;font-size:26px;");
    cameraView->setFixedHeight(360);

    distanceLabel = new QLabel("距离: --");
    distanceLabel->setAlignment(Qt::AlignCenter);
    distanceLabel->setStyleSheet("font-size:32px; color:white; background:#444;");

    warningLabel = new QLabel;
    warningLabel->setAlignment(Qt::AlignCenter);
    warningLabel->setStyleSheet("font-size:40px; color:red; font-weight:bold;");

    mainLayout->addWidget(cameraView);
    mainLayout->addWidget(distanceLabel);
    mainLayout->addWidget(warningLabel);

    /* 返回按钮 */
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose,
            this, &Parking::requestClose);

    /* 摄像头设备 */
#ifdef Q_PROCESSOR_ARM
    QString dev = "/dev/video2";
#else
    QString dev = "/dev/video0";
#endif
    cameraDev.openDevice(dev.toStdString(), 640, 480);

    camTimer = new QTimer(this);
    connect(camTimer, &QTimer::timeout,
            this, &Parking::updateCamera);
    camTimer->start(33); // ~30fps

    /* 启动超声波线程 */
    worker = new HCSR04Worker(this);
    connect(worker, &HCSR04Worker::distanceUpdated,
            this, &Parking::onDistance);
    worker->start();

    /* LED 初始化 */
    ledFile.setFileName("/sys/class/leds/sys-led/brightness");
    setLed(false);
}

Parking::~Parking()
{
    camTimer->stop();
    cameraDev.closeDevice();

    if (worker) {
        worker->stop();
        worker->wait();
    }

    setLed(false);
}

/* ================= Camera ================= */

void Parking::updateCamera()
{
    cv::Mat frame;
    if (cameraDev.readFrame(frame)) {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        QImage img(frame.data,
                   frame.cols,
                   frame.rows,
                   frame.step,
                   QImage::Format_RGB888);
        cameraView->setPixmap(QPixmap::fromImage(img));
    }
}

/* ================= Distance ================= */

void Parking::onDistance(double meters)
{
    distanceLabel->setText(
        QString("距离: %1 m").arg(meters, 0, 'f', 2));

    if (meters < 1.0) {
        warningLabel->setText("⚠ 过近！");
        setLed(true);
    } else {
        warningLabel->clear();
        setLed(false);
    }
}

/* ================= LED ================= */

void Parking::setLed(bool on)
{
    if (ledState == on)
        return;

    if (ledFile.exists()) {
        if (ledFile.open(QIODevice::WriteOnly)) {
            ledFile.write(on ? "1" : "0");
            ledFile.close();
        }
    }
    ledState = on;
}

/* ================= Back ================= */

void Parking::onBack()
{
    setLed(false);
    emit requestClose();
}
