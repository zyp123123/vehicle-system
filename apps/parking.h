#ifndef PARKING_H
#define PARKING_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QThread>
#include <QFile>

#include "tools/v4l2capture.h"

/* ================= 超声波线程 ================= */

class HCSR04Worker : public QThread
{
    Q_OBJECT
public:
    explicit HCSR04Worker(QObject *parent = nullptr);
    ~HCSR04Worker();

    void stop();

signals:
    void distanceUpdated(double meters);

protected:
    void run() override;

private:
    bool running = true;
    int fd = -1;
};

/* ================= Parking 页面 ================= */

class Parking : public QWidget
{
    Q_OBJECT
public:
    explicit Parking(QWidget *parent = nullptr);
    ~Parking();

signals:
    void requestClose();

private slots:
    void updateCamera();
    void onDistance(double meters);
    void onBack();

private:
    /* UI */
    QLabel *cameraView;
    QLabel *distanceLabel;
    QLabel *warningLabel;

    /* Camera */
    QTimer *camTimer;
    V4L2Capture cameraDev;

    /* Ultrasonic */
    HCSR04Worker *worker;

    /* LED GPIO */
    QFile ledFile;
    bool ledState = false;

private:
    void setLed(bool on);
};

#endif // PARKING_H
