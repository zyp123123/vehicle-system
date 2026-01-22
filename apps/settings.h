#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include <QPushButton>
#include <QProcess>

class Settings : public QWidget
{
    Q_OBJECT
public:
    explicit Settings(QWidget *parent = nullptr);

signals:
    void requestClose();
    void requestExitApp();

private slots:
    void onBackClicked();
    void onExitClicked();

    void loadDHT11();
    void unloadDHT11();

    void loadHCSR04();
    void unloadHCSR04();

    void loadSR501();
    void unloadSR501();

private:
    QPushButton *backBtn;
    QPushButton *exitBtn;

    QPushButton *btnLoadDHT11;
    QPushButton *btnUnloadDHT11;

    QPushButton *btnLoadHCSR04;
    QPushButton *btnUnloadHCSR04;

    QPushButton *btnLoadSR501;
    QPushButton *btnUnloadSR501;

    bool runCommand(const QString &cmd, QString &output);
};

#endif // SETTINGS_H
