#include "mappage.h"
#include "mychannel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
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

MapPage::MapPage(QWidget *parent)
    : QWidget(parent)
{
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "7777");

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

    // 资源文件校验
    QFile file(":/mymap_ba.html");
    if(file.exists()) {
        qDebug() << "HTML file FOUND success!";
    } else {
        qDebug() << "HTML file NOT FOUND!";
    }

    m_webView->load(QUrl("qrc:/mymap_ba.html"));

    // 4. 连接信号槽
    connect(m_setCityBtn, &QPushButton::clicked, this, &MapPage::onSetCityClicked);
    connect(m_startSearchEdit, &QLineEdit::textEdited, this, &MapPage::onSearch1Changed);
    connect(m_endSearchEdit, &QLineEdit::textEdited, this, &MapPage::onSearch2Changed);
    connect(m_navBtn, &QPushButton::clicked, this, &MapPage::onNavButtonClicked);
    connect(m_startListView, &QListView::clicked, this, &MapPage::onStartListClicked);
    connect(m_endListView, &QListView::clicked, this, &MapPage::onEndListClicked);

    connect(m_channel, &myChannel::routeError, this, &MapPage::handleRouteError);
    connect(m_channel, &myChannel::setCityLable, this, &MapPage::updateCityLabel);
    connect(m_channel, &myChannel::sendAutocomplete_1, this, &MapPage::updateAutoComplete_1);
    connect(m_channel, &myChannel::sendAutocomplete_2, this, &MapPage::updateAutoComplete_2);
}

MapPage::~MapPage()
{
}

void MapPage::fillNavigationData(const QString &city, const QString &start, const QString &end)
{
    m_cityEdit->setText(city);
    m_startSearchEdit->setText(start);
    m_endSearchEdit->setText(end);

    onSearch1Changed(start);
    onSearch2Changed(end);

    checkRouteStatus();
    onNavButtonClicked();
}

QImage MapPage::captureMapImage()
{
    // 检查指针是否为空
    if (m_webView) {
        // 只截取 webView 控件的内容，不包含右侧的面板
        return m_webView->grab().toImage();
    }

    // 如果没有 webView，则返回一个空图片
        return QImage();
}

void MapPage::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(this);
    mainLayout->addWidget(m_webView, 3);

    QWidget *rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(300);
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

    // 2. 起点设置区域
    QGroupBox *startGroup = new QGroupBox("起点设置", this);
    QVBoxLayout *startLayout = new QVBoxLayout(startGroup);
    m_startSearchEdit = new QLineEdit(this);
    m_startSearchEdit->setPlaceholderText("搜索起点...");
    m_startListView = new QListView(this);
    m_startListView->setModel(m_startModel);
    startLayout->addWidget(m_startSearchEdit);
    startLayout->addWidget(m_startListView);
    rightLayout->addWidget(startGroup);

    // 3. 终点设置区域
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
    m_navBtn->setEnabled(false);
    rightLayout->addWidget(m_navBtn);

    mainLayout->addWidget(rightPanel, 1);
}

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
            QString locationStr = QString("%1,%2").arg(locationObj["lng"].toDouble(), 0, 'f', 6)
                                                  .arg(locationObj["lat"].toDouble(), 0, 'f', 6);
            QStandardItem *item = new QStandardItem(d["name"].toString());
            item->setData(locationStr, Qt::UserRole);
            m_startModel->appendRow(item);
        }
    }
    if (m_startModel->rowCount() > 0) {
        m_autoStartReady1 = false;
        QModelIndex first = m_startModel->index(0, 0);
        m_startListView->blockSignals(true);
        m_startListView->setCurrentIndex(first);
        m_startListView->blockSignals(false);
        onStartListClicked(first);
        m_autoStartReady1 = true;
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
            QString locationStr = QString("%1,%2").arg(locationObj["lng"].toDouble(), 0, 'f', 6)
                                                  .arg(locationObj["lat"].toDouble(), 0, 'f', 6);
            QStandardItem *item = new QStandardItem(d["name"].toString());
            item->setData(locationStr, Qt::UserRole);
            m_endModel->appendRow(item);
        }
    }
    if (m_endModel->rowCount() > 0) {
        m_autoStartReady2 = false;
        QModelIndex first = m_endModel->index(0, 0);
        m_endListView->blockSignals(true);
        m_endListView->setCurrentIndex(first);
        m_endListView->blockSignals(false);
        onEndListClicked(first);
        m_autoStartReady2 = true;
    }
    tryAutoStartNavigation();
}

void MapPage::onStartListClicked(const QModelIndex &index)
{
    QString locationStr = index.data(Qt::UserRole).toString();
    m_channel->startlocation(locationStr);
    m_hasStart = true;
    checkRouteStatus();
}

void MapPage::onEndListClicked(const QModelIndex &index)
{
    QString locationStr = index.data(Qt::UserRole).toString();
    m_channel->endlocation(locationStr);
    m_hasEnd = true;
    checkRouteStatus();
}

void MapPage::checkRouteStatus()
{
    m_navBtn->setEnabled(true);
}

void MapPage::onNavButtonClicked()
{
    if (m_hasStart && m_hasEnd) {
        m_channel->selectRoute();
    }
}

void MapPage::handleRouteError(QString msg)
{
    QMessageBox::critical(this, "坐标错误", msg);
}

void MapPage::tryAutoStartNavigation()
{
    if (m_autoStartReady1 && m_autoStartReady2 && m_hasStart && m_hasEnd) {
        onNavButtonClicked();
    }
}
