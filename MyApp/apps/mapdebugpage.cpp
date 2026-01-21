#include "mapdebugpage.h"
#include <QVBoxLayout>

MapDebugPage::MapDebugPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    logEdit = new QTextEdit(this);
    logEdit->setReadOnly(true);
    layout->addWidget(logEdit);
}

void MapDebugPage::appendLog(const QString &msg)
{
    logEdit->append(msg);
}
