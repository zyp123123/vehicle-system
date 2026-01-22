#include "returnbutton.h"

ReturnButton::ReturnButton(QWidget *parent)
    : QPushButton(parent)
{
    setFixedSize(60, 60);
    setCursor(Qt::PointingHandCursor);

    // 半透明白色悬浮球样式
    setStyleSheet(
        "QPushButton {"
           "   background-color: rgba(255, 255, 255, 235);"   /* 接近不透明 */
           "   border-radius: 30px;"
           "   border: 2px solid rgba(180, 180, 180, 255);"   /* 关键：灰边 */
           "}"
           "QPushButton:pressed {"
           "   background-color: rgba(245, 245, 245, 255);"
           "}"
    );

    move(15, 15);  // 默认起始位置
    raise();       // 确保显示在最上层
}

void ReturnButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 计算按下位置相对于按钮左上角的偏移
        dragPosition = event->globalPos() - parentWidget()->mapToGlobal(pos());
        pressPos = event->globalPos();
    }
    QPushButton::mousePressEvent(event);
}

void ReturnButton::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        // 计算新位置
        QPoint newPos = parentWidget()->mapFromGlobal(event->globalPos() - dragPosition);

        // 限制移动范围，不超出父窗口
        int maxX = parentWidget()->width() - width();
        int maxY = parentWidget()->height() - height();

        if (newPos.x() < 0) newPos.setX(0);
        if (newPos.y() < 0) newPos.setY(0);
        if (newPos.x() > maxX) newPos.setX(maxX);
        if (newPos.y() > maxY) newPos.setY(maxY);

        move(newPos);
        raise(); // 移动时保持在最前
    }
    QPushButton::mouseMoveEvent(event);
}

void ReturnButton::mouseReleaseEvent(QMouseEvent *event)
{
    // 逻辑判断：如果移动距离曼哈顿长度小于 5 像素，视为点击而非拖动
    if ((event->globalPos() - pressPos).manhattanLength() < 5) {
        emit requestClose();
    }
    QPushButton::mouseReleaseEvent(event);
}
