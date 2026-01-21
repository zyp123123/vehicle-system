#ifndef HUMIDITY_H
#define HUMIDITY_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QList>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

class Humidity : public QWidget
{
    Q_OBJECT
public:
    explicit Humidity(QWidget *parent = nullptr);
    ~Humidity() = default;

signals:
    void requestClose();

private slots:
    void updateData();
    void onBackClicked();

private:
    /* ---------- UI ---------- */
    QLabel *tempLabel;
    QLabel *humiLabel;
    QLabel *tempStats;
    QLabel *humiStats;

    QTimer *timer;

    /* ---------- 折线图 ---------- */
    QChart *chart;
    QChartView *chartView;
    QLineSeries *tempSeries;
    QLineSeries *humiSeries;
    QValueAxis *axisX;
    QValueAxis *axisTemp;
    QValueAxis *axisHumi;

    /* ---------- 数据 ---------- */
    QList<double> tempList;
    QList<double> humiList;

    int lastTemp = -1;
    int lastHumi = -1;

    /* ---------- 工具函数 ---------- */
    QString decideTempColor(int t);
    QString decideHumiColor(int h);
    QString trendArrow(int value, int &lastValue);
};

#endif // HUMIDITY_H
