#include "album.h"
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMessageBox>
#include <algorithm>
#include <QImageReader>

#include "tools/returnbutton.h"
#include "tools/appthreadpool.h"

Album::Album(QWidget *parent)
    : QWidget(parent), currentIndex(-1), isFullscreen(false),
      dragging(false), dragStartX(0)
{
    setStyleSheet("background-color: #FFFFFF;");
    screenRect = QRect(0, 0, 800, 480);
    setFixedSize(screenRect.size());

    // 1. 全屏预览层
    fullscreenLabel = new QLabel(this);
    fullscreenLabel->setGeometry(screenRect);
    fullscreenLabel->setAlignment(Qt::AlignCenter);
    fullscreenLabel->setVisible(false);
    fullscreenLabel->setStyleSheet("background-color: black;");
    fullscreenLabel->installEventFilter(this);

    // 2. 滚动区域初始化
    scrollArea = new QScrollArea(this);
    scrollArea->setGeometry(screenRect);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
    scrollArea->viewport()->installEventFilter(this);
    scrollArea->setWidgetResizable(true);

    contentWidget = new QWidget;
    gridLayout = new QGridLayout(contentWidget);
    gridLayout->setSpacing(8);
    gridLayout->setContentsMargins(8, 8, 8, 8);
    gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scrollArea->setWidget(contentWidget);

    // 3. 计算缩略图尺寸 (4列)
    int spacing = gridLayout->spacing();
    int usableWidth = screenRect.width() - spacing * 3 - 16;
    int cellW = usableWidth / 4;
    thumbSize = QSize(cellW, cellW * 0.625);

    // 4. 加载数据与构建界面
    loadImageList();
    buildGrid();

    // 5. 功能按钮
    deleteModeBtn = new QPushButton(this);
    deleteModeBtn->setFixedSize(50, 50);
    deleteModeBtn->move(740, 10);
    deleteModeBtn->setStyleSheet("border-image: url(:/images/icons/delete.png); background: rgba(0,0,0,100); border-radius:25px;");
    connect(deleteModeBtn, &QPushButton::clicked, this, &Album::onDeleteModeClicked);

    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose, this, [=](){ emit requestClose(); });
}

Album::~Album()
{
    for (auto w : watchers) {
        if (w) {
            w->disconnect();
            if (w->isRunning()) {
                w->cancel();
                w->waitForFinished();
            }
            delete w;
        }
    }
}

void Album::loadImageList()
{
    imagePaths.clear();
    imageNames.clear();
    QString folder = QCoreApplication::applicationDirPath() + "/myAlbum";
    QDir dir(folder);
    if (!dir.exists()) return;

    QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fi : files) {
        imagePaths.append(fi.absoluteFilePath());
        imageNames.append(fi.completeBaseName());
    }
}

void Album::buildGrid()
{
    // 清理旧布局
    QLayoutItem *item;
    while ((item = gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    // 清理旧 Watchers
    for (auto w : watchers) {
        if (w) { w->disconnect(); w->cancel(); w->waitForFinished(); delete w; }
    }
    watchers.clear();
    thumbButtons.clear();
    thumbCache.clear();

    if (imagePaths.isEmpty()) {
        QLabel *lbl = new QLabel("相册为空", contentWidget);
        gridLayout->addWidget(lbl, 0, 0);
        return;
    }

    int count = imagePaths.size();
    thumbButtons.resize(count);
    watchers.resize(count);
    watchers.fill(nullptr);

    for (int i = 0; i < count; ++i) {
        QWidget *cell = new QWidget;
        QVBoxLayout *v = new QVBoxLayout(cell);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(4);

        // 缩略图
        QToolButton *thumb = new QToolButton;
        thumb->setFixedSize(thumbSize);
        thumb->setIconSize(thumbSize);
        thumb->setStyleSheet("QToolButton { border:2px solid #CCCCCC; border-radius:6px; background: #F8F8F8; }");

        // 图片名字
        QLabel *name = new QLabel;
        name->setAlignment(Qt::AlignCenter);
        name->setFixedWidth(thumbSize.width());  // 固定宽度
        name->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        name->setWordWrap(false);

        // 手动省略文字
        QFontMetrics fm(name->font());
        QString elided = fm.elidedText(imageNames.value(i), Qt::ElideRight, thumbSize.width());
        name->setText(elided);

        // 添加到布局
        v->addWidget(thumb, 0, Qt::AlignHCenter);
        v->addWidget(name);

        // 点击事件
        connect(thumb, &QToolButton::clicked, this, [this, i]() {
            if (selectingMode) toggleSelect(i);
            else showFullscreenAt(i);
        });

        thumbButtons[i] = thumb;
        gridLayout->addWidget(cell, i / 4, i % 4);

        startLoadThumbnail(i);  // 异步加载缩略图
    }

    contentWidget->adjustSize();
}

void Album::startLoadThumbnail(int index)
{
    if (index < 0 || index >= imagePaths.size()) return;

    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
    watchers[index] = watcher;

    connect(watcher, &QFutureWatcher<void>::finished, this, [this, index]() {
        if (thumbCache.contains(index)) {
            if (index < thumbButtons.size() && thumbButtons[index])
                thumbButtons[index]->setIcon(QIcon(thumbCache[index]));
        }
    });

    QString path = imagePaths.at(index);

    QFuture<void> future = QtConcurrent::run(AppThreadPool::instance(), [this, path, index]() {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        reader.setScaledSize(thumbSize);          // 直接读取缩略图大小
        QImage img = reader.read();
        if (img.isNull()) return;

        QPixmap thumb = QPixmap::fromImage(img);

        // 更新 UI
        QMetaObject::invokeMethod(this, "onThumbnailReady", Qt::QueuedConnection,
                                  Q_ARG(int, index), Q_ARG(QPixmap, thumb));
    });

    watcher->setFuture(future);
}


void Album::onThumbnailReady(int index, const QPixmap &thumb)
{
    thumbCache.insert(index, thumb);
    if (index < thumbButtons.size() && thumbButtons[index]) {
        thumbButtons[index]->setIcon(QIcon(thumb));
    }
}

void Album::showFullscreenAt(int index)
{
    currentIndex = index;
    showImageAtIndex(currentIndex);
    isFullscreen = true;
    fullscreenLabel->setVisible(true);
    fullscreenLabel->raise();
}

void Album::showImageAtIndex(int index)
{
    if (index < 0 || index >= imagePaths.size()) return;
    QPixmap pix(imagePaths.at(index));
    if (pix.isNull()) return;

    if (pix.width() <= screenRect.width() && pix.height() <= screenRect.height()) {
        fullscreenLabel->setPixmap(pix);
    } else {
        fullscreenLabel->setPixmap(pix.scaled(screenRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void Album::exitFullscreen()
{
    isFullscreen = false;
    fullscreenLabel->setVisible(false);
}

bool Album::eventFilter(QObject *obj, QEvent *event)
{
    // 缩略图滚动逻辑
    if (obj == scrollArea->viewport()) {
        static QPoint last;
        if (event->type() == QEvent::MouseButtonPress) {
            last = static_cast<QMouseEvent*>(event)->pos();
            return true;
        } else if (event->type() == QEvent::MouseMove) {
            QPoint now = static_cast<QMouseEvent*>(event)->pos();
            int dy = last.y() - now.y();
            scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + dy);
            last = now;
            return true;
        }
    }

    // 全屏手势逻辑
    if (obj == fullscreenLabel) {
        if (event->type() == QEvent::MouseButtonPress) {
            dragStartX = static_cast<QMouseEvent*>(event)->pos().x();
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            int dx = static_cast<QMouseEvent*>(event)->pos().x() - dragStartX;
            if (qAbs(dx) < 20) exitFullscreen();
            else {
                currentIndex = (dx < 0) ? clampedIndex(currentIndex + 1) : clampedIndex(currentIndex - 1);
                showImageAtIndex(currentIndex);
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Album::keyPressEvent(QKeyEvent *event)
{
    if (isFullscreen) {
        if (event->key() == Qt::Key_Right) { currentIndex = clampedIndex(currentIndex + 1); showImageAtIndex(currentIndex); }
        else if (event->key() == Qt::Key_Left) { currentIndex = clampedIndex(currentIndex - 1); showImageAtIndex(currentIndex); }
        else if (event->key() == Qt::Key_Escape) exitFullscreen();
    } else QWidget::keyPressEvent(event);
}

int Album::clampedIndex(int idx) const
{
    return qBound(0, idx, (int)imagePaths.size() - 1);
}

void Album::onDeleteModeClicked()
{
    if (!selectingMode) {
        selectingMode = true;
        selectedIndexSet.clear();
        updateThumbStyles();
        deleteModeBtn->setStyleSheet("border-image: url(:/images/icons/delete_confirm.png); background: rgba(255,0,0,150); border-radius:25px;");
    } else {
        if (selectedIndexSet.isEmpty()) exitSelectMode();
        else {
            if (QMessageBox::question(this, "删除", "确定删除选中的图片吗？") == QMessageBox::Yes) deleteSelectedImages();
            exitSelectMode();
        }
    }
}

void Album::exitSelectMode()
{
    selectingMode = false;
    selectedIndexSet.clear();
    updateThumbStyles();
    deleteModeBtn->setStyleSheet("border-image: url(:/images/icons/delete.png); background: rgba(0,0,0,100); border-radius:25px;");
}

void Album::toggleSelect(int index)
{
    if (selectedIndexSet.contains(index)) selectedIndexSet.remove(index);
    else selectedIndexSet.insert(index);
    updateThumbStyles();
}

void Album::updateThumbStyles()
{
    for (int i = 0; i < thumbButtons.size(); i++) {
        if (!thumbButtons[i]) continue;
        if (selectingMode && selectedIndexSet.contains(i))
            thumbButtons[i]->setStyleSheet("QToolButton { border:3px solid #409EFF; border-radius:6px; background:#F0F8FF; }");
        else
            thumbButtons[i]->setStyleSheet("QToolButton { border:2px solid #CCCCCC; border-radius:6px; background:#F8F8F8; }");
    }
}

void Album::deleteSelectedImages()
{
    QList<int> list = selectedIndexSet.values();
    std::sort(list.begin(), list.end(), std::greater<int>());
    for (int index : list) {
        QFile::remove(imagePaths[index]);
        imagePaths.removeAt(index);
        imageNames.removeAt(index);
    }
    buildGrid();
}
