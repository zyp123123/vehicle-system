#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QSpacerItem>
#include <QDebug>
#include <QVector>
#include "tools/mediaobjectinfo.h"

class MusicPlayer : public QWidget
{
    Q_OBJECT

public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

private:
    /*媒体播放器，用于播放音乐*/
    QMediaPlayer *musicPlayer;
    /*媒体列表*/
    QMediaPlaylist *mediaPlaylist;
    /*音乐列表*/
    QListWidget *listWidget;
    /*播放进度条*/
    QSlider *durationSlider;
    /*音乐播放器按钮*/
    QPushButton *pushButton[7];
    /*垂直布局*/
    QVBoxLayout *vBoxLayout[3];
    /*水平布局*/
    QHBoxLayout *hBoxLayout[4];
    /*垂直容器*/
    QWidget *vWidget[3];
    /*水平容器*/
    QWidget *hWidget[4];
    /*标签文本*/
    QLabel *label[4];
    /*用于遮罩*/
    QWidget *listMask;

    /*音乐布局函数*/
    void musicLayout();
    /*主窗体大小重设大小函数重写*/
    void resizeEvent(QResizeEvent *event);
    /*媒体信息存储*/
    QVector<MediaObjectInfo> mediaObjectInfo;

    /*异步扫描歌曲*/
    void scanSongsAsync();
    /*媒体播放器类初始化*/
    void mediaPlayerInit();

private slots:
    /*扫描完成后的回调*/
    void onSongsScanned(const QVector<MediaObjectInfo> &songs);
    /*播放按钮点击*/
    void btn_play_clicked();
    /*下一曲按钮点击*/
    void btn_next_clicked();
    /*上一曲按钮点击*/
    void btn_previous_clicked();
    /*媒体按钮改变*/
    void mediaPlayerStateChanged(QMediaPlayer::State);
    /*列表单击*/
    void listWidgetClicked(QListWidgetItem*);
    /*媒体列表项改变*/
    void mediaPlaylistCurrentIndexChanged(int);
    /*媒体总长度改变*/
    void musicPlayerDurationChanged(qint64);
    /*媒体播放位置改变*/
    void mediaPlayerPositionChanged(qint64);
    /*播放进度条松开*/
    void durationSliderReleased();

signals:
    void requestClose();
};

#endif // MUSIC_PLAYER_H
