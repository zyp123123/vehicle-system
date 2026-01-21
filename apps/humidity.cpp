#include "humidity.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QDebug>
#include <QDir>

#include <numeric>
#include <algorithm>

#include "tools/returnbutton.h"

Humidity::Humidity(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(20);

    /* ================= 温湿度显示 ================= */
    QHBoxLayout *topRow = new QHBoxLayout;

    tempLabel = new QLabel("🌡 温度: -- °C", this);
    humiLabel = new QLabel("💧 湿度: -- %", this);

    tempLabel->setStyleSheet("font-size:26px;");
    humiLabel->setStyleSheet("font-size:26px;");

    topRow->addWidget(tempLabel);
    topRow->addWidget(humiLabel);
    layout->addLayout(topRow);

    /* ================= 统计信息 ================= */
    tempStats = new QLabel("温度：Min -- | Max -- | Avg --", this);
    humiStats = new QLabel("湿度：Min -- | Max -- | Avg --", this);

    tempStats->setStyleSheet("font-size:20px;");
    humiStats->setStyleSheet("font-size:20px;");

    layout->addWidget(tempStats);
    layout->addWidget(humiStats);

    /* ================= 折线图 ================= */
    tempSeries = new QLineSeries();
    humiSeries = new QLineSeries();
    tempSeries->setName("温度");
    humiSeries->setName("湿度");

    chart = new QChart();
    chart->addSeries(tempSeries);
    chart->addSeries(humiSeries);
    chart->setTitle("温湿度变化趋势");

    axisX = new QValueAxis;
    axisX->setRange(0, 60);
    axisX->setTitleText("时间 (秒)");
    axisX->setLabelFormat("%d");

    axisTemp = new QValueAxis;
    axisTemp->setRange(0, 50);
    axisTemp->setTitleText("温度 (°C)");

    axisHumi = new QValueAxis;
    axisHumi->setRange(0, 100);
    axisHumi->setTitleText("湿度 (%)");

    chart->setAxisX(axisX, tempSeries);
    chart->setAxisY(axisTemp, tempSeries);
    chart->setAxisX(axisX, humiSeries);
    chart->setAxisY(axisHumi, humiSeries);

    chartView = new QChartView(chart);
    chartView->setMinimumHeight(300);
    layout->addWidget(chartView);

    /* ================= 返回按钮 ================= */
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(back, &ReturnButton::requestClose,
            this, &Humidity::onBackClicked);

    /* ================= 定时器 ================= */
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &Humidity::updateData);
    timer->start(1000);

    updateData();
}

/* ================= 工具函数 ================= */

QString Humidity::trendArrow(int value, int &lastValue)
{
    if (lastValue == -1) {
        lastValue = value;
        return "→";
    }

    QString arrow = "→";
    if (value > lastValue) arrow = "↑";
    else if (value < lastValue) arrow = "↓";

    lastValue = value;
    return arrow;
}

QString Humidity::decideTempColor(int t)
{
    if (t < 10) return "blue";
    if (t <= 25) return "black";
    if (t <= 35) return "orange";
    return "red";
}

QString Humidity::decideHumiColor(int h)
{
    if (h < 30) return "blue";
    if (h < 60) return "black";
    if (h < 80) return "orange";
    return "red";
}

/* ================= 数据更新 ================= */

void Humidity::updateData()
{
    QFile file("/sys/class/misc/dht11/value");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法读取 DHT11 数据";
        return;
    }

    QString line = QString(file.readLine()).trimmed();
    QStringList parts = line.split(',');

    if (parts.size() < 2) {
        qWarning() << "DHT11 数据格式错误:" << line;
        return;
    }

    double humi = parts[0].toDouble();
    double temp = parts[1].toDouble();

    tempLabel->setText(
        QString("🌡 温度: <font color='%1'>%2°C</font> %3")
            .arg(decideTempColor((int)temp))
            .arg(temp, 0, 'f', 1)
            .arg(trendArrow((int)temp, lastTemp))
    );

    humiLabel->setText(
        QString("💧 湿度: <font color='%1'>%2%</font> %3")
            .arg(decideHumiColor((int)humi))
            .arg(humi, 0, 'f', 1)
            .arg(trendArrow((int)humi, lastHumi))
    );

    tempList.append(temp);
    humiList.append(humi);

    if (tempList.size() > 60) tempList.removeFirst();
    if (humiList.size() > 60) humiList.removeFirst();

    double tMin = *std::min_element(tempList.begin(), tempList.end());
    double tMax = *std::max_element(tempList.begin(), tempList.end());
    double tAvg = std::accumulate(tempList.begin(), tempList.end(), 0.0) / tempList.size();

    double hMin = *std::min_element(humiList.begin(), humiList.end());
    double hMax = *std::max_element(humiList.begin(), humiList.end());
    double hAvg = std::accumulate(humiList.begin(), humiList.end(), 0.0) / humiList.size();

    tempStats->setText(
        QString("温度：Min %1° | Max %2° | Avg %3°")
            .arg(tMin, 0, 'f', 1)
            .arg(tMax, 0, 'f', 1)
            .arg(tAvg, 0, 'f', 1)
    );

    humiStats->setText(
        QString("湿度：Min %1% | Max %2% | Avg %3%")
            .arg(hMin, 0, 'f', 1)
            .arg(hMax, 0, 'f', 1)
            .arg(hAvg, 0, 'f', 1)
    );

    tempSeries->clear();
    humiSeries->clear();

    for (int i = 0; i < tempList.size(); ++i) {
        tempSeries->append(i, tempList[i]);
        humiSeries->append(i, humiList[i]);
    }
}

void Humidity::onBackClicked()
{
    emit requestClose();
}
