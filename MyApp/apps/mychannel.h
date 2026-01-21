#ifndef MYCHANNEL_H
#define MYCHANNEL_H

#include <QObject>
#include <QJsonObject>

class myChannel : public QObject
{
    Q_OBJECT

public:
    explicit myChannel(QObject* parent = nullptr);

    // QT调用此函数 -> 发送信号给 HTML
    void setCity(QString city);
    void inputChanged_1(QString input);  // Start input
    void inputChanged_2(QString input);  // End input
    void startlocation(QString location);
    void endlocation(QString location);
    void selectRoute();

signals:
    // 发送给 HTML 的信号
    void cityChanged(QString city);
    void inputChanged_1_sig(QString input);  // JS端连接的信号名
    void inputChanged_2_sig(QString input);
    void startlocation_sig(QString location);
    void endlocation_sig(QString location);
    void selectRoute_sig();

    // 传递给 MapPage 的信号 (UI更新)
    void setCityLable(QString city);
    void sendAutocomplete_1(QJsonObject autocom);
    void sendAutocomplete_2(QJsonObject autocom);
    void routeError(QString message);

public slots:
    // HTML 调用的槽函数
    void cityChangeResult(QString result);
    void getAutocomplete_1(QJsonObject result);
    void getAutocomplete_2(QJsonObject result);
};

#endif // MYCHANNEL_H
