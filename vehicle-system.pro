QT += core gui
QT += multimedia multimediawidgets
QT += concurrent charts mqtt sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 弃用API警告
DEFINES += QT_DEPRECATED_WARNINGS

# 要禁用特定版本之前的弃用API，取消下面行的注释
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000  # 禁用Qt 6.0.0之前的所有弃用API

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc \
    res.qrc

# 自动区分 x86 与 ARM

contains(QT_ARCH, arm) {
    message("🔧 编译架构：ARM 平台 - 使用开发板 OpenCV 库")

    INCLUDEPATH += /opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi/usr/include/opencv4
    LIBS += -L/opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi/usr/lib \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_videoio \
        -lopencv_imgcodecs
} else {
    message("💻 编译架构：x86 平台 - 使用 Ubuntu 本地 OpenCV 库")

    INCLUDEPATH += /usr/local/include/opencv4
    LIBS += -L/usr/local/lib \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_videoio \
        -lopencv_imgcodecs
}

# 包含其他项目文件
include(apps/apps.pri)
include(tools/tools.pri)

# 编译器标志
QMAKE_CXXFLAGS += -Wno-deprecated-copy
