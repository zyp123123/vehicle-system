#include "monitor.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QThread>

#include <QtConcurrent/QtConcurrent>

#include "tools/returnbutton.h"
#include "tools/appthreadpool.h"

Monitor::Monitor(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(800, 480);
    initUI();

    // 启动即打开本地预览
    startLocalPreview();
}

Monitor::~Monitor()
{
    onStopStream();
    stopLocalPreview();
}

void Monitor::initUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 左侧画面
    cameraView = new QLabel("初始化中...", this);
    cameraView->setFixedSize(650, 480);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setStyleSheet(
        "QLabel { background:black; color:white; font-size:24px; }"
    );

    // 右侧按钮
    QWidget *rightWidget = new QWidget;
    rightWidget->setFixedSize(150, 480);

    QVBoxLayout *btnLayout = new QVBoxLayout(rightWidget);

    btnStartStream = new QPushButton("开始推流");
    btnStopStream  = new QPushButton("停止推流");
    btnTakePhoto   = new QPushButton("拍照");
    btnAlbum       = new QPushButton("相册");

    QString btnStyle =
        "QPushButton { height:60px; font-size:16px; border-radius:10px;"
        "background:#E0E0E0; margin:5px; }"
        "QPushButton:pressed { background:#C0C0C0; }";

    btnStartStream->setStyleSheet(btnStyle);
    btnStopStream->setStyleSheet(btnStyle);
    btnTakePhoto->setStyleSheet(btnStyle);
    btnAlbum->setStyleSheet(btnStyle);

    btnStopStream->setEnabled(false);

    btnLayout->addStretch();
    btnLayout->addWidget(btnStartStream);
    btnLayout->addWidget(btnStopStream);
    btnLayout->addWidget(btnTakePhoto);
    btnLayout->addWidget(btnAlbum);
    btnLayout->addStretch();

    mainLayout->addWidget(cameraView);
    mainLayout->addWidget(rightWidget);

    // 返回按钮
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(back, &ReturnButton::requestClose, this, [=](){
        emit requestClose();
    });

    // 信号连接
    connect(btnStartStream, &QPushButton::clicked, this, &Monitor::onStartStream);
    connect(btnStopStream,  &QPushButton::clicked, this, &Monitor::onStopStream);
    connect(btnTakePhoto,   &QPushButton::clicked, this, &Monitor::onTakeBitmap);
    connect(btnAlbum,       &QPushButton::clicked, this, &Monitor::openAlbum);
}

void Monitor::startLocalPreview()
{
    if (frameTimer) return;

#ifdef Q_PROCESSOR_ARM
    QString device = "/dev/video2";
#else
    QString device = "/dev/video0";
#endif

    if (!v4l2Camera.openDevice(device.toStdString(), 640, 480)) {
        cameraView->setText("无法打开摄像头");
        return;
    }

    frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, this, &Monitor::updateFrame);
    frameTimer->start(30);
}

void Monitor::stopLocalPreview()
{
    if (frameTimer) {
        frameTimer->stop();
        delete frameTimer;
        frameTimer = nullptr;
    }

    v4l2Camera.closeDevice();
    cameraView->setText("摄像头待机中");
}

void Monitor::updateFrame()
{
    cv::Mat frame;
        if (!v4l2Camera.readFrame(frame))
            return;

        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

        QImage img(frame.data,
                   frame.cols,
                   frame.rows,
                   frame.step,
                   QImage::Format_RGB888);

        cameraView->setPixmap(QPixmap::fromImage(img));
        currentFrame = img.copy(); // 只有拍照才 copy
}

void Monitor::onTakeBitmap()
{
    if (currentFrame.isNull()) return;

    QString albumPath = QCoreApplication::applicationDirPath() + "/myAlbum";
    QDir().mkpath(albumPath);

    QString fileName = albumPath + "/" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";

    QImage img = currentFrame;

    QtConcurrent::run(AppThreadPool::instance(), [img, fileName]() {
        img.save(fileName);
        qDebug() << "拍照保存:" << fileName;
    });
}

void Monitor::onStartStream()
{
    if (isStreaming) return;

    previewEnabled = false;
    stopLocalPreview();

#ifdef Q_PROCESSOR_ARM
    QString device = "/dev/video2";
#else
    QString device = "/dev/video0";
#endif

    QString program = "/usr/bin/ffmpeg";
    QString rtmpUrl = "rtmp://192.168.50.128/live/stream";

    QStringList args {
        "-use_wallclock_as_timestamps","1",

        "-f","v4l2",
        "-input_format","mjpeg",
        "-video_size","640x360",
        "-framerate","30",
        "-i",device,

        "-vf","scale=320:240",
        "-r","10",

        "-c:v","libx264",
        "-preset","ultrafast",
        "-tune","zerolatency",
        "-x264-params","keyint=10:min-keyint=10:scenecut=0",
        "-pix_fmt","yuv420p",

        "-b:v","200k",
        "-bufsize","400k",

        "-f","flv",
        rtmpUrl
    };

    ffmpegProc = new QProcess(this);
    ffmpegProc->setProcessChannelMode(QProcess::MergedChannels);
    ffmpegProc->start(program, args);

    if (ffmpegProc->waitForStarted(3000)) {
        isStreaming = true;
        btnStartStream->setEnabled(false);
        btnStopStream->setEnabled(true);
        btnTakePhoto->setEnabled(false);
        cameraView->setText("正在推流...");
    } else {
        cameraView->setText("FFmpeg 启动失败");
        delete ffmpegProc;
        ffmpegProc = nullptr;
        startLocalPreview();
    }
}

void Monitor::onStopStream()
{
    if (!isStreaming) return;

    if (ffmpegProc) {
        ffmpegProc->terminate();
        if (!ffmpegProc->waitForFinished(3000)) {
            ffmpegProc->kill();
            ffmpegProc->waitForFinished(2000);
        }
        delete ffmpegProc;
        ffmpegProc = nullptr;
    }

    isStreaming = false;
    btnStartStream->setEnabled(true);
    btnStopStream->setEnabled(false);

    previewEnabled = true;    // ⭐ 恢复
    startLocalPreview();
    btnTakePhoto->setEnabled(frameTimer != nullptr);
}

void Monitor::onBackClicked()
{
    onStopStream();
    stopLocalPreview();
    emit requestClose();
}
