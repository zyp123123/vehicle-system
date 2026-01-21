#ifndef MONITOR_H
#define MONITOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QImage>
#include <QProcess>
#include <atomic>

#include "tools/v4l2capture.h"

class Monitor : public QWidget
{
    Q_OBJECT
public:
    explicit Monitor(QWidget *parent = nullptr);
    ~Monitor();

signals:
    void requestClose();   // 返回主页
    void openAlbum();      // 打开相册

private slots:
    void updateFrame();
    void onTakeBitmap();
    void onStartStream();
    void onStopStream();
    void onBackClicked();

private:
    void initUI();
    void startLocalPreview();
    void stopLocalPreview();

private:
    QLabel *cameraView = nullptr;

    QPushButton *btnStartStream = nullptr;
    QPushButton *btnStopStream = nullptr;
    QPushButton *btnTakePhoto   = nullptr;
    QPushButton *btnAlbum       = nullptr;

    // V4L2 + 本地预览
    V4L2Capture v4l2Camera;
    QTimer *frameTimer = nullptr;
    QImage currentFrame;

    // FFmpeg 推流
    QProcess *ffmpegProc = nullptr;
    bool isStreaming = false;

    // ⭐ 防止 updateFrame 线程池任务堆积
    std::atomic_bool captureBusy{false};
};

#endif // MONITOR_H
