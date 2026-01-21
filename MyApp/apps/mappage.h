#ifndef MAPPAGE_H
#define MAPPAGE_H

#include <QWidget>
#include <QStandardItemModel>
#include <QJsonObject>

// 前向声明，减少头文件依赖
class QWebEngineView;
class QLineEdit;
class QLabel;
class QPushButton;
class QListView;
class myChannel;

class MapPage : public QWidget
{
    Q_OBJECT

public:
    explicit MapPage(QWidget *parent = nullptr);
    ~MapPage();

private slots:
    // 逻辑槽函数
    void onSetCityClicked();
    void onSearch1Changed(const QString &text);
    void onSearch2Changed(const QString &text);
    void onNavButtonClicked();

    // 来自 myChannel 的回调
    void updateCityLabel(QString city);
    void updateAutoComplete_1(QJsonObject result);
    void updateAutoComplete_2(QJsonObject result);
    void handleRouteError(QString msg);

    // 列表点击处理
    void onStartListClicked(const QModelIndex &index);
    void onEndListClicked(const QModelIndex &index);

private:
    void setupUI();                 // 纯代码布局初始化
    void checkRouteStatus();        // 检查是否可以导航

    // UI 指针
    QWebEngineView *m_webView;
    QLineEdit *m_cityEdit;
    QPushButton *m_setCityBtn;
    QLabel *m_cityLabel;
    QLineEdit *m_startSearchEdit;
    QListView *m_startListView;
    QLineEdit *m_endSearchEdit;
    QListView *m_endListView;
    QPushButton *m_navBtn;

    // 逻辑成员
    myChannel *m_channel;
    QStandardItemModel *m_startModel;
    QStandardItemModel *m_endModel;
};

#endif // MAPPAGE_H
