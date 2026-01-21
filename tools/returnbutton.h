#ifndef RETURNBUTTON_H
#define RETURNBUTTON_H

#include <QPushButton>

class ReturnButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ReturnButton(QWidget *parent = nullptr);

signals:
    void requestClose();
};

#endif // RETURNBUTTON_H
