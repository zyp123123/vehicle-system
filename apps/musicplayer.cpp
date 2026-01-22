#include "musicplayer.h"
#include <QCoreApplication>
#include <QFileInfoList>
#include <QDir>
#include <QFile>
#include <QPalette>
#include <QImage>
#include <QPixmap>
#include <QFont>
#include <QtConcurrent/QtConcurrent>
#include "tools/returnbutton.h"
#include "tools/appthreadpool.h"

MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)
{
    QFile file(":/style/musicplayerstyle.qss");

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

    /*布局初始化*/
    musicLayout();

    /*媒体播放器初始化*/
    mediaPlayerInit();

    /*扫描歌曲 (异步)*/
    scanSongsAsync();

    /*按钮信号槽连接*/
    connect(pushButton[0], SIGNAL(clicked()), this, SLOT(btn_previous_clicked()));
    connect(pushButton[1], SIGNAL(clicked()), this, SLOT(btn_play_clicked()));
    connect(pushButton[2], SIGNAL(clicked()), this, SLOT(btn_next_clicked()));

    /*媒体信号槽连接*/
    connect(musicPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this,
            SLOT(mediaPlayerStateChanged(QMediaPlayer::State)));
    connect(mediaPlaylist, SIGNAL(currentIndexChanged(int)), this,
            SLOT(mediaPlaylistCurrentIndexChanged(int)));
    connect(musicPlayer, SIGNAL(durationChanged(qint64)), this,
            SLOT(musicPlayerDurationChanged(qint64)));
    connect(musicPlayer, SIGNAL(positionChanged(qint64)), this,
            SLOT(mediaPlayerPositionChanged(qint64)));

    /*列表信号槽连接*/
    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this,
            SLOT(listWidgetClicked(QListWidgetItem*)));

    /*slider信号槽连接*/
    connect(durationSlider, SIGNAL(sliderReleased()), this,
            SLOT(durationSliderReleased()));

    /*失去焦点*/
    this->setFocus();
}

MusicPlayer::~MusicPlayer()
{
}

void MusicPlayer::musicLayout()
{
    this->setGeometry(0, 0, 800, 480);
    QPalette pal;

    // 创建返回按钮
    ReturnButton *back = new ReturnButton(this);

    // 保证永远在最上层
    back->raise();
    back->setStyleSheet(back->styleSheet() + "z-index: 9999;");

    // 永远接收点击事件（避免被 layout 控件吞掉）
    back->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    connect(back, &ReturnButton::requestClose, this, [=](){
        emit requestClose();
    });

    /*按钮*/
    for(int i = 0; i < 7; i ++)
        pushButton[i] = new QPushButton ();

    /*标签*/
    for(int i = 0; i < 4; i ++)
        label[i] = new QLabel ();

    /*垂直容器和布局*/
    for (int i = 0; i < 3; i ++) {
        vWidget[i] = new QWidget ();
        vWidget[i]->setAttribute(Qt::WA_StyledBackground, true);
        vBoxLayout[i] = new QVBoxLayout ();
    }

    /*水平容器和布局*/
    for (int i = 0; i < 4; i ++) {
        hWidget[i] = new QWidget ();
        hWidget[i]->setAttribute(Qt::WA_StyledBackground, true);
        hBoxLayout[i] = new QHBoxLayout ();
    }

    /*播放进度条*/
    durationSlider = new QSlider (Qt::Horizontal);
    durationSlider->setMinimumSize(300, 15);
    durationSlider->setMaximumHeight(15);
    durationSlider->setObjectName("durationSlider");

    /*音乐列表*/
    listWidget = new QListWidget ();
    listWidget->setObjectName("listWidget");
    listWidget->resize(310, 265);
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*列表遮罩*/
    listMask = new QWidget (listWidget);
    listMask->setMinimumSize(310, 50);
    listMask->setMinimumHeight(50);
    listMask->setObjectName("listMask");
    listMask->setGeometry(0, listWidget->height() - 50, 310, 50);

    /*设置对象名称*/
    pushButton[0]->setObjectName("btn_previous");
    pushButton[1]->setObjectName("btn_play");
    pushButton[2]->setObjectName("btn_next");
    pushButton[3]->setObjectName("btn_favorite");
    pushButton[4]->setObjectName("btn_mode");
    pushButton[5]->setObjectName("btn_menu");
    pushButton[6]->setObjectName("btn_volume");

    /*设置按钮属性*/
    pushButton[1]->setCheckable(true);
    pushButton[3]->setCheckable(true);

    /*H0布局*/
    vWidget[0]->setMinimumSize(310, 480);
    vWidget[0]->setMaximumWidth(310);
    vWidget[1]->setMinimumSize(320, 480);
    QSpacerItem *hSpacer0 = new QSpacerItem (70, 480, QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *hSpacer1 = new QSpacerItem (65, 480, QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *hSpacer2 = new QSpacerItem (60, 480, QSizePolicy::Minimum, QSizePolicy::Maximum);

    hBoxLayout[0]->addSpacerItem(hSpacer0);
    hBoxLayout[0]->addWidget(vWidget[0]);
    hBoxLayout[0]->addSpacerItem(hSpacer1);
    hBoxLayout[0]->addWidget(vWidget[1]);
    hBoxLayout[0]->addSpacerItem(hSpacer2);
    hBoxLayout[0]->setContentsMargins(0, 0, 0, 0);

    this->setLayout(hBoxLayout[0]);

    /*V0布局*/
    listWidget->setMinimumSize(310, 265);
    hWidget[1]->setMinimumSize(310, 80);
    hWidget[1]->setMaximumHeight(80);
    label[0]->setMinimumSize(310, 95);
    label[0]->setMaximumHeight(95);
    QSpacerItem *vSpacer0 = new QSpacerItem (310, 10, QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *vSpacer1 = new QSpacerItem (310, 30, QSizePolicy::Minimum, QSizePolicy::Minimum);
    vBoxLayout[0]->addWidget(label[0]);
    vBoxLayout[0]->addWidget(listWidget);
    vBoxLayout[0]->addSpacerItem(vSpacer0);
    vBoxLayout[0]->addWidget(hWidget[1]);
    vBoxLayout[0]->addSpacerItem(vSpacer1);
    vBoxLayout[0]->setContentsMargins(0, 0, 0, 0);

    vWidget[0]->setLayout(vBoxLayout[0]);

    /*H1布局*/
    for(int i = 0; i < 3; i ++){
        pushButton[i]->setMinimumSize(80, 80);
    }
    QSpacerItem *hSpacer3 = new QSpacerItem (40, 80, QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSpacerItem *hSpacer4 = new QSpacerItem (40, 80, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hBoxLayout[1]->addWidget(pushButton[0]);
    hBoxLayout[1]->addSpacerItem(hSpacer3);
    hBoxLayout[1]->addWidget(pushButton[1]);
    hBoxLayout[1]->addSpacerItem(hSpacer4);
    hBoxLayout[1]->addWidget(pushButton[2]);
    hBoxLayout[1]->setContentsMargins(0, 0, 0, 0);

    hWidget[1]->setLayout(hBoxLayout[1]);

    /*V1布局*/
    QSpacerItem *vSpacer2 = new QSpacerItem (320, 40, QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *vSpacer3 = new QSpacerItem (320, 20, QSizePolicy::Minimum, QSizePolicy::Maximum);
    QSpacerItem *vSpacer4 = new QSpacerItem (320, 30, QSizePolicy::Minimum, QSizePolicy::Maximum);
    label[1]->setMinimumSize(320, 320);
    QImage Image;
    Image.load(":/images/musicplayerimages/cd.png");
    QPixmap pixmap = QPixmap::fromImage(Image);
    QPixmap fitpixmap = pixmap.scaled(320, 320, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    label[1]->setPixmap(fitpixmap);
    label[1]->setAlignment(Qt::AlignCenter);
    vWidget[2]->setMinimumSize(300, 80);
    vWidget[2]->setMaximumHeight(80);
    vBoxLayout[1]->addSpacerItem(vSpacer2);
    vBoxLayout[1]->addWidget(label[1]);
    vBoxLayout[1]->addSpacerItem(vSpacer3);
    vBoxLayout[1]->addWidget(durationSlider);
    vBoxLayout[1]->addWidget(vWidget[2]);
    vBoxLayout[1]->addSpacerItem(vSpacer4);
    vBoxLayout[1]->setContentsMargins(0, 0, 0, 0);

    vWidget[1]->setLayout(vBoxLayout[1]);

    /*V2布局*/
    QSpacerItem *vSpacer5 = new QSpacerItem (300, 10, QSizePolicy::Minimum, QSizePolicy::Maximum);
    hWidget[2]->setMinimumSize(320, 20);
    hWidget[3]->setMinimumSize(320, 60);
    vBoxLayout[2]->addWidget(hWidget[2]);
    vBoxLayout[2]->addSpacerItem(vSpacer5);
    vBoxLayout[2]->addWidget(hWidget[3]);
    vBoxLayout[2]->setContentsMargins(0, 0, 0, 0);

    vWidget[2]->setLayout(vBoxLayout[2]);

    /*H2布局*/
    QFont font;
    font.setPixelSize(10);
    label[0]->setText("Q Music, Enjoy it");
    label[2]->setText("00:00");
    label[3]->setText("00:00");
    label[2]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label[3]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label[3]->setAlignment(Qt::AlignRight);
    label[2]->setAlignment(Qt::AlignLeft);
    label[2]->setFont(font);
    label[3]->setFont(font);

    pal.setColor(QPalette::WindowText, Qt::white);
    label[0]->setPalette(pal);
    label[2]->setPalette(pal);
    label[3]->setPalette(pal);

    hBoxLayout[2]->addWidget(label[2]);
    hBoxLayout[2]->addWidget(label[3]);
    hBoxLayout[2]->setContentsMargins(0, 0, 0, 0);
    hWidget[2]->setLayout(hBoxLayout[2]);

    /*H3布局*/
    for(int i = 3; i < 7; i ++){
        pushButton[i]->setMinimumSize(25, 25);
        pushButton[i]->setMaximumSize(25, 25);
    }

    hBoxLayout[3]->addStretch();
    hBoxLayout[3]->addWidget(pushButton[3]);
    hBoxLayout[3]->addSpacing(80);
    hBoxLayout[3]->addWidget(pushButton[4]);
    hBoxLayout[3]->addSpacing(80);
    hBoxLayout[3]->addWidget(pushButton[5]);
    hBoxLayout[3]->addSpacing(80);
    hBoxLayout[3]->addWidget(pushButton[6]);
    hBoxLayout[3]->addStretch();
    hBoxLayout[3]->setContentsMargins(0, 0, 0, 0);
    hBoxLayout[3]->setAlignment(Qt::AlignHCenter);

    hWidget[3]->setLayout(hBoxLayout[3]);
}

void MusicPlayer::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    listMask->setGeometry(0, listWidget->height() - 50, 310, 50);
}

void MusicPlayer::scanSongsAsync()
{
    QtConcurrent::run(
        AppThreadPool::instance(),
        [this]() {
            QVector<MediaObjectInfo> result;
            QDir dir(QCoreApplication::applicationDirPath() + "/myMusic");
            if (!dir.exists())
                return;

            QStringList filter;
            filter << "*.mp3";

            QFileInfoList files = dir.entryInfoList(filter, QDir::Files);

            for (const QFileInfo &fi : files) {
                MediaObjectInfo info;
                QString fileName = QString::fromUtf8(
                    fi.fileName().replace(".mp3", "").toUtf8().data()
                );

                if (fileName.contains("-")) {
                    info.fileName = fileName + "\n" + fileName.split("-").value(1);
                } else {
                    info.fileName = fileName;
                }

                info.filePath = fi.absoluteFilePath();
                result.append(info);
            }

            // 回到 UI 线程
            QMetaObject::invokeMethod(
                this,
                [this, result]() {
                    onSongsScanned(result);
                },
                Qt::QueuedConnection
            );
        }
    );
}

void MusicPlayer::onSongsScanned(const QVector<MediaObjectInfo> &songs)
{
    mediaPlaylist->clear();
    mediaObjectInfo.clear();
    listWidget->clear();

    for (const auto &info : songs) {
        if (mediaPlaylist->addMedia(QUrl::fromLocalFile(info.filePath))) {
            mediaObjectInfo.append(info);
            listWidget->addItem(info.fileName);
        }
    }
}

void MusicPlayer::mediaPlayerInit()
{
    musicPlayer = new QMediaPlayer (this);
    mediaPlaylist = new QMediaPlaylist (this);
    mediaPlaylist->clear();
    musicPlayer->setPlaylist(mediaPlaylist);
    mediaPlaylist->setPlaybackMode(QMediaPlaylist::Loop);
}

void MusicPlayer::btn_play_clicked()
{
    int state = musicPlayer->state();
    switch (state) {
    case QMediaPlayer::StoppedState:
        musicPlayer->play();
        break;
    case QMediaPlayer::PlayingState:
        musicPlayer->pause();
        break;
    case QMediaPlayer::PausedState:
        musicPlayer->play();
        break;
    }
}

void MusicPlayer::btn_next_clicked()
{
    musicPlayer->stop();
    if(mediaPlaylist->mediaCount() == 0)
        return;
    mediaPlaylist->next();
    musicPlayer->play();
}


void MusicPlayer::btn_previous_clicked()
{
    musicPlayer->stop();
    if(mediaPlaylist->mediaCount() == 0)
        return;
    mediaPlaylist->previous();
    musicPlayer->play();
}

void MusicPlayer::mediaPlayerStateChanged(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::StoppedState:
        pushButton[1]->setChecked(false);
        break;
    case QMediaPlayer::PlayingState:
        pushButton[1]->setChecked(true);
        break;
    case QMediaPlayer::PausedState:
        pushButton[1]->setChecked(false);
        break;
    }
}

void MusicPlayer::listWidgetClicked(QListWidgetItem *item)
{
    musicPlayer->stop();
    mediaPlaylist->setCurrentIndex(listWidget->row(item));
    musicPlayer->play();
}

void MusicPlayer::mediaPlaylistCurrentIndexChanged(int index)
{
    if(index == -1)
        return;
    listWidget->setCurrentRow(index);
}

void MusicPlayer::musicPlayerDurationChanged(qint64 duration)
{
    durationSlider->setRange(0, duration / 1000);
    int second = duration / 1000;
    int minute = second / 60;
    second %= 60;

    QString mediaDuration = QString("%1:%2")
        .arg(minute, 2, 10, QChar('0'))
        .arg(second, 2, 10, QChar('0'));

    label[3]->setText(mediaDuration);
}

void MusicPlayer::mediaPlayerPositionChanged(qint64 position)
{
    if(!durationSlider->isSliderDown())
        durationSlider->setValue(position / 1000);

    int second = position / 1000;
    int minute = second / 60;
    second %= 60;

    QString mediaPosition = QString("%1:%2")
        .arg(minute, 2, 10, QChar('0'))
        .arg(second, 2, 10, QChar('0'));

    label[2]->setText(mediaPosition);
}

void MusicPlayer::durationSliderReleased()
{
    musicPlayer->setPosition(durationSlider->value() * 1000);
}
