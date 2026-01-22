#include "album.h"

#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMessageBox>
#include <QApplication>

#include "tools/returnbutton.h"
#include "tools/appthreadpool.h"

/* -------------------------
 * 构造 / 析构
 * ------------------------ */
Album::Album(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #FFFFFF;");

    screenRect = QRect(0, 0, 800, 480);
    setFixedSize(screenRect.size());

    /* ---------- 全屏预览 ---------- */
    fullscreenLabel = new QLabel(this);
    fullscreenLabel->setGeometry(screenRect);
    fullscreenLabel->setAlignment(Qt::AlignCenter);
    fullscreenLabel->setVisible(false);
    fullscreenLabel->setStyleSheet("background-color: black;");
    fullscreenLabel->setScaledContents(false);
    fullscreenLabel->installEventFilter(this);

    /* ---------- 滚动区域 ---------- */
    scrollArea = new QScrollArea(this);
    scrollArea->setGeometry(screenRect);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollArea->viewport()->installEventFilter(this);

    contentWidget = new QWidget;
    gridLayout = new QGridLayout(contentWidget);
    gridLayout->setSpacing(8);
    gridLayout->setContentsMargins(8, 8, 8, 8);
    gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scrollArea->setWidget(contentWidget);

    /* ---------- 加载数据 ---------- */
    loadImageList();

    int cols = 4;
    int spacing = gridLayout->spacing();
    int usableWidth = screenRect.width() - spacing * (cols - 1) - 16;
    int cellW = usableWidth / cols;
    int cellH = cellW * 0.625;
    thumbSize = QSize(cellW, cellH);

    buildGrid();

    /* ---------- 删除按钮 ---------- */
    deleteModeBtn = new QPushButton(this);
    deleteModeBtn->setFixedSize(50, 50);
    deleteModeBtn->move(730, 10);
    deleteModeBtn->setStyleSheet(
        "border-image: url(:/images/icons/delete.png);"
        "background: rgba(0,0,0,100);"
        "border-radius:25px;"
    );
    deleteModeBtn->raise();
    connect(deleteModeBtn, &QPushButton::clicked,
            this, &Album::onDeleteModeClicked);

    /* ---------- 返回按钮 ---------- */
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(back, &ReturnButton::requestClose, this, [=](){
        emit requestClose();
    });
}

Album::~Album()
{
    for (auto w : watchers) {
        if (!w) continue;
        w->disconnect();
        if (w->isRunning()) {
            w->cancel();
            w->waitForFinished();
        }
        delete w;
    }
}

/* -------------------------
 * 加载图片列表
 * ------------------------ */
void Album::loadImageList()
{
    QString folder = QCoreApplication::applicationDirPath() + "/myAlbum";
    QDir dir(folder);

    if (!dir.exists()) {
        qWarning() << "Album folder not found:" << folder;
        return;
    }

    QStringList filters = {
        "*.jpg","*.jpeg","*.png","*.bmp","*.gif"
    };

    QFileInfoList files =
        dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fi : files) {
        imagePaths.append(fi.absoluteFilePath());
        imageNames.append(fi.completeBaseName());
    }
}

/* -------------------------
 * 构建缩略图网格
 * ------------------------ */
void Album::buildGrid()
{
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (auto w : watchers) {
        if (!w) continue;
        w->disconnect();
        w->cancel();
        w->waitForFinished();
        delete w;
    }

    watchers.clear();
    thumbButtons.clear();
    thumbCache.clear();

    int count = imagePaths.size();
    if (count == 0) {
        QLabel *lbl = new QLabel(
            "相册为空\n请将图片放到 myAlbum 文件夹",
            contentWidget
        );
        lbl->setStyleSheet("font-size:16px;color:#666;");
        gridLayout->addWidget(lbl, 0, 0);
        return;
    }

    watchers.resize(count);
    watchers.fill(nullptr);
    thumbButtons.resize(count);

    int cols = 4;
    int r = 0, c = 0;

    for (int i = 0; i < count; ++i) {
        QWidget *cell = new QWidget;
        QVBoxLayout *v = new QVBoxLayout(cell);
        v->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

        QToolButton *thumb = new QToolButton;
        thumb->setFixedSize(thumbSize);
        thumb->setIconSize(thumbSize);
        thumb->setStyleSheet(
            "QToolButton { border:2px solid #CCC; background:#F8F8F8; }"
        );

        QLabel *name = new QLabel(imageNames.value(i));
        name->setAlignment(Qt::AlignCenter);

        v->addWidget(thumb);
        v->addWidget(name);

        connect(thumb, &QToolButton::clicked, this, [this, i](){
            selectingMode ? toggleSelect(i) : showFullscreenAt(i);
        });

        thumbButtons[i] = thumb;
        gridLayout->addWidget(cell, r, c);

        if (++c >= cols) { c = 0; ++r; }

        startLoadThumbnail(i);
    }
}

/* -------------------------
 * 异步加载缩略图
 * ------------------------ */
void Album::startLoadThumbnail(int index)
{
    if (index < 0 || index >= imagePaths.size()) return;

    if (watchers[index]) {
        watchers[index]->disconnect();
        watchers[index]->cancel();
        watchers[index]->waitForFinished();
        delete watchers[index];
    }

    QFutureWatcher<void> *watcher =
        new QFutureWatcher<void>(this);
    watchers[index] = watcher;

    QString path = imagePaths[index];

    connect(watcher, &QFutureWatcher<void>::finished, this, [=](){
        if (thumbCache.contains(index) && thumbButtons[index]) {
            thumbButtons[index]->setIcon(
                QIcon(thumbCache.value(index))
            );
        }
    });

    watcher->setFuture(QtConcurrent::run(
        AppThreadPool::instance(),
        [=](){
            QPixmap pix(path);
            if (pix.isNull()) return;
            QPixmap thumb = pix.scaled(
                thumbSize, Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            QMetaObject::invokeMethod(
                this, "onThumbnailReady",
                Qt::QueuedConnection,
                Q_ARG(int, index),
                Q_ARG(QPixmap, thumb)
            );
        }
    ));
}

void Album::onThumbnailReady(int index, const QPixmap &thumb)
{
    thumbCache[index] = thumb;
    if (thumbButtons[index])
        thumbButtons[index]->setIcon(QIcon(thumb));
}

/* -------------------------
 * 全屏查看 / 切换
 * ------------------------ */
void Album::showFullscreenAt(int index)
{
    currentIndex = index;
    showImageAtIndex(index);
    isFullscreen = true;
    fullscreenLabel->setVisible(true);
    fullscreenLabel->raise();
}

void Album::showImageAtIndex(int index)
{
    QPixmap pix(imagePaths[index]);
    QSize screen = screenRect.size();

    fullscreenLabel->setPixmap(
        pix.scaled(screen, Qt::KeepAspectRatio,
                   Qt::SmoothTransformation)
    );
}

void Album::exitFullscreen()
{
    isFullscreen = false;
    fullscreenLabel->setVisible(false);
    currentIndex = -1;
}

/* -------------------------
 * 工具函数
 * ------------------------ */
int Album::clampedIndex(int idx) const
{
    if (idx < 0) return 0;
    if (idx >= imagePaths.size()) return imagePaths.size() - 1;
    return idx;
}

/* -------------------------
 * 删除模式
 * ------------------------ */
void Album::onDeleteModeClicked()
{
    if (!selectingMode) {
        selectingMode = true;
        selectedIndexSet.clear();
        updateThumbStyles();
        deleteModeBtn->setStyleSheet(
            "border-image: url(:/images/icons/delete_confirm.png);"
            "background: rgba(255,0,0,150);"
        );
        return;
    }

    if (selectedIndexSet.isEmpty()) {
        exitSelectMode();
        return;
    }

    if (QMessageBox::question(
            this, "删除照片",
            QString("确定删除 %1 张照片？")
                .arg(selectedIndexSet.size()))
        == QMessageBox::Yes) {
        deleteSelectedImages();
    }

    exitSelectMode();
}

void Album::exitSelectMode()
{
    selectingMode = false;
    selectedIndexSet.clear();
    updateThumbStyles();
    deleteModeBtn->setStyleSheet(
        "border-image: url(:/images/icons/delete.png);"
        "background: rgba(0,0,0,100);"
    );
}

void Album::toggleSelect(int index)
{
    if (selectedIndexSet.contains(index)) {
        selectedIndexSet.remove(index);
    } else {
        selectedIndexSet.insert(index);
    }
    updateThumbStyles();
}

void Album::updateThumbStyles()
{
    for (int i = 0; i < thumbButtons.size(); ++i) {
        if (!thumbButtons[i]) continue;
        thumbButtons[i]->setStyleSheet(
            selectedIndexSet.contains(i)
            ? "QToolButton { border:3px solid #409EFF; }"
            : "QToolButton { border:2px solid #CCC; }"
        );
    }
}

void Album::deleteSelectedImages()
{
    QList<int> list = selectedIndexSet.values();
    std::sort(list.begin(), list.end(), std::greater<int>());

    for (int idx : list) {
        QFile::remove(imagePaths[idx]);
        imagePaths.removeAt(idx);
        imageNames.removeAt(idx);
        delete watchers.takeAt(idx);
        thumbButtons.removeAt(idx);
    }

    buildGrid();
}

/* -------------------------
 * 事件过滤
 * ------------------------ */
bool Album::eventFilter(QObject *obj, QEvent *event)
{
    // 目前不处理，交给父类
    return QWidget::eventFilter(obj, event);
}

/* -------------------------
 * 键盘事件
 * ------------------------ */
void Album::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
}
