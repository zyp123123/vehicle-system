#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include <QPushButton>
#include <QProcess>
#include <QLabel>

class Settings : public QWidget
{
    Q_OBJECT
public:
    explicit Settings(QWidget *parent = nullptr);

signals:
    void requestClose();
    void requestExitApp();

private slots:
    void onExitClicked();

    void loadDHT11();
    void unloadDHT11();

    void loadHCSR04();
    void unloadHCSR04();

    void loadSR501();
    void unloadSR501();

private:
    QLabel *dht11Status;
    QLabel *hcsr04Status;
    QLabel *sr501Status;

    QPushButton *exitBtn;

    bool runCommand(const QString &cmd, QString &output);
    bool isModuleLoaded(const QString &module);
};

#endif // SETTINGS_H
