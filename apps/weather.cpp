#include "weather.h"
#include "tools/returnbutton.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>
#include <QDebug>

/* ================= ctor ================= */

Weather::Weather(QWidget *parent)
    : QWidget(parent)
{
    client = new Client(this);
    connect(client, &Client::weatherReceived,
            this, &Weather::onWeatherJson);
    connect(client, &Client::errorOccurred,
            this, &Weather::onError);

    loadingMovie = new QMovie(":/images/weatherimages/loading.png", QByteArray(), this);

    iconEffect = new QGraphicsOpacityEffect(this);
    iconAnim = new QPropertyAnimation(iconEffect, "opacity", this);
    iconAnim->setDuration(300);
    iconAnim->setStartValue(0.0);
    iconAnim->setEndValue(1.0);

    setupUi();
    setupStyle();
    loadCityList();
}

/* ================= UI ================= */

void Weather::setupUi()
{
    /* 顶部栏 */
    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->setContentsMargins(8, 8, 8, 8);

    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose,
            this, &Weather::requestClose);

    QLabel *title = new QLabel("天气");
    QFont titleFont;
    titleFont.setPointSize(26);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    topBar->addWidget(back);
    topBar->addStretch();
    topBar->addWidget(title);
    topBar->addStretch();
    topBar->addSpacing(36);

    /* 城市选择 */
    cityBox = new QComboBox;
    cityBox->setEditable(true);
    cityBox->setInsertPolicy(QComboBox::NoInsert);
    cityBox->setFixedHeight(48);
    cityBox->setMinimumWidth(300);

    btnGet = new QPushButton("获取天气");
    btnGet->setFixedHeight(48);
    btnGet->setMinimumWidth(140);
    connect(btnGet, &QPushButton::clicked,
            this, &Weather::onRequestWeather);

    QHBoxLayout *sel = new QHBoxLayout;
    sel->addWidget(cityBox);
    sel->addWidget(btnGet);

    /* 天气卡片 */
    QFrame *card = new QFrame;
    card->setObjectName("weatherCard");

    QHBoxLayout *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 12, 12);

    iconLabel = new QLabel;
    iconLabel->setFixedSize(200, 200);
    iconLabel->setScaledContents(true);
    iconLabel->setGraphicsEffect(iconEffect);

    locationLabel = new QLabel("--");
    QFont locFont;
    locFont.setPointSize(28);
    locFont.setBold(true);
    locationLabel->setFont(locFont);
    locationLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *left = new QVBoxLayout;
    left->addWidget(iconLabel, 0, Qt::AlignHCenter);
    left->addWidget(locationLabel, 0, Qt::AlignHCenter);

    tempLabel = new QLabel("--°C");
    QFont tempFont;
    tempFont.setPointSize(60);
    tempFont.setBold(true);
    tempLabel->setFont(tempFont);

    weatherLabel = new QLabel("天气：--");
    humiLabel = new QLabel("湿度：--");
    windLabel = new QLabel("风向：--");

    QFont infoFont;
    infoFont.setPointSize(26);
    weatherLabel->setFont(infoFont);
    humiLabel->setFont(infoFont);
    windLabel->setFont(infoFont);

    QVBoxLayout *right = new QVBoxLayout;
    right->addWidget(tempLabel);
    right->addWidget(weatherLabel);
    right->addWidget(humiLabel);
    right->addWidget(windLabel);
    right->addStretch();

    cardLayout->addLayout(left);
    cardLayout->addLayout(right);
    card->setMinimumHeight(300);

    /* 主布局 */
    QVBoxLayout *main = new QVBoxLayout(this);
    main->addLayout(topBar);
    main->addLayout(sel);
    main->addWidget(card);
    main->addStretch();
}

/* ================= Style ================= */

void Weather::setupStyle()
{
    setStyleSheet(R"(
        #weatherCard {
            background: white;
            border-radius: 12px;
            border: 1px solid rgba(0,0,0,0.07);
        }
        QPushButton {
            background: qlineargradient(x1:0,y1:0, x2:0,y2:1,
                        stop:0 #63b3ff, stop:1 #3b8de6);
            color: white;
            border-radius: 6px;
        }
        QPushButton:pressed {
            opacity: 0.85;
        }
    )");
}

/* ================= City ================= */

void Weather::loadCityList()
{
    struct City { const char *name; const char *code; } citys[] = {
        {"北京","110000"},{"上海","310000"},{"广州","440100"},{"深圳","440300"},
        {"杭州","330100"},{"南京","320100"},{"成都","510100"},{"武汉","420100"},
        {"西安","610100"},{"长沙","430100"},{"厦门","350200"},{"青岛","370200"}
    };

    QStringList names;
    for (auto &c : citys) {
        cityMap.insert(c.name, c.code);
        names << c.name;
        cityBox->addItem(c.name);
    }

    QCompleter *comp = new QCompleter(names, this);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    cityBox->setCompleter(comp);
}

QString Weather::cityCode(const QString &name)
{
    if (cityMap.contains(name))
        return cityMap.value(name);

    for (auto it = cityMap.begin(); it != cityMap.end(); ++it) {
        if (it.key().contains(name))
            return it.value();
    }
    return "110000"; // 北京
}

/* ================= Logic ================= */

void Weather::onRequestWeather()
{
    QString city = cityBox->currentText();
    if (city.isEmpty())
        return;

    showLoading(true);
    client->requestWeather("192.168.2.222", 8888, cityCode(city));
}

void Weather::onWeatherJson(QString json)
{
    showLoading(false);

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject())
        return;

    QJsonArray arr = doc["lives"].toArray();
    if (arr.isEmpty())
        return;

    QJsonObject d = arr.first().toObject();

    tempLabel->setText(d["temperature"].toString() + "°C");
    humiLabel->setText("湿度：" + d["humidity"].toString() + "%");
    windLabel->setText("风向：" + d["winddirection"].toString());
    weatherLabel->setText("天气：" + d["weather"].toString());
    locationLabel->setText(d["city"].toString());

    QString icon = iconForWeather(d["weather"].toString());
    QPixmap px(icon);
    if (!px.isNull()) {
        iconLabel->setPixmap(px.scaled(
            iconLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
        animateIcon();
    }
}

void Weather::onError(QString err)
{
    showLoading(false);
    qWarning() << "Weather error:" << err;
}

/* ================= Helpers ================= */

QString Weather::iconForWeather(const QString &w)
{
    if (w.contains("晴")) return ":/images/weatherimages/sun.png";
    if (w.contains("雨")) return ":/images/weatherimages/raining.png";
    if (w.contains("雪")) return ":/images/weatherimages/snowy.png";
    if (w.contains("云") || w.contains("阴")) return ":/images/weatherimages/clouds.png";
    if (w.contains("雾") || w.contains("霾")) return ":/images/weatherimages/windy.png";
    return ":/images/weatherimages/sun.png";
}

void Weather::showLoading(bool on)
{
    if (on) {
        iconLabel->setMovie(loadingMovie);
        loadingMovie->start();
    } else {
        loadingMovie->stop();
        iconLabel->setMovie(nullptr);
    }
}

void Weather::animateIcon()
{
    iconAnim->stop();
    iconEffect->setOpacity(0.0);
    iconAnim->start();
}
