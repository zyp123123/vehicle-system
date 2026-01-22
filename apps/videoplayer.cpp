#include "videoplayer.h"
#include <QCoreApplication>
#include <QFileInfoList>
#include <QDir>
#include <QFile>
#include <QPalette>
#include <QFont>
#include "tools/returnbutton.h"

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
{
    QFile file(":/style/videoplayerstyle.qss");

    /* 判断文件是否存在 */
    if (file.exists()) {
        /* 以只读的方式打开 */
        file.open(QFile::ReadOnly);
        /* 以字符串的方式保存读出的结果 */
        QString styleSheet = QLatin1String(file.readAll());
        /* 设置全局样式 */
        this->setStyleSheet(styleSheet);
        /* 关闭文件 */
        file.close();
    }

    this->setAttribute(Qt::WA_StyledBackground, true);

    /* 视频播放器布局初始化 */
    videoLayout();

    /* 媒体初始化 */
    mediaPlayerInit();

    /* 扫描本地视频 */
    scanVideoFiles();

    /* 设置按钮的属性 */
    pushButton[0]->setCheckable(true);
    pushButton[4]->setCheckable(true);

    /* 按钮连接信号槽 */
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(btn_play_clicked()));
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(btn_next_clicked()));
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(btn_volmedown_clicked()));
    connect(pushButton[3], SIGNAL(clicked()), this, SLOT(btn_volmeup_clicked()));
    connect(pushButton[4], SIGNAL(clicked()), this, SLOT(btn_fullscreen_clicked()));

    /* 列表连接信号槽 */
    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this,
            SLOT(listWidgetClicked(QListWidgetItem*)));

    /* 媒体连接信号槽 */
    connect(videoPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this,
            SLOT(mediaPlayerStateChanged(QMediaPlayer::State)));
    connect(mediaPlaylist, SIGNAL(currentIndexChanged(int)), this,
            SLOT(mediaPlaylistCurrentIndexChanged(int)));
    connect(videoPlayer, SIGNAL(durationChanged(qint64)), this,
            SLOT(musicPlayerDurationChanged(qint64)));
    connect(videoPlayer, SIGNAL(positionChanged(qint64)), this,
            SLOT(mediaPlayerPositionChanged(qint64)));

    /* slider信号槽连接 */
    connect(durationSlider, SIGNAL(sliderReleased()), this,
            SLOT(durationSliderReleased()));
    connect(volumeSlider, SIGNAL(sliderReleased()), this,
            SLOT(volumeSliderReleased()));
}

VideoPlayer::~VideoPlayer()
{
    qDebug() << "VideoPlayer destroyed";

    // 显式断开信号槽，防止槽函数在对象销毁过程中被触发
    disconnect(videoPlayer, nullptr, this, nullptr);
    disconnect(mediaPlaylist, nullptr, this, nullptr);

    // 停止播放器，断开输出
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setVideoOutput(static_cast<QVideoWidget*>(nullptr));
    }
}

void VideoPlayer::videoLayout()
{
    this->setGeometry(0, 0, 800, 480);

    QPalette pal;
    pal.setColor(QPalette::WindowText, Qt::white);

    for(int i = 0; i < 3; i ++){
        hWidget[i] = new QWidget (this);
        hWidget[i]->setAutoFillBackground(true);
        hBoxLayout[i] = new QHBoxLayout ();
    }

    for(int i = 0; i < 2; i ++){
        vWidget[i] = new QWidget (this);
        vWidget[i]->setAutoFillBackground(true);
        vBoxLayout[i] = new QVBoxLayout ();
    }

    for(int i = 0; i < 2; i ++){
        label[i] = new QLabel (this);
    }

    for(int i = 0; i < 5; i ++){
        pushButton[i] = new QPushButton (this);
        pushButton[i]->setMaximumSize(44, 44);
        pushButton[i]->setMinimumSize(44, 44);
    }

    /* 设置对象名称 */
    vWidget[0]->setObjectName("vWidget0");
    vWidget[1]->setObjectName("vWidget1");
    hWidget[1]->setObjectName("hWidget1");
    hWidget[2]->setObjectName("hWidget2");
    pushButton[0]->setObjectName("btn_play");
    pushButton[1]->setObjectName("btn_next");
    pushButton[2]->setObjectName("btn_volumedown");
    pushButton[3]->setObjectName("btn_volumeup");
    pushButton[4]->setObjectName("btn_screen");

    QFont font;
    font.setPixelSize(18);
    label[0]->setFont(font);
    label[1]->setFont(font);

    label[0]->setPalette(pal);
    label[1]->setPalette(pal);
    label[0]->setText("00:00");
    label[1]->setText("/00:00");

    durationSlider = new QSlider (Qt::Horizontal, this);
    durationSlider->setMaximumHeight(15);
    durationSlider->setObjectName("durationSlider");

    volumeSlider = new QSlider (Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setMaximumWidth(80);
    volumeSlider->setObjectName("volumeSlider");
    volumeSlider->setValue(50);

    listWidget = new QListWidget (this);
    listWidget->setObjectName("listWidget");
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    videoWidget = new QVideoWidget (this);
    videoWidget->setStyleSheet("border-image: none; background: transparent; border: none");

    /* H0布局 */
    vWidget[0]->setMinimumSize(300, 480);
    vWidget[0]->setMaximumWidth(300);
    videoWidget->setMinimumSize(500, 480);

    hBoxLayout[0]->addWidget(videoWidget);
    hBoxLayout[0]->addWidget(vWidget[0]);
    hBoxLayout[0]->setContentsMargins(0, 0, 0, 0);
    this->setLayout(hBoxLayout[0]);

    /* V0布局 */
    QSpacerItem *vSpacer0 = new QSpacerItem (0, 80, QSizePolicy::Minimum, QSizePolicy::Maximum);
    vBoxLayout[0]->addWidget(listWidget);
    vBoxLayout[0]->addSpacerItem(vSpacer0);
    vBoxLayout[0]->setContentsMargins(0, 0, 0, 0);
    vWidget[0]->setLayout(vBoxLayout[0]);

    /* V1布局 (底板控制栏) */
    hWidget[1]->setMaximumHeight(15);
    hWidget[2]->setMinimumHeight(65);
    vBoxLayout[1]->addWidget(hWidget[1]);
    vBoxLayout[1]->addWidget(hWidget[2]);
    vBoxLayout[1]->setAlignment(Qt::AlignCenter);
    vBoxLayout[1]->setContentsMargins(0, 0, 0, 0);

    vWidget[1]->setLayout(vBoxLayout[1]);
    vWidget[1]->setGeometry(0, this->height() - 80, this->width(), 80);
    vWidget[1]->raise();

    /* H1布局 */
    hBoxLayout[1]->addWidget(durationSlider);
    hBoxLayout[1]->setContentsMargins(0, 0, 0, 0);
    hWidget[1]->setLayout(hBoxLayout[1]);

    /* H2布局 */
    QSpacerItem *hSpacer0 = new QSpacerItem (300, 80, QSizePolicy::Expanding, QSizePolicy::Maximum);
    hBoxLayout[2]->addSpacing(20);
    hBoxLayout[2]->addWidget(pushButton[0]);
    hBoxLayout[2]->addSpacing(10);
    hBoxLayout[2]->addWidget(pushButton[1]);
    hBoxLayout[2]->addSpacing(10);
    hBoxLayout[2]->addWidget(pushButton[2]);
    hBoxLayout[2]->addWidget(volumeSlider);
    hBoxLayout[2]->addWidget(pushButton[3]);
    hBoxLayout[2]->addWidget(label[0]);
    hBoxLayout[2]->addWidget(label[1]);
    hBoxLayout[2]->addSpacerItem(hSpacer0);
    hBoxLayout[2]->addWidget(pushButton[4]);
    hBoxLayout[2]->addSpacing(20);
    hBoxLayout[2]->setContentsMargins(0, 0, 0 ,0);
    hBoxLayout[2]->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    hWidget[2]->setLayout(hBoxLayout[2]);

    // 返回按钮
    ReturnButton *back = new ReturnButton(this);
    back->raise();
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(back, &ReturnButton::requestClose, this, [=](){
        emit requestClose();
    });
}

void VideoPlayer::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    vWidget[1]->setGeometry(0, this->height() - 80, this->width(), 80);
}

void VideoPlayer::scanVideoFiles()
{
    QDir dir(QCoreApplication::applicationDirPath() + "/myVideo");
    if(dir.exists()){
        QStringList filter;
        filter << "*.mp4" << "*.mkv" << "*.wmv" << "*.avi";
        QFileInfoList files = dir.entryInfoList(filter, QDir::Files);
        for(int i = 0; i < files.count(); i++){
            MediaObjectInfo info;
            info.fileName = files.at(i).fileName();
            info.filePath = files.at(i).filePath();
            if(mediaPlaylist->addMedia(QUrl::fromLocalFile(info.filePath))){
                mediaObjectInfo.append(info);
                listWidget->addItem(info.fileName);
            }
        }
    }
}

void VideoPlayer::mediaPlayerInit()
{
    videoPlayer = new QMediaPlayer (this);
    mediaPlaylist = new QMediaPlaylist (this);
    mediaPlaylist->clear();
    videoPlayer->setPlaylist(mediaPlaylist);
    videoPlayer->setVideoOutput(videoWidget);
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
    videoPlayer->setVolume(50);
}

void VideoPlayer::btn_play_clicked()
{
    if (videoPlayer->state() == QMediaPlayer::PlayingState)
        videoPlayer->pause();
    else
        videoPlayer->play();
}

void VideoPlayer::btn_next_clicked()
{
    videoPlayer->stop();
    if(mediaPlaylist->mediaCount() > 0) {
        mediaPlaylist->next();
        videoPlayer->play();
    }
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
    videoPlayer->stop();
    mediaPlaylist->setCurrentIndex(listWidget->row(item));
    videoPlayer->play();
}

void VideoPlayer::mediaPlaylistCurrentIndexChanged(int index)
{
    if(index != -1)
        listWidget->setCurrentRow(index);
}

void VideoPlayer::musicPlayerDurationChanged(qint64 duration)
{
    durationSlider->setRange(0, duration/1000);
    int second = (duration / 1000) % 60;
    int minute = (duration / 1000) / 60;
    label[1]->setText(QString("/%1:%2")
                      .arg(minute, 2, 10, QChar('0'))
                      .arg(second, 2, 10, QChar('0')));
}

void VideoPlayer::mediaPlayerPositionChanged(qint64 position)
{
    if(!durationSlider->isSliderDown())
        durationSlider->setValue(position / 1000);

    int second = (position / 1000) % 60;
    int minute = (position / 1000) / 60;
    label[0]->setText(QString("%1:%2")
                      .arg(minute, 2, 10, QChar('0'))
                      .arg(second, 2, 10, QChar('0')));
}

void VideoPlayer::durationSliderReleased()
{
    videoPlayer->setPosition(durationSlider->value() * 1000);
}

void VideoPlayer::volumeSliderReleased()
{
    videoPlayer->setVolume(volumeSlider->value());
}
