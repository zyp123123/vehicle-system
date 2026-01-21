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
    void appendLog(const QString &msg);

private:
    QTextEdit *logEdit;
};

#endif // MAPDEBUGPAGE_H
