#ifndef WEATHER_H
#define WEATHER_H

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMap>
#include <QMovie>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QCompleter>

#include "weatherclient.h"

class Weather : public QWidget
{
    Q_OBJECT
public:
    explicit Weather(QWidget *parent = nullptr);
    ~Weather() override = default;

signals:
    void requestClose();

private slots:
    void onRequestWeather();
    void onWeatherJson(QString json);
    void onError(QString errMsg);

private:
    /* UI */
    void setupUi();
    void setupStyle();
    void loadCityList();

    /* helpers */
    QString cityCode(const QString &cityName);
    QString iconForWeather(const QString &weatherText);
    void showLoading(bool on);
    void animateIcon();

private:
    QLabel *tempLabel;
    QLabel *humiLabel;
    QLabel *weatherLabel;
    QLabel *windLabel;
    QLabel *locationLabel;
    QLabel *iconLabel;

    QComboBox *cityBox;
    QPushButton *btnGet;

    QMovie *loadingMovie;
    QGraphicsOpacityEffect *iconEffect;
    QPropertyAnimation *iconAnim;

    Client *client;
    QMap<QString, QString> cityMap;
};

#endif // WEATHER_H
