#include "mappage.h"
#include "mychannel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QWebEngineView>
#include <QWebChannel>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QJsonArray>
#include <QDebug>
#include <QGroupBox>
#include <QFile>

MapPage::MapPage(QWidget *parent) : QWidget(parent)
{
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "7777");  // 开启远程调试

    // 1. 初始化数据模型和通信通道
    m_channel = new myChannel(this);
    m_startModel = new QStandardItemModel(this);
    m_endModel = new QStandardItemModel(this);

    // 2. 构建界面
    setupUI();

    // 3. 配置 WebEngine 和 WebChannel
    QWebChannel *webChannel = new QWebChannel(this);
    webChannel->registerObject("qtChannel", m_channel);
    m_webView->page()->setWebChannel(webChannel);

    // ★★★ 加上这行调试代码 ★★★
    QFile file(":/mymap_ba.html");  // 注意冒号开头，等同于 qrc:/
    if(file.exists()) {
        qDebug() << "HTML file FOUND success!";
    } else {
        qDebug() << "HTML file NOT FOUND! Please check .qrc prefix.";
    }

    // 注意：这里假设你的资源文件里有 mymap_ba.html
    m_webView->load(QUrl("qrc:/mymap_ba.html"));

    // 4. 连接信号槽
    // UI -> Logic
    connect(m_setCityBtn, &QPushButton::clicked, this, &MapPage::onSetCityClicked);
    connect(m_startSearchEdit, &QLineEdit::textEdited, this, &MapPage::onSearch1Changed);
    connect(m_endSearchEdit, &QLineEdit::textEdited, this, &MapPage::onSearch2Changed);
    connect(m_navBtn, &QPushButton::clicked, this, &MapPage::onNavButtonClicked);
    connect(m_startListView, &QListView::clicked, this, &MapPage::onStartListClicked);
    connect(m_endListView, &QListView::clicked, this, &MapPage::onEndListClicked);

    // Channel -> Logic/UI
    connect(m_channel, &myChannel::routeError, this, &MapPage::handleRouteError);
    connect(m_channel, &myChannel::setCityLable, this, &MapPage::updateCityLabel);
    connect(m_channel, &myChannel::sendAutocomplete_1, this, &MapPage::updateAutoComplete_1);
    connect(m_channel, &myChannel::sendAutocomplete_2, this, &MapPage::updateAutoComplete_2);
}

MapPage::~MapPage()
{
}

void MapPage::setupUI()
{
    // 主布局：水平 (左边地图，右边菜单)
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // --- 左边：地图 ---
    m_webView = new QWebEngineView(this);
    // 地图占据更多空间 (比如 3:1)
    mainLayout->addWidget(m_webView, 3);

    // --- 右边：菜单栏 ---
    QWidget *rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(300);  // 或者使用 layout stretch
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    // 1. 城市设置区域
    QGroupBox *cityGroup = new QGroupBox("城市设置", this);
    QVBoxLayout *cityLayout = new QVBoxLayout(cityGroup);
    QHBoxLayout *cityInputLayout = new QHBoxLayout;
    m_cityEdit = new QLineEdit(this);
    m_cityEdit->setPlaceholderText("输入城市...");
    m_setCityBtn = new QPushButton("确定", this);
    cityInputLayout->addWidget(m_cityEdit);
    cityInputLayout->addWidget(m_setCityBtn);
    m_cityLabel = new QLabel("当前城市: 未定位", this);
    cityLayout->addLayout(cityInputLayout);
    cityLayout->addWidget(m_cityLabel);
    rightLayout->addWidget(cityGroup);

    // 2. 起点搜索区域
    QGroupBox *startGroup = new QGroupBox("起点设置", this);
    QVBoxLayout *startLayout = new QVBoxLayout(startGroup);
    m_startSearchEdit = new QLineEdit(this);
    m_startSearchEdit->setPlaceholderText("搜索起点...");
    m_startListView = new QListView(this);
    m_startListView->setModel(m_startModel);
    startLayout->addWidget(m_startSearchEdit);
    startLayout->addWidget(m_startListView);
    rightLayout->addWidget(startGroup);

    // 3. 终点搜索区域
    QGroupBox *endGroup = new QGroupBox("终点设置", this);
    QVBoxLayout *endLayout = new QVBoxLayout(endGroup);
    m_endSearchEdit = new QLineEdit(this);
    m_endSearchEdit->setPlaceholderText("搜索终点...");
    m_endListView = new QListView(this);
    m_endListView->setModel(m_endModel);
    endLayout->addWidget(m_endSearchEdit);
    endLayout->addWidget(m_endListView);
    rightLayout->addWidget(endGroup);

    // 4. 导航按钮
    m_navBtn = new QPushButton("开始导航", this);
    m_navBtn->setFixedHeight(40);
    m_navBtn->setEnabled(false);  // 初始禁用
    rightLayout->addWidget(m_navBtn);

    // 将右侧面板加入主布局
    mainLayout->addWidget(rightPanel, 1);
}

// ---- 逻辑实现 ----
void MapPage::onSetCityClicked()
{
    QString city = m_cityEdit->text().trimmed();
    if (city.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a city!");
        return;
    }
    m_channel->setCity(city);
}

void MapPage::updateCityLabel(QString city)
{
    m_cityLabel->setText("当前城市: " + city);
}

void MapPage::onSearch1Changed(const QString &text)
{
    m_channel->inputChanged_1(text.trimmed());
}

void MapPage::onSearch2Changed(const QString &text)
{
    m_channel->inputChanged_2(text.trimmed());
}

void MapPage::updateAutoComplete_1(QJsonObject result)
{
    m_startModel->clear();
    if (!result.contains("tips") || !result["tips"].isArray()) return;

    QJsonArray tips = result["tips"].toArray();
    for (const auto& e : tips) {
        QJsonObject d = e.toObject();
        if (d.contains("name") && d.contains("location")) {
            QJsonObject locationObj = d["location"].toObject();
            double lng = locationObj["lng"].toDouble();
            double lat = locationObj["lat"].toDouble();
            QString locationStr = QString("%1,%2").arg(lng, 0, 'f', 6).arg(lat, 0, 'f', 6);
            QStandardItem *item = new QStandardItem(d["name"].toString());
            item->setData(locationStr, Qt::UserRole);
            m_startModel->appendRow(item);
        }
    }
}

void MapPage::updateAutoComplete_2(QJsonObject result)
{
    m_endModel->clear();
    if (!result.contains("tips") || !result["tips"].isArray()) return;

    QJsonArray tips = result["tips"].toArray();
    for (const auto& e : tips) {
        QJsonObject d = e.toObject();
        if (d.contains("name") && d.contains("location")) {
            QJsonObject locationObj = d["location"].toObject();
            double lng = locationObj["lng"].toDouble();
            double lat = locationObj["lat"].toDouble();
            QString locationStr = QString("%1,%2").arg(lng, 0, 'f', 6).arg(lat, 0, 'f', 6);
            QStandardItem *item = new QStandardItem(d["name"].toString());
            item->setData(locationStr, Qt::UserRole);
            m_endModel->appendRow(item);
        }
    }
}

void MapPage::onStartListClicked(const QModelIndex &index)
{
    QString locationStr = index.data(Qt::UserRole).toString();
    qDebug() << "Start Location Selected:" << locationStr;
    m_channel->startlocation(locationStr);
    checkRouteStatus();
}

void MapPage::onEndListClicked(const QModelIndex &index)
{
    QString locationStr = index.data(Qt::UserRole).toString();
    qDebug() << "End Location Selected:" << locationStr;
    m_channel->endlocation(locationStr);
    checkRouteStatus();
}

void MapPage::checkRouteStatus()
{
    // 这里简单的判断逻辑：只要两个搜索列表都有内容，并且用户都点击过了(通常需要更严格的状态记录，
    // 但为了保持原代码逻辑，这里只开启按钮，实际依赖 web 端的状态)
    // 更好的做法是增加成员变量 bool m_hasStart, m_hasEnd，点击列表时置为 true
    m_navBtn->setEnabled(true);
}

void MapPage::onNavButtonClicked()
{
    // 如果需要更严格检查，可以在这里判断 m_hasStart && m_hasEnd
    m_channel->selectRoute();
}

void MapPage::handleRouteError(QString msg)
{
    QMessageBox::critical(this, "坐标错误", msg);
}
