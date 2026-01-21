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
#include <QVector>

#include "tools/mediaobjectinfo.h"

class MusicPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

signals:
    void requestClose();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void btn_play_clicked();
    void btn_next_clicked();
    void btn_previous_clicked();

    void mediaPlayerStateChanged(QMediaPlayer::State state);
    void listWidgetClicked(QListWidgetItem *item);
    void mediaPlaylistCurrentIndexChanged(int index);

    void musicPlayerDurationChanged(qint64 duration);
    void mediaPlayerPositionChanged(qint64 position);
    void durationSliderReleased();

    void onSongsScanned(const QVector<MediaObjectInfo> &songs);

private:
    /* UI */
    QListWidget *listWidget;
    QSlider *durationSlider;
    QPushButton *pushButton[7];
    QLabel *label[4];

    QVBoxLayout *vBoxLayout[3];
    QHBoxLayout *hBoxLayout[4];
    QWidget *vWidget[3];
    QWidget *hWidget[4];
    QWidget *listMask;

    /* Media */
    QMediaPlayer *musicPlayer;
    QMediaPlaylist *mediaPlaylist;
    QVector<MediaObjectInfo> mediaObjectInfo;

private:
    void musicLayout();
    void mediaPlayerInit();
    void scanSongsAsync();
};

#endif // MUSIC_PLAYER_H
