#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QTimer>
#include <QHash>
#include <functional>

class SlidePage;

/* -------------------------
 * 应用 ID 枚举
 * ------------------------ */
enum class AppID {
    Settings = 0,
    Monitor,
    Music,
    Video,
    Parking,
    Sentinel,
    Humidity,
    Remote,
    Alarm,
    Album,
    Weather,
    Map,
};

/* 使 QHash 支持 enum class */
inline uint qHash(AppID key, uint seed = 0)
{
    return qHash(static_cast<int>(key), seed);
}

/* -------------------------
 * 主窗口
 * ------------------------ */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTime();
    void openApp(AppID id);
    void handlePageClose();   // ★ SIGNAL 不带参数 → 必须无参

private:
    /* UI 初始化 */
    void setupStack();
    void setupHomePage();
    void setupSecondPage();

    QWidget* createAppGrid(
        const QStringList &names,
        const QStringList &icons,
        std::function<void(int)> onClick
    );

    /* 页面管理 */
    void setupFactories();
    QWidget* createPage(AppID id);
    void preparePage(AppID id, QWidget *page);
    void showPage(AppID id, QWidget *page);
    void replaceAndOpen(AppID oldId, AppID newId);
    void backToOriginPage();
    void bindClose(QWidget *w, AppID id);

    /* 页面注册模板 */
    template<typename T>
    void reg(AppID id)
    {
        factories[id] = []() { return new T; };
    }

private:
    QStackedWidget *stack = nullptr;

    QWidget *homePage = nullptr;
    QWidget *secondPage = nullptr;
    QWidget *currentOriginPage = nullptr;

    SlidePage *slide = nullptr;

    QPushButton *timeButton = nullptr;
    QPushButton *timeButton2 = nullptr;
    QTimer *timer = nullptr;

    QHash<AppID, QWidget*> pages;
    QHash<AppID, std::function<QWidget*()>> factories;
};

#endif // MAINWINDOW_H
