#include "settings.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCoreApplication>

Settings::Settings(QWidget *parent)
    : QWidget(parent)
{
    this->setFixedSize(800, 480);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(30);

    QLabel *title = new QLabel("系统设置", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold;");
    layout->addWidget(title);
    layout->addStretch();

    exitBtn = new QPushButton("⚠ 退出程序");
    exitBtn->setFixedHeight(90);
    exitBtn->setStyleSheet(
        "QPushButton { font-size: 30px; border-radius: 20px; background: #FF6666; color: white; }"
        "QPushButton:pressed { background: #CC3333; }"
    );
    layout->addWidget(exitBtn);
    layout->addStretch();

    // 🔙 左上角返回按钮
    backBtn = new QPushButton(this);
    backBtn->setFixedSize(50, 50);
    backBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 25px;"
        "border-image: url(:/images/icons/back.png); }"
        "QPushButton:pressed { opacity: 0.7; }"
    );
    backBtn->move(10, 10);
    backBtn->raise();

    connect(backBtn, &QPushButton::clicked, this, &Settings::onBackClicked);
    connect(exitBtn, &QPushButton::clicked, this, &Settings::onExitClicked);
}

void Settings::onBackClicked()
{
    emit requestClose();   // 让 MainWindow 返回
}

void Settings::onExitClicked()
{
    emit requestExitApp();  // 让 MainWindow 退出
}
