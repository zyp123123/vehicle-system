#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QWidget>

class MapServer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showPreviousPage();
    void showNextPage();

private:
    void setupUI();
    void createPages();
    void addPage(QWidget *page);   // ★ 新增：通用添加页面方法

    QStackedWidget *stackedWidget;
    QPushButton *btnPrev;
    QPushButton *btnNext;

    int currentIndex;

    MapServer *mapServer;
};

#endif // MAINWINDOW_H
