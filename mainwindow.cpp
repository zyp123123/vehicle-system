#include "mainwindow.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QDateTime>
#include <QFont>
#include <QDebug>

#include "tools/slidepage.h"

#include "apps/musicplayer.h"
#include "apps/videoplayer.h"
#include "apps/monitor.h"
#include "apps/album.h"
#include "apps/humidity.h"
#include "apps/settings.h"
#include "apps/parking.h"
#include "apps/sentinel.h"
#include "apps/weather.h"
#include "apps/map.h"
#include "apps/remotewidget.h"
#include "apps/alarm.h"

/* -------------------------
 * 构造 / 析构
 * ------------------------ */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setFixedSize(800, 480);

    setupStack();
    setupHomePage();
    setupSecondPage();

    stack->addWidget(slide);
    stack->setCurrentWidget(slide);

    setupFactories();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000);

    updateTime();
}

MainWindow::~MainWindow()
{
    for (auto w : pages.values()) {
        stack->removeWidget(w);
        w->deleteLater();
    }
}

/* -------------------------
 * Stack
 * ------------------------ */
void MainWindow::setupStack()
{
    stack = new QStackedWidget(this);
    setCentralWidget(stack);
}

/* -------------------------
 * 首页第一页
 * ------------------------ */
void MainWindow::setupHomePage()
{
    homePage = new QWidget(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(homePage);

    timeButton = new QPushButton;
    timeButton->setFixedHeight(160);
    timeButton->setFont(QFont("Microsoft Yahei", 30, QFont::Bold));
    timeButton->setStyleSheet(
        "QPushButton { background:white; border-radius:24px; font-size:32px; }"
    );

    connect(timeButton, &QPushButton::clicked, [=]() {
        openApp(AppID::Alarm);
    });

    mainLayout->addWidget(timeButton, 1);

    QStringList names = {
        "设置","监控","音乐","视频",
        "倒车影像","哨兵模式","温湿度","远程控制"
    };

    QStringList icons = {
        ":/images/icons/settings.png",
        ":/images/icons/monitor.png",
        ":/images/icons/music-player.png",
        ":/images/icons/video-player.png",
        ":/images/icons/parking.png",
        ":/images/icons/sentinel.png",
        ":/images/icons/humidity.png",
        ":/images/icons/remote.png"
    };

    QWidget *grid = createAppGrid(
        names, icons,
        [this](int idx) {
            openApp(static_cast<AppID>(idx));
        }
    );

    mainLayout->addWidget(grid, 2);

    homePage->setFixedSize(800, 476);
}

/* -------------------------
 * 创建图标网格
 * ------------------------ */
QWidget* MainWindow::createAppGrid(
    const QStringList &names,
    const QStringList &icons,
    std::function<void(int)> onClick)
{
    QWidget *gridWidget = new QWidget;
    QGridLayout *grid = new QGridLayout(gridWidget);

    for (int i = 0; i < 8; ++i) {
        QWidget *appWidget = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(appWidget);
        layout->setAlignment(Qt::AlignHCenter);

        QPushButton *btn = new QPushButton;
        QLabel *label = new QLabel;
        label->setAlignment(Qt::AlignCenter);

        if (i < names.size() && !names[i].isEmpty()) {
            if (i < icons.size() && !icons[i].isEmpty()) {
                btn->setIcon(QIcon(icons[i]));
                btn->setFixedSize(100, 100);
                btn->setIconSize(QSize(100, 100));
                btn->setStyleSheet(
                    "QPushButton { background:white; border-radius:24px; border:1px solid #E5E5E5; }"
                    "QPushButton:pressed { background:#DCDCDC; }"
                );
            }

            label->setText(names[i]);
            connect(btn, &QPushButton::clicked, [onClick, i]() {
                onClick(i);
            });
        } else {
            btn->hide();
            label->hide();
        }

        layout->addWidget(btn);
        layout->addWidget(label);
        grid->addWidget(appWidget, i / 4, i % 4);
    }

    return gridWidget;
}

/* -------------------------
 * 第二页
 * ------------------------ */
void MainWindow::setupSecondPage()
{
    slide = new SlidePage(this);
    slide->addPage(homePage);

    secondPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(secondPage);

    timeButton2 = new QPushButton;
    timeButton2->setFixedHeight(160);
    timeButton2->setFont(timeButton->font());
    timeButton2->setStyleSheet(timeButton->styleSheet());

    connect(timeButton2, &QPushButton::clicked, [=]() {
        openApp(AppID::Alarm);
    });

    layout->addWidget(timeButton2, 1);

    QStringList names = { "相册","天气","导航","","","","","" };
    QStringList icons = {
        ":/images/icons/photobook.png",
        ":/images/icons/weather.png",
        ":/images/icons/map.png",
        "","","","",""
    };

    QWidget *grid = createAppGrid(
        names, icons,
        [this](int idx) {
            if (idx == 0) openApp(AppID::Album);
            else if (idx == 1) openApp(AppID::Weather);
            else if (idx == 2) openApp(AppID::Map);
        }
    );

    layout->addWidget(grid, 2);
    slide->addPage(secondPage);
}

/* -------------------------
 * 工厂注册
 * ------------------------ */
void MainWindow::setupFactories()
{
    reg<Settings>(AppID::Settings);
    reg<Monitor>(AppID::Monitor);
    reg<MusicPlayer>(AppID::Music);
    reg<VideoPlayer>(AppID::Video);
    reg<Parking>(AppID::Parking);
    reg<Sentinel>(AppID::Sentinel);
    reg<Humidity>(AppID::Humidity);
    reg<RemoteWidget>(AppID::Remote);
    reg<Alarm>(AppID::Alarm);
    reg<Album>(AppID::Album);
    reg<Weather>(AppID::Weather);
    reg<Map>(AppID::Map);
}

/* -------------------------
 * 时间更新
 * ------------------------ */
void MainWindow::updateTime()
{
    QString text = QDateTime::currentDateTime()
                       .toString("hh:mm:ss\nyyyy-MM-dd dddd");
    timeButton->setText(text);
    timeButton2->setText(text);
}

/* -------------------------
 * 页面关闭管理
 * ------------------------ */
void MainWindow::bindClose(QWidget *w, AppID id)
{
    connect(w, SIGNAL(requestClose()),
            this, SLOT(handlePageClose()));
    w->setProperty("pageIndex", static_cast<int>(id));
}

void MainWindow::handlePageClose()
{
    QObject *o = sender();
    if (!o) return;

    AppID id = static_cast<AppID>(
        o->property("pageIndex").toInt()
    );

    backToOriginPage();

    if (pages.contains(id)) {
        QWidget *page = pages[id];
        stack->removeWidget(page);
        pages.remove(id);
        page->deleteLater();
    }
}

/* -------------------------
 * 页面打开逻辑
 * ------------------------ */
void MainWindow::openApp(AppID id)
{
    if (pages.contains(id)) {
        stack->setCurrentWidget(pages[id]);
        return;
    }

    currentOriginPage = stack->currentWidget();

    QWidget *page = createPage(id);
    if (!page) return;

    preparePage(id, page);
    showPage(id, page);
}

QWidget* MainWindow::createPage(AppID id)
{
    if (factories.contains(id))
        return factories[id]();

    qWarning() << "Factory not found:" << static_cast<int>(id);
    return nullptr;
}

void MainWindow::preparePage(AppID id, QWidget *page)
{
    bindClose(page, id);

    if (id == AppID::Monitor) {
        auto *mon = qobject_cast<Monitor*>(page);
        if (mon) {
            connect(mon, &Monitor::openAlbum, this, [this]() {
                replaceAndOpen(AppID::Monitor, AppID::Album);
            });
        }
    }

    if (id == AppID::Sentinel) {
        auto *sen = qobject_cast<Sentinel*>(page);
        if (sen) {
            connect(sen, &Sentinel::openAlbum, this, [this]() {
                replaceAndOpen(AppID::Sentinel, AppID::Album);
            });
        }
    }
}

void MainWindow::showPage(AppID id, QWidget *page)
{
    pages[id] = page;
    stack->addWidget(page);
    stack->setCurrentWidget(page);
}

void MainWindow::replaceAndOpen(AppID oldId, AppID newId)
{
    if (pages.contains(oldId)) {
        QWidget *oldPage = pages[oldId];
        stack->removeWidget(oldPage);
        pages.remove(oldId);
        oldPage->deleteLater();
    }

    openApp(newId);
}

void MainWindow::backToOriginPage()
{
    stack->setCurrentWidget(
        currentOriginPage ? currentOriginPage : slide
    );
}
