#include "musicplayer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QPalette>
#include <QtConcurrent>

#include "tools/returnbutton.h"
#include "tools/appthreadpool.h"

MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)
{
    QFile file(":/style/musicplayerstyle.qss");
    if (file.open(QFile::ReadOnly)) {
        setStyleSheet(file.readAll());
        file.close();
    }

    setAttribute(Qt::WA_StyledBackground, true);

    musicLayout();
    mediaPlayerInit();
    scanSongsAsync();

    connect(pushButton[0], &QPushButton::clicked, this, &MusicPlayer::btn_previous_clicked);
    connect(pushButton[1], &QPushButton::clicked, this, &MusicPlayer::btn_play_clicked);
    connect(pushButton[2], &QPushButton::clicked, this, &MusicPlayer::btn_next_clicked);

    connect(musicPlayer, &QMediaPlayer::stateChanged,
            this, &MusicPlayer::mediaPlayerStateChanged);
    connect(mediaPlaylist, &QMediaPlaylist::currentIndexChanged,
            this, &MusicPlayer::mediaPlaylistCurrentIndexChanged);
    connect(musicPlayer, &QMediaPlayer::durationChanged,
            this, &MusicPlayer::musicPlayerDurationChanged);
    connect(musicPlayer, &QMediaPlayer::positionChanged,
            this, &MusicPlayer::mediaPlayerPositionChanged);

    connect(listWidget, &QListWidget::itemClicked,
            this, &MusicPlayer::listWidgetClicked);
    connect(durationSlider, &QSlider::sliderReleased,
            this, &MusicPlayer::durationSliderReleased);
}

MusicPlayer::~MusicPlayer()
{
}

/* ================= UI ================= */

void MusicPlayer::musicLayout()
{
    setGeometry(0, 0, 800, 480);

    ReturnButton *back = new ReturnButton(this);
    back->raise();
    connect(back, &ReturnButton::requestClose, this, &MusicPlayer::requestClose);

    for (int i = 0; i < 7; ++i)
        pushButton[i] = new QPushButton(this);

    for (int i = 0; i < 4; ++i)
        label[i] = new QLabel(this);

    for (int i = 0; i < 3; ++i) {
        vWidget[i] = new QWidget(this);
        vBoxLayout[i] = new QVBoxLayout;
    }

    for (int i = 0; i < 4; ++i) {
        hWidget[i] = new QWidget(this);
        hBoxLayout[i] = new QHBoxLayout;
    }

    durationSlider = new QSlider(Qt::Horizontal, this);
    durationSlider->setObjectName("durationSlider");

    listWidget = new QListWidget(this);
    listWidget->setObjectName("listWidget");

    listMask = new QWidget(listWidget);
    listMask->setObjectName("listMask");

    pushButton[1]->setCheckable(true);
}

/* ================= Media ================= */

void MusicPlayer::mediaPlayerInit()
{
    musicPlayer = new QMediaPlayer(this);
    mediaPlaylist = new QMediaPlaylist(this);

    mediaPlaylist->clear();
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
    musicPlayer->setPlaylist(mediaPlaylist);
}

/* ================= Scan Music ================= */

void MusicPlayer::scanSongsAsync()
{
    QtConcurrent::run(AppThreadPool::instance(), [this]() {
        QVector<MediaObjectInfo> result;
        QDir dir(QCoreApplication::applicationDirPath() + "/myMusic");

        if (!dir.exists())
            return;

        QStringList filters;
        filters << "*.mp3";

        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fi : files) {
            MediaObjectInfo info;
            QString name = fi.baseName();
            info.fileName = name;
            info.filePath = fi.absoluteFilePath();
            result.append(info);
        }

        QMetaObject::invokeMethod(this, [this, result]() {
            onSongsScanned(result);
        }, Qt::QueuedConnection);
    });
}

void MusicPlayer::onSongsScanned(const QVector<MediaObjectInfo> &songs)
{
    mediaPlaylist->clear();
    listWidget->clear();
    mediaObjectInfo.clear();

    for (const auto &info : songs) {
        if (mediaPlaylist->addMedia(QUrl::fromLocalFile(info.filePath))) {
            mediaObjectInfo.append(info);
            listWidget->addItem(info.fileName);
        }
    }
}

/* ================= Slots ================= */

void MusicPlayer::btn_play_clicked()
{
    if (musicPlayer->state() == QMediaPlayer::PlayingState)
        musicPlayer->pause();
    else
        musicPlayer->play();
}

void MusicPlayer::btn_next_clicked()
{
    mediaPlaylist->next();
    musicPlayer->play();
}

void MusicPlayer::btn_previous_clicked()
{
    mediaPlaylist->previous();
    musicPlayer->play();
}

void MusicPlayer::mediaPlayerStateChanged(QMediaPlayer::State state)
{
    pushButton[1]->setChecked(state == QMediaPlayer::PlayingState);
}

void MusicPlayer::listWidgetClicked(QListWidgetItem *item)
{
    mediaPlaylist->setCurrentIndex(listWidget->row(item));
    musicPlayer->play();
}

void MusicPlayer::mediaPlaylistCurrentIndexChanged(int index)
{
    if (index >= 0)
        listWidget->setCurrentRow(index);
}

void MusicPlayer::musicPlayerDurationChanged(qint64 duration)
{
    durationSlider->setRange(0, duration / 1000);
}

void MusicPlayer::mediaPlayerPositionChanged(qint64 position)
{
    if (!durationSlider->isSliderDown())
        durationSlider->setValue(position / 1000);
}

void MusicPlayer::durationSliderReleased()
{
    musicPlayer->setPosition(durationSlider->value() * 1000);
}

void MusicPlayer::resizeEvent(QResizeEvent *)
{
    if (listMask)
        listMask->setGeometry(0, listWidget->height() - 50, 310, 50);
}
