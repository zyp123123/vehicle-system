#ifndef SENTINEL_H
#define SENTINEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QElapsedTimer>
#include "tools/v4l2capture.h"

class Sentinel : public QWidget
{
    Q_OBJECT

public:
    explicit Sentinel(QWidget *parent = nullptr);
    ~Sentinel();

signals:
    void requestClose(); // 返回主界面
    void openAlbum();    // 跳转相册

private slots:
    void checkSensor();       // 定时检测 /dev/sr501
    void onBackClicked();
    void onViewAlbumClicked();

private:
    int sensorFd;
    QTimer *checkTimer;

    QLabel *statusLabel;      // 显示“有人靠近！”固定 3 秒
    QTextEdit *historyBox;    // 历史记录
    QPushButton *viewAlbumBtn;

    QElapsedTimer lockTimer;  // 锁定计时
    bool locked = false;

    // 摄像头对象和拍照函数
    V4L2Capture camera;
    void captureEvidence();
};

#endif // SENTINEL_H
