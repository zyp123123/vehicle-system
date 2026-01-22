#ifndef RETURNBUTTON_H
#define RETURNBUTTON_H

#include <QPushButton>
#include <QMouseEvent>

class ReturnButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ReturnButton(QWidget *parent = nullptr);

signals:
    void requestClose();   // 点击返回信号

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint dragPosition;   // 拖动时的位移参考
    QPoint pressPos;       // 记录按下的起始位置用于判断是点击还是拖动
};

#endif // RETURNBUTTON_H
