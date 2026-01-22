#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QTWEBENGINE_DISABLE_GPU", "1");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
                "--disable-gpu --disable-software-rasterizer");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
