#ifndef MAPDEBUGPAGE_H
#define MAPDEBUGPAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QString>

class MapDebugPage : public QWidget
{
    Q_OBJECT
public:
    explicit MapDebugPage(QWidget *parent = nullptr);

public slots:
    /**
     * @brief 向文本框追加一条日志信息
     * @param msg 日志内容
     */
    void appendLog(const QString &msg);

private:
    QTextEdit *logEdit; // 用于显示日志的只读文本框
};

#endif // MAPDEBUGPAGE_H
