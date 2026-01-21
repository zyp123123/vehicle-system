#ifndef MAP_H
#define MAP_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

#include "maptcpclient.h"

class Map : public QWidget
{
    Q_OBJECT
public:
    explicit Map(QWidget *parent = nullptr);
    ~Map() override = default;

    void setCity(const QString &city);
    void setStart(const QString &start);
    void setEnd(const QString &end);
    void startNavigation();

signals:
    void requestClose();   // 返回主页

private slots:
    void onCityComboChanged(int index);
    void onStartComboChanged(int index);
    void onEndComboChanged(int index);

    void onStartNavClicked();

    void onTcpNavigationCommand(const QString &city,
                                const QString &start,
                                const QString &end);
    void onTcpMapImageReceived(const QImage &img);

private:
    /* UI */
    QLabel     *m_imageLabel;

    QLineEdit  *m_cityEdit;
    QLineEdit  *m_startEdit;
    QLineEdit  *m_endEdit;

    QComboBox  *m_cityCombo;
    QComboBox  *m_startCombo;
    QComboBox  *m_endCombo;

    QPushButton *m_navBtn;

    /* TCP */
    MapTcpClient *m_tcpClient;
};

#endif // MAP_H
