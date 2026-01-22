#include "videoplayer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QPalette>
#include <QDebug>

#include "tools/returnbutton.h"

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
{
    QFile file(":/style/videoplayerstyle.qss");
    if (file.open(QFile::ReadOnly)) {
        setStyleSheet(file.readAll());
        file.close();
    }

    setAttribute(Qt::WA_StyledBackground, true);

    videoLayout();
    mediaPlayerInit();
    scanVideoFiles();

    pushButton[0]->setCheckable(true);
    pushButton[4]->setCheckable(true);

    connect(pushButton[0], &QPushButton::clicked, this, &VideoPlayer::btn_play_clicked);
    connect(pushButton[1], &QPushButton::clicked, this, &VideoPlayer::btn_next_clicked);
    connect(pushButton[2], &QPushButton::clicked, this, &VideoPlayer::btn_volmedown_clicked);
    connect(pushButton[3], &QPushButton::clicked, this, &VideoPlayer::btn_volmeup_clicked);
    connect(pushButton[4], &QPushButton::clicked, this, &VideoPlayer::btn_fullscreen_clicked);

    connect(listWidget, &QListWidget::itemClicked,
            this, &VideoPlayer::listWidgetClicked);

    connect(videoPlayer, &QMediaPlayer::stateChanged,
            this, &VideoPlayer::mediaPlayerStateChanged);
    connect(mediaPlaylist, &QMediaPlaylist::currentIndexChanged,
            this, &VideoPlayer::mediaPlaylistCurrentIndexChanged);
    connect(videoPlayer, &QMediaPlayer::durationChanged,
            this, &VideoPlayer::musicPlayerDurationChanged);
    connect(videoPlayer, &QMediaPlayer::positionChanged,
            this, &VideoPlayer::mediaPlayerPositionChanged);

    connect(durationSlider, &QSlider::sliderReleased,
            this, &VideoPlayer::durationSliderReleased);
    connect(volumeSlider, &QSlider::sliderReleased,
            this, &VideoPlayer::volumeSliderReleased);
}

VideoPlayer::~VideoPlayer()
{
    if (videoPlayer) {
        videoPlayer->stop();
    }
}

/* ================= Layout ================= */

void VideoPlayer::videoLayout()
{
    setGeometry(0, 0, 800, 480);

    for (int i = 0; i < 3; ++i) {
        hWidget[i] = new QWidget(this);
        hBoxLayout[i] = new QHBoxLayout;
    }

    for (int i = 0; i < 2; ++i) {
        vWidget[i] = new QWidget(this);
        vBoxLayout[i] = new QVBoxLayout;
        label[i] = new QLabel(this);
    }

    for (int i = 0; i < 5; ++i) {
        pushButton[i] = new QPushButton(this);
        pushButton[i]->setFixedSize(44, 44);
    }

    durationSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);

    listWidget = new QListWidget(this);
    videoWidget = new QVideoWidget(this);

    /* 主布局 */
    hBoxLayout[0]->addWidget(videoWidget);
    hBoxLayout[0]->addWidget(vWidget[0]);
    setLayout(hBoxLayout[0]);

    /* 左侧列表 */
    vBoxLayout[0]->addWidget(listWidget);
    vWidget[0]->setLayout(vBoxLayout[0]);

    /* 底部控制条 */
    vBoxLayout[1]->addWidget(durationSlider);
    vBoxLayout[1]->addWidget(hWidget[2]);
    vWidget[1]->setLayout(vBoxLayout[1]);
    vWidget[1]->setGeometry(0, height() - 80, width(), 80);
    vWidget[1]->raise();

    hBoxLayout[2]->addWidget(pushButton[0]);
    hBoxLayout[2]->addWidget(pushButton[1]);
    hBoxLayout[2]->addWidget(pushButton[2]);
    hBoxLayout[2]->addWidget(volumeSlider);
    hBoxLayout[2]->addWidget(pushButton[3]);
    hBoxLayout[2]->addWidget(label[0]);
    hBoxLayout[2]->addWidget(label[1]);
    hBoxLayout[2]->addStretch();
    hBoxLayout[2]->addWidget(pushButton[4]);
    hWidget[2]->setLayout(hBoxLayout[2]);

    /* 返回按钮 */
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose, this, &VideoPlayer::requestClose);
}

void VideoPlayer::resizeEvent(QResizeEvent *)
{
    vWidget[1]->setGeometry(0, height() - 80, width(), 80);
}

/* ================= Media ================= */

void VideoPlayer::mediaPlayerInit()
{
    videoPlayer = new QMediaPlayer(this);
    mediaPlaylist = new QMediaPlaylist(this);

    videoPlayer->setPlaylist(mediaPlaylist);
    videoPlayer->setVideoOutput(videoWidget);
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
    videoPlayer->setVolume(50);
}

/* ================= Scan ================= */

void VideoPlayer::scanVideoFiles()
{
    QDir dir(QCoreApplication::applicationDirPath() + "/myVideo");
    if (!dir.exists())
        return;

    QStringList filters { "*.mp4", "*.avi", "*.mkv", "*.wmv" };
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &fi : files) {
        MediaObjectInfo info;
        info.fileName = fi.fileName();
        info.filePath = fi.absoluteFilePath();

        if (mediaPlaylist->addMedia(QUrl::fromLocalFile(info.filePath))) {
            mediaObjectInfo.append(info);
            listWidget->addItem(info.fileName);
        }
    }
}

/* ================= Slots ================= */

void VideoPlayer::btn_play_clicked()
{
    videoPlayer->state() == QMediaPlayer::PlayingState
        ? videoPlayer->pause()
        : videoPlayer->play();
}

void VideoPlayer::btn_next_clicked()
{
    mediaPlaylist->next();
    videoPlayer->play();
}

void VideoPlayer::btn_volmeup_clicked()
{
    volumeSlider->setValue(volumeSlider->value() + 5);
    videoPlayer->setVolume(volumeSlider->value());
}

void VideoPlayer::btn_volmedown_clicked()
{
    volumeSlider->setValue(volumeSlider->value() - 5);
    videoPlayer->setVolume(volumeSlider->value());
}

void VideoPlayer::btn_fullscreen_clicked()
{
    vWidget[0]->setVisible(!pushButton[4]->isChecked());
}

void VideoPlayer::mediaPlayerStateChanged(QMediaPlayer::State state)
{
    pushButton[0]->setChecked(state == QMediaPlayer::PlayingState);
}

void VideoPlayer::listWidgetClicked(QListWidgetItem *item)
{
    mediaPlaylist->setCurrentIndex(listWidget->row(item));
    videoPlayer->play();
}

void VideoPlayer::mediaPlaylistCurrentIndexChanged(int index)
{
    if (index >= 0)
        listWidget->setCurrentRow(index);
}

void VideoPlayer::musicPlayerDurationChanged(qint64 duration)
{
    durationSlider->setRange(0, duration / 1000);
}

void VideoPlayer::mediaPlayerPositionChanged(qint64 position)
{
    if (!durationSlider->isSliderDown())
        durationSlider->setValue(position / 1000);
}

void VideoPlayer::durationSliderReleased()
{
    videoPlayer->setPosition(durationSlider->value() * 1000);
}

void VideoPlayer::volumeSliderReleased()
{
    videoPlayer->setVolume(volumeSlider->value());
}
