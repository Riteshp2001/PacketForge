/**
 * @file MainWindow.h
 * @brief Main application window managing tabs and global settings.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt Core
#include <QWidget>
#include <QPoint>

// Qt GUI
#include <QMouseEvent>
#include <QCloseEvent>

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
    
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif
    bool eventFilter(QObject *watched, QEvent *event) override;


private:
    void setupUiDefaults();
    void applyDarkTheme();
    void applyLightTheme();
    void createNewTab();
    void showUpdateDialog(const QString &version, const QString &url, const QString &releaseNotes);

private:
    Ui::MainWindow *ui;
    bool m_isDarkTheme;
    QPoint m_dragPosition;
    class AutoUpdater *m_autoUpdater;
};

#endif // MAINWINDOW_H
