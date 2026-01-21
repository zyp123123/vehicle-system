#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include <QPushButton>

class Settings : public QWidget
{
    Q_OBJECT
public:
    explicit Settings(QWidget *parent = nullptr);

signals:
    void requestClose();     // 通知 MainWindow 返回上一级
    void requestExitApp();   // 通知 MainWindow 退出程序

private slots:
    void onBackClicked();
    void onExitClicked();

private:
    QPushButton *backBtn;
    QPushButton *exitBtn;
};

#endif // SETTINGS_H
