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

    // 外部调用接口
    void fillNavigationData(const QString &city, const QString &start, const QString &end);
    QImage captureMapImage();  // 截图接口

private slots:
    // UI 交互槽函数
    void onSetCityClicked();
    void onSearch1Changed(const QString &text);
    void onSearch2Changed(const QString &text);
    void onNavButtonClicked();
    void onStartListClicked(const QModelIndex &index);
    void onEndListClicked(const QModelIndex &index);

    // WebChannel 回调槽函数
    void updateCityLabel(QString city);
    void updateAutoComplete_1(QJsonObject result);
    void updateAutoComplete_2(QJsonObject result);
    void handleRouteError(QString msg);

private:
    void setupUI();          // 界面布局初始化
    void checkRouteStatus(); // 检查导航按钮状态
    void tryAutoStartNavigation(); // 尝试自动触发导航

private:
    // UI 控件指针
    QWebEngineView *m_webView;
    QLineEdit      *m_cityEdit;
    QPushButton    *m_setCityBtn;
    QLabel         *m_cityLabel;

    QLineEdit      *m_startSearchEdit;
    QListView      *m_startListView;

    QLineEdit      *m_endSearchEdit;
    QListView      *m_endListView;

    QPushButton    *m_navBtn;

    // 逻辑成员
    myChannel          *m_channel;
    QStandardItemModel *m_startModel;
    QStandardItemModel *m_endModel;

    bool m_hasStart = false;
    bool m_hasEnd = false;
    bool m_autoStartReady1 = false; // 起点自动选择完成
    bool m_autoStartReady2 = false; // 终点自动选择完成
};

#endif // MAPPAGE_H
