#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QMouseEvent>
#include "ConnectionTab.h"

namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class
 * 
 * Main container for the application. Manages multiple ConnectionTabs
 * and global application settings (Theme, StayOnTop).
 */
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Global Actions
    void on_closeApp_clicked();
    void toggleTheme();
    void on_chkStayOnTop_clicked(bool checked);

    // Tab Management
    void on_btnNewTab_clicked();
    void onTabCloseRequested(int index);

protected:
    // Window Drag
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
    // Persistence
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUiDefaults();
    void applyDarkTheme();
    void createNewTab();

private:
    Ui::MainWindow *ui;
    bool m_isDarkTheme;
    QPoint m_dragPosition;
};

#endif // MAINWINDOW_H
