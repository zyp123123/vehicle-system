#ifndef WEATHER_H
#define WEATHER_H

#include <QWidget>
#include "server.h"
#include <QTextEdit>

class Weather : public QWidget
{
    Q_OBJECT
public:
    explicit Weather(QWidget *parent = nullptr);
    ~Weather();

private:
    Server *server;
    QTextEdit *logText;

private slots:
    void appendLog(const QString &msg);
};

#endif // WEATHER_H
