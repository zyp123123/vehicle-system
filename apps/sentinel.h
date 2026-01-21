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
    void requestClose();     // 返回主界面
    void openAlbum();        // 跳转相册

private slots:
    void checkSensor();          // 定时检测 /dev/sr501
    void onBackClicked();
    void onViewAlbumClicked();

private:
    /* PIR 传感器 */
    int sensorFd = -1;
    QTimer *checkTimer;

    /* UI */
    QLabel *statusLabel;     // “有人靠近！”
    QTextEdit *historyBox;   // 历史记录
    QPushButton *viewAlbumBtn;

    /* 锁定逻辑 */
    QElapsedTimer lockTimer;
    bool locked = false;

    /* 摄像头 */
    V4L2Capture camera;
    void captureEvidence();  // 拍照存证
};

#endif // SENTINEL_H
