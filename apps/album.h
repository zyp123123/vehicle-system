#ifndef ALBUM_H
#define ALBUM_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QToolButton>
#include <QPushButton>
#include <QFutureWatcher>
#include <QSet>
#include <QMap>

class Album : public QWidget
{
    Q_OBJECT

public:
    explicit Album(QWidget *parent = nullptr);
    ~Album();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    /* UI 组件 */
    QScrollArea *scrollArea;
    QWidget *contentWidget;
    QGridLayout *gridLayout;
    QLabel *fullscreenLabel;
    QPushButton *deleteModeBtn;

    /* 数据与状态 */
    QStringList imagePaths;
    QStringList imageNames;
    QVector<QToolButton*> thumbButtons;
    QVector<QFutureWatcher<void>*> watchers;
    QMap<int, QPixmap> thumbCache;

    QRect screenRect;
    QSize thumbSize;
    int currentIndex;
    bool isFullscreen;

    /* 滑动切换相关 */
    bool dragging;
    int dragStartX;

    /* 删除模式相关 */
    bool selectingMode = false;
    QSet<int> selectedIndexSet;

    /* 私有方法 */
    void loadImageList();
    void buildGrid();
    void startLoadThumbnail(int index);
    void showFullscreenAt(int index);
    void showImageAtIndex(int index);
    void exitFullscreen();
    int clampedIndex(int idx) const;

    /* 删除功能相关方法 */
    void toggleSelect(int index);
    void updateThumbStyles();
    void exitSelectMode();
    void deleteSelectedImages();

private slots:
    void onThumbnailReady(int index, const QPixmap &thumb);
    void onDeleteModeClicked();

signals:
    void requestClose();
};

#endif // ALBUM_H
