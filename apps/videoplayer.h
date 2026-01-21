#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QVideoWidget>
#include <QVector>

#include "tools/mediaobjectinfo.h"

class VideoPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

signals:
    void requestClose();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void btn_play_clicked();
    void btn_next_clicked();
    void btn_volmeup_clicked();
    void btn_volmedown_clicked();
    void btn_fullscreen_clicked();

    void mediaPlayerStateChanged(QMediaPlayer::State state);
    void listWidgetClicked(QListWidgetItem *item);
    void mediaPlaylistCurrentIndexChanged(int index);

    void musicPlayerDurationChanged(qint64 duration);
    void mediaPlayerPositionChanged(qint64 position);
    void durationSliderReleased();
    void volumeSliderReleased();

private:
    /* Media */
    QMediaPlayer   *videoPlayer;
    QMediaPlaylist *mediaPlaylist;
    QVideoWidget   *videoWidget;

    /* UI */
    QListWidget *listWidget;
    QSlider *durationSlider;
    QSlider *volumeSlider;
    QPushButton *pushButton[5];
    QLabel *label[2];

    QHBoxLayout *hBoxLayout[3];
    QWidget *hWidget[3];
    QWidget *vWidget[2];
    QVBoxLayout *vBoxLayout[2];

    QVector<MediaObjectInfo> mediaObjectInfo;

private:
    void videoLayout();
    void mediaPlayerInit();
    void scanVideoFiles();
};

#endif // VIDEO_PLAYER_H
