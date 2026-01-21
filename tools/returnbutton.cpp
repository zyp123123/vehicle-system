#include "returnbutton.h"

ReturnButton::ReturnButton(QWidget *parent)
    : QPushButton(parent)
{
    setFixedSize(48, 48);
    setIcon(QIcon(":/images/icons/back.png"));   // 你自己的返回图标
    setIconSize(QSize(32, 32));
    setFlat(true);

    setStyleSheet(
        "QPushButton { background: transparent; }"
        "QPushButton:pressed { background: rgba(0,0,0,30); }"
    );

    connect(this, &QPushButton::clicked,
            this, &ReturnButton::requestClose);
}
