#include "map.h"
#include "tools/returnbutton.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QDebug>

/* ================= ctor ================= */

Map::Map(QWidget *parent)
    : QWidget(parent)
    , m_tcpClient(new MapTcpClient(this))
{
    setStyleSheet(
        "QLineEdit { height:28px; font-size:14px; }"
        "QComboBox { height:28px; font-size:14px; }"
        "QPushButton { height:36px; font-size:16px; padding:6px 12px; }"
    );

    /* ---------- 左侧地图 ---------- */
    m_imageLabel = new QLabel(this);
    m_imageLabel->setMinimumSize(500, 400);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setText("等待地图截图...");
    m_imageLabel->setStyleSheet(
        "background:#ddd; border:1px solid #bbb;"
    );

    /* ---------- 右侧菜单 ---------- */
    QLabel *cityLabel  = new QLabel("城市：");
    QLabel *startLabel = new QLabel("起点：");
    QLabel *endLabel   = new QLabel("终点：");

    m_cityEdit  = new QLineEdit(this);
    m_startEdit = new QLineEdit(this);
    m_endEdit   = new QLineEdit(this);

    m_cityCombo  = new QComboBox(this);
    m_startCombo = new QComboBox(this);
    m_endCombo   = new QComboBox(this);

    m_cityCombo->addItems({"城市", "北京", "上海", "深圳"});

    QStringList pois = {
        "地点",
        "北京站", "北京南站", "北京西站", "北京朝阳站",
        "故宫博物院", "天安门", "天坛公园"
    };
    m_startCombo->addItems(pois);
    m_endCombo->addItems(pois);

    m_navBtn = new QPushButton("开始导航", this);
    m_navBtn->setFixedWidth(160);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(12);
    rightLayout->addWidget(cityLabel);
    rightLayout->addWidget(m_cityEdit);
    rightLayout->addWidget(m_cityCombo);
    rightLayout->addWidget(startLabel);
    rightLayout->addWidget(m_startEdit);
    rightLayout->addWidget(m_startCombo);
    rightLayout->addWidget(endLabel);
    rightLayout->addWidget(m_endEdit);
    rightLayout->addWidget(m_endCombo);
    rightLayout->addSpacing(15);
    rightLayout->addWidget(m_navBtn);
    rightLayout->addStretch();

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_imageLabel, 1);
    mainLayout->addLayout(rightLayout);

    /* ---------- 信号 ---------- */
    connect(m_cityCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Map::onCityComboChanged);

    connect(m_startCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Map::onStartComboChanged);

    connect(m_endCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Map::onEndComboChanged);

    connect(m_navBtn,
            &QPushButton::clicked,
            this, &Map::onStartNavClicked);

    connect(m_tcpClient,
            &MapTcpClient::navigationCommandReceived,
            this, &Map::onTcpNavigationCommand);

    connect(m_tcpClient,
            &MapTcpClient::mapImageReceived,
            this, &Map::onTcpMapImageReceived);

    m_tcpClient->connectToServer("192.168.2.222", 5555);

    /* ---------- 返回按钮 ---------- */
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(back, &ReturnButton::requestClose,
            this, &Map::requestClose);
}

/* ================= public ================= */

void Map::setCity(const QString &city)
{
    m_cityEdit->setText(city);
    m_cityCombo->setCurrentText(city);
}

void Map::setStart(const QString &start)
{
    m_startEdit->setText(start);
    m_startCombo->setCurrentText(start);
}

void Map::setEnd(const QString &end)
{
    m_endEdit->setText(end);
    m_endCombo->setCurrentText(end);
}

void Map::startNavigation()
{
    onStartNavClicked();
}

/* ================= UI slots ================= */

void Map::onCityComboChanged(int index)
{
    m_cityEdit->setText(m_cityCombo->itemText(index));
}

void Map::onStartComboChanged(int index)
{
    m_startEdit->setText(m_startCombo->itemText(index));
}

void Map::onEndComboChanged(int index)
{
    m_endEdit->setText(m_endCombo->itemText(index));
}

/* ================= navigation ================= */

void Map::onStartNavClicked()
{
    QString city  = m_cityEdit->text();
    QString start = m_startEdit->text();
    QString end   = m_endEdit->text();

    if (city.isEmpty() || start.isEmpty() || end.isEmpty())
        return;

    m_tcpClient->sendNavigationData(city, start, end);
}

void Map::onTcpNavigationCommand(const QString &city,
                                 const QString &start,
                                 const QString &end)
{
    setCity(city);
    setStart(start);
    setEnd(end);
    startNavigation();
}

void Map::onTcpMapImageReceived(const QImage &img)
{
    if (img.isNull()) {
        qWarning() << "接收到的地图图片无效";
        return;
    }

    m_imageLabel->setPixmap(QPixmap::fromImage(img));

    QString dir = QCoreApplication::applicationDirPath() + "/myAlbum";
    QDir().mkpath(dir);

    QString filename = dir + "/map_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
        ".png";

    if (img.save(filename)) {
        qDebug() << "地图截图已保存：" << filename;
    } else {
        qWarning() << "地图保存失败：" << filename;
    }
}
