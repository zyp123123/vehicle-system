#include "mapdebugpage.h"
#include <QVBoxLayout>

MapDebugPage::MapDebugPage(QWidget *parent)
    : QWidget(parent)
{
    // 初始化布局
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 初始化日志控件
    logEdit = new QTextEdit(this);
    logEdit->setReadOnly(true); // 设置为只读

    // 将控件加入布局
    layout->addWidget(logEdit);

    // 初始提示信息
    logEdit->append("MapDebugPage 启动成功，等待客户端连接...");
}

void MapDebugPage::appendLog(const QString &msg)
{
    // 在 UI 线程追加日志
    logEdit->append(msg);
}
