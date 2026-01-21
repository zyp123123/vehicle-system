#ifndef ALBUM_H
#define ALBUM_H

#include <QWidget>
#include <QLabel>
#include <QStringList>
#include <QVector>
#include <QPixmap>
#include <QFutureWatcher>
#include <QPushButton>
#include <QHash>
#include <QSet>

class QScrollArea;
class QGridLayout;
class QToolButton;

/* -------------------------
 * 相册页面
 * ------------------------ */
class Album : public QWidget
{
    Q_OBJECT
public:
    explicit Album(QWidget *parent = nullptr);
    ~Album();

signals:
    void requestClose();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onThumbnailReady(int index, const QPixmap &thumb);

private:
    /* 数据加载与 UI */
    void loadImageList();
    void buildGrid();
    void startLoadThumbnail(int index);

    /* 全屏显示 */
    void showFullscreenAt(int index);
    void showImageAtIndex(int index);
    void exitFullscreen();
    int  clampedIndex(int idx) const;

    /* 删除模式 */
    void onDeleteModeClicked();
    void exitSelectMode();
    void toggleSelect(int index);
    void updateThumbStyles();
    void deleteSelectedImages();

private:
    /* ---------- UI ---------- */
    QScrollArea *scrollArea = nullptr;
    QWidget     *contentWidget = nullptr;
    QGridLayout *gridLayout = nullptr;

    QLabel *fullscreenLabel = nullptr;

    QPushButton *deleteModeBtn = nullptr;

    /* ---------- 状态 ---------- */
    bool isFullscreen = false;
    int  currentIndex = -1;

    bool dragging = false;
    int  dragStartX = 0;

    bool selectingMode = false;

    /* ---------- 数据 ---------- */
    QStringList imagePaths;
    QStringList imageNames;

    QVector<QToolButton*> thumbButtons;
    QVector<QFutureWatcher<void>*> watchers;
    QHash<int, QPixmap> thumbCache;

    QSet<int> selectedIndexSet;

    /* ---------- 配置 ---------- */
    QSize thumbSize;
    QRect screenRect;
};

#endif // ALBUM_H
