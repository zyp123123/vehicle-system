#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include "apps/weather.h"
#include "apps/mappage.h"
#include "apps/mapdebugpage.h"
#include "apps/mapserver.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), currentIndex(0)
{
    setupUI();
    createPages();
    this->showMaximized();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    stackedWidget = new QStackedWidget(this);
    btnPrev = new QPushButton("<", this);
    btnNext = new QPushButton(">", this);

    btnPrev->setFixedWidth(50);
    btnNext->setFixedWidth(50);

    // 加一点样式让按钮好看点（可选）
    btnPrev->setStyleSheet("font-size:20px; font-weight:bold;");
    btnNext->setStyleSheet("font-size:20px; font-weight:bold;");

    connect(btnPrev, &QPushButton::clicked, this, &MainWindow::showPreviousPage);
    connect(btnNext, &QPushButton::clicked, this, &MainWindow::showNextPage);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(btnPrev);
    hLayout->addWidget(stackedWidget, 1);
    hLayout->addWidget(btnNext);

    central->setLayout(hLayout);
}

void MainWindow::addPage(QWidget *page)
{
    stackedWidget->addWidget(page);
}

void MainWindow::createPages()
{
    // -------- 页面 0：天气 --------
    addPage(new Weather());

    // -------- 页面 1：地图导航 --------
    MapPage *mapPage = new MapPage();
    addPage(mapPage);

    // -------- 页面 2：地图 TCP 调试 --------
    MapDebugPage *debugPage = new MapDebugPage();
    addPage(debugPage);

    // -------- 创建 MapServer 并连接日志到 debugPage --------
    mapServer = new MapServer(this);
    mapServer->start(5555);   // 监听端口5555
    connect(mapServer, &MapServer::logMessage,
            debugPage, &MapDebugPage::appendLog);
}

void MainWindow::showPreviousPage()
{
    currentIndex--;
    if (currentIndex < 0)
        currentIndex = stackedWidget->count() - 1;
    stackedWidget->setCurrentIndex(currentIndex);
}

void MainWindow::showNextPage()
{
    currentIndex++;
    if (currentIndex >= stackedWidget->count())
        currentIndex = 0;
    stackedWidget->setCurrentIndex(currentIndex);
}
