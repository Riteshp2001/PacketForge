/**
 * @file MainWindow.cpp
 * @brief Implementation of the main application window.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"

// Qt Core
#include <QDebug>
#include <QSettings>
#include <QUrl>

// Qt Widgets
#include <QAction>
#include <QComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QTabBar>

// Qt GUI
#include <QDesktopServices>
#include <QGuiApplication>
#include <QScreen>

// Project Modules
#include <ByteVisualizerWidget.h>
#include <ConnectionTab.h>
#include <ModbusClientWidget.h>
#include <OscilloscopeWidget.h>
#include <TrafficMonitorWidget.h>
#include "AutoUpdater.h"
#include <QTimer>
#include <QMessageBox>

#ifdef Q_OS_WIN
// Windows-specific includes for frameless window handling
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#ifdef _MSC_VER
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#endif
#endif

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWindow), m_isDarkTheme(true) {
  ui->setupUi(this);

  this->setMinimumSize(1024, 600);

  QSettings settings("PacketForge", "PacketTransmitter");
  if (settings.contains("geometry")) {
    restoreGeometry(settings.value("geometry").toByteArray());
  } else {
    // Center on primary screen if no saved geometry
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
      QRect screenGeometry = screen->availableGeometry();
      int x = (screenGeometry.width() - width()) / 2;
      int y = (screenGeometry.height() - height()) / 2;
      move(x, y);
    }
  }

  connect(ui->closeApp, &QPushButton::clicked, this,
          &MainWindow::on_closeApp_clicked);
  connect(ui->btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
  connect(ui->chkStayOnTop, &QCheckBox::clicked, this,
          &MainWindow::on_chkStayOnTop_clicked);

  ui->btnMinimize->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
  ui->btnMaximize->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
  ui->closeApp->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));

  // Set Theme Icon (using Menu/Settings icon)
  ui->btnTheme->setIcon(style()->standardIcon(QStyle::SP_TitleBarMenuButton));

  // Ensure all title bar controls have same height
  int titleBarHeight = ui->closeApp->height();
  ui->btnMinimize->setFixedHeight(titleBarHeight);
  ui->btnMaximize->setFixedHeight(titleBarHeight);
  ui->btnTheme->setFixedHeight(titleBarHeight);

  connect(ui->btnMinimize, &QPushButton::clicked, this,
          &MainWindow::showMinimized);
  connect(ui->btnMaximize, &QPushButton::clicked, this, [this]() {
    if (this->isMaximized()) {
      this->showNormal();
      // Async resize to allow WM to process state change
      QTimer::singleShot(10, [this]() { this->resize(1280, 720); });
      ui->btnMaximize->setIcon(
          style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    } else {
      this->showMaximized();
      ui->btnMaximize->setIcon(
          style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    }
  });

  QComboBox *cmbTools = new QComboBox(this);
  cmbTools->setObjectName("cmbTools");
  cmbTools->addItem(style()->standardIcon(QStyle::SP_DialogResetButton),
                    "Tools");
  cmbTools->addItem("New Terminal");
  cmbTools->addItem("New Modbus Client");
  cmbTools->addItem("Oscilloscope");
  cmbTools->addItem("Byte Visualizer");
  cmbTools->setCurrentIndex(0);
  cmbTools->setMinimumWidth(110);
  cmbTools->setFixedHeight(ui->closeApp->height());
  cmbTools->setCursor(Qt::PointingHandCursor);
  ui->headerLayout->insertWidget(3, cmbTools);

  connect(cmbTools, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [this, cmbTools](int index) {
            if (index == 0)
              return;

            switch (index) {
            case 1:
              createNewTab();
              break;
            case 2: {
              ModbusClientWidget *tab = new ModbusClientWidget(this);
              int idx = ui->mainTabWidget->addTab(
                  tab,
                  "Modbus " + QString::number(ui->mainTabWidget->count() + 1));
              ui->mainTabWidget->setCurrentIndex(idx);
              break;
            }
            case 3: {
              for (int i = 0; i < ui->mainTabWidget->count(); i++) {
                if (qobject_cast<OscilloscopeWidget *>(
                        ui->mainTabWidget->widget(i))) {
                  ui->mainTabWidget->setCurrentIndex(i);
                  cmbTools->setCurrentIndex(0);
                  return;
                }
              }
              OscilloscopeWidget *scope = new OscilloscopeWidget(this);
              ui->mainTabWidget->addTab(scope, "Oscilloscope");
              ui->mainTabWidget->setCurrentWidget(scope);
              for (int i = 0; i < ui->mainTabWidget->count(); i++) {
                if (ConnectionTab *tab = qobject_cast<ConnectionTab *>(
                        ui->mainTabWidget->widget(i))) {
                  connect(tab, &ConnectionTab::logData, scope,
                          &OscilloscopeWidget::addData);
                }
              }
              break;
            }
            case 4: {
              for (int i = 0; i < ui->mainTabWidget->count(); i++) {
                if (qobject_cast<ByteVisualizerWidget *>(
                        ui->mainTabWidget->widget(i))) {
                  ui->mainTabWidget->setCurrentIndex(i);
                  cmbTools->setCurrentIndex(0);
                  return;
                }
              }
              ByteVisualizerWidget *viz = new ByteVisualizerWidget(this);
              ui->mainTabWidget->addTab(viz, "Byte Visualizer");
              ui->mainTabWidget->setCurrentWidget(viz);
              for (int i = 0; i < ui->mainTabWidget->count(); i++) {
                if (ConnectionTab *tab = qobject_cast<ConnectionTab *>(
                        ui->mainTabWidget->widget(i))) {
                  connect(tab, &ConnectionTab::logData, viz,
                          &ByteVisualizerWidget::addData);
                }
              }
              break;
            }
            }
            cmbTools->setCurrentIndex(0);
          });

  ui->mainTabWidget->setTabsClosable(true);
  ui->mainTabWidget->setMovable(true);
  connect(ui->mainTabWidget, &QTabWidget::tabCloseRequested, this,
          &MainWindow::onTabCloseRequested);
  connect(ui->mainTabWidget, &QTabWidget::tabBarDoubleClicked, this,
          [this](int index) {
            if (index < 0)
              return;
            bool ok;
            QString oldName = ui->mainTabWidget->tabText(index);
            QString newName = QInputDialog::getText(
                this, "Rename Tab", "Enter new name:", QLineEdit::Normal,
                oldName, &ok);
            if (ok && !newName.isEmpty()) {
              ui->mainTabWidget->setTabText(index, newName);
            }
          });

  // Middle-click to close tab
  ui->mainTabWidget->tabBar()->installEventFilter(this);

  // Set Window Icon and Title
  QIcon appIcon(":/Images/app_icon.png");
  setWindowIcon(appIcon);
  ui->lblTitle->setText("PacketForge v" + QCoreApplication::applicationVersion());

  applyDarkTheme();

  for (int i = 0; i < 2; i++) {
    createNewTab();
  }

  // Auto-Update Check
  m_autoUpdater = new AutoUpdater(this);
  connect(m_autoUpdater, &AutoUpdater::updateAvailable, this, &MainWindow::showUpdateDialog);
  connect(m_autoUpdater, &AutoUpdater::updateReady, this, [](const QString &path){
      QDesktopServices::openUrl(QUrl::fromLocalFile(path));
      QApplication::quit();
  });
  connect(m_autoUpdater, &AutoUpdater::errorOccurred, this, [](const QString &error){
      qDebug() << "Update Error:" << error;
  });

  // Check for updates shortly after startup
  QTimer::singleShot(2000, m_autoUpdater, &AutoUpdater::checkForUpdates);
}

MainWindow::~MainWindow() { delete ui; }

/**
 * @brief Closes the application.
 */
void MainWindow::on_closeApp_clicked() { close(); }

/**
 * @brief Creates a new connection terminal tab.
 *
 * Creates a ConnectionTab and connects it to existing visualization widgets
 * (Oscilloscope, ByteVisualizer, TrafficMonitor).
 */
void MainWindow::createNewTab() {
  ConnectionTab *tab = new ConnectionTab(this);
  int index = ui->mainTabWidget->insertTab(
      ui->mainTabWidget->count() - 1, tab,
      "Terminal " + QString::number(ui->mainTabWidget->count()));

  TrafficMonitorWidget *monitor = nullptr;
  for (int i = 0; i < ui->mainTabWidget->count(); i++) {
    monitor =
        qobject_cast<TrafficMonitorWidget *>(ui->mainTabWidget->widget(i));
    if (monitor)
      break;
  }

  if (monitor) {
    connect(tab, &ConnectionTab::logData, monitor,
            &TrafficMonitorWidget::appendData);
  }

  OscilloscopeWidget *scope = nullptr;
  ByteVisualizerWidget *viz = nullptr;
  for (int i = 0; i < ui->mainTabWidget->count(); i++) {
    QWidget *w = ui->mainTabWidget->widget(i);
    if (!scope)
      scope = qobject_cast<OscilloscopeWidget *>(w);
    if (!viz)
      viz = qobject_cast<ByteVisualizerWidget *>(w);
  }

  if (scope)
    connect(tab, &ConnectionTab::logData, scope, &OscilloscopeWidget::addData);
  if (viz)
    connect(tab, &ConnectionTab::logData, viz, &ByteVisualizerWidget::addData);

  ui->mainTabWidget->setCurrentIndex(index);
}

/**
 * @brief Handler for new tab button click.
 */
void MainWindow::on_btnNewTab_clicked() { createNewTab(); }

/**
 * @brief Handles tab close request.
 * @param index Index of the tab to close
 */
void MainWindow::onTabCloseRequested(int index) {
  if (ui->mainTabWidget->count() > 1) {
    QWidget *widget = ui->mainTabWidget->widget(index);
    ui->mainTabWidget->removeTab(index);
    delete widget;
  }
}

/**
 * @brief Toggles between dark and light themes.
 */
void MainWindow::toggleTheme() {
  m_isDarkTheme = !m_isDarkTheme;

  if (m_isDarkTheme) {
    applyDarkTheme();
  } else {
    applyLightTheme();
  }
}

void MainWindow::applyLightTheme() {
  QString lightStyle = R"(
        QWidget {
            background-color: #f3f3f3;
            color: #333333;
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
        }
        QWidget:disabled {
            color: #787878;
        }

        /* Buttons */
        QPushButton {
            background-color: #e1e1e1;
            border: 1px solid #adadad;
            border-radius: 4px;
            padding: 6px 16px;
            color: #333333;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #dbeeff;
            border-color: #0078d7;
        }
        QPushButton:disabled {
            background-color: #f0f0f0;
            border: 1px solid #dcdcdc;
            color: #787878;
        }
        QPushButton::menu-indicator {
            image: none;
            width: 0px;
        }

        /* Checkbox & Radio - Transparent Background */
        QCheckBox, QRadioButton {
            background: transparent;
        }

        /* Tab Close Button - Big Orange/Red Box */
        QTabBar::close-button {
            image: url(:/Images/close.svg);
            background-color: #FF9800; /* Orange Default */
            subcontrol-position: right;
            margin: 2px;
            padding: 2px;
            width: 32px;
            height: 32px;
            border-radius: 4px;
        }
        QTabBar::close-button:hover {
            background-color: #FF0000; /* Red Hover */
        }

        /* Message Box */
        QMessageBox {
            background-color: #f3f3f3;
            color: #333333;
        }
        QMessageBox QLabel {
            color: #333333;
            background-color: transparent;
        }

        /* Group Boxes */
        QGroupBox {
            border: 1px solid #cccccc;
            border-radius: 6px;
            margin-top: 24px;
            font-weight: bold;
            background-color: transparent;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #0078d7;
            background-color: transparent;
        }

        /* Inputs */
        QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
            background-color: #ffffff;
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 5px;
            color: #333333;
            font-weight: bold;
        }
        QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QPlainTextEdit:disabled, QTextEdit:disabled {
            background-color: #f0f0f0;
            border: 1px solid #dcdcdc;
            color: #787878;
        }

        /* Checkboxes - Light Theme */
        QCheckBox {
            color: #333333;
            spacing: 5px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 2px solid #0078d7;
            border-radius: 3px;
            background-color: #ffffff;
        }
        QCheckBox::indicator:checked {
            background-color: #0078d7;
            border-color: #0078d7;
        }
        QCheckBox::indicator:hover {
            border-color: #005a9e;
        }

        /* Other Controls Disabled */
        QCheckBox:disabled, QRadioButton:disabled, QLabel:disabled {
            color: #787878;
        }
        QCheckBox:disabled::indicator {
            border-color: #cccccc;
        }
        QTabBar::tab:disabled {
            color: #d0d0d0;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 1px;
            border-left-color: #cccccc;
            border-left-style: solid;
            border-top-right-radius: 4px;
            border-bottom-right-radius: 4px;
        }
        QComboBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 12px;
            height: 12px;
        }

        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 16px;
            border-left-width: 1px;
            border-left-color: #cccccc;
            border-left-style: solid;
            border-top-right-radius: 4px;
            border-bottom-width: 1px;
            border-bottom-color: #cccccc;
            border-bottom-style: solid;
        }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 16px;
            border-left-width: 1px;
            border-left-color: #cccccc;
            border-left-style: solid;
            border-bottom-right-radius: 4px;
        }
        QSpinBox::up-arrow {
            image: url(:/Images/arrow_up.svg);
            width: 10px;
            height: 10px;
        }
        QSpinBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 10px;
            height: 10px;
        }

        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #cccccc;
            background: #f3f3f3;
        }
        QTabBar::tab {
            background: #e1e1e1;
            border: 1px solid #cccccc;
            padding: 8px 24px;
            border-bottom-color: #cccccc;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            color: #555555;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background: #f3f3f3;
            border-bottom-color: #f3f3f3;
            color: #0078d7;
            font-weight: bold;
        }

        /* Tables & Headers */
        QHeaderView::section {
            background-color: #e1e1e1;
            color: #333333;
            padding: 5px;
            border: 1px solid #cccccc;
        }
        QTableWidget {
            gridline-color: #cccccc;
            selection-background-color: #0078d7;
            selection-color: #ffffff;
        }
        QTableWidget::item {
            padding: 5px;
        }
        QScrollBar:vertical {
            border: none;
            background: #f3f3f3;
            width: 14px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #cdcdcd;
            min-height: 20px;
            border-radius: 7px;
        }
        QScrollBar::handle:vertical:hover {
            background: #a6a6a6;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* Window Controls - Light Theme */
        QPushButton#btnMinimize, QPushButton#btnMaximize, QPushButton#closeApp {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #dddddd;
            border-radius: 4px;
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
            padding: 4px; /* Ensure click area */
        }

        /* Macro Buttons Split Style */
        QPushButton[type="macro_main"] {
            border-top-right-radius: 0px;
            border-bottom-right-radius: 0px;
            border-right: 0px;
            margin-right: 0px;
        }
        QPushButton[type="macro_config"] {
            border-top-left-radius: 0px;
            border-bottom-left-radius: 0px;
            margin-left: 0px;
            border-left: 1px solid #cccccc; /* Separator */
        }
        QPushButton#btnMinimize:hover {
            background-color: rgba(0, 0, 0, 0.1);
        }
        QPushButton#btnMaximize:hover {
            background-color: #ff9a00; /* Orange */
            color: white;
        }
        QPushButton#closeApp:hover {
            background-color: #e81123;
            color: white;
        }
        )";
    this->setStyleSheet(lightStyle);
}

void MainWindow::applyDarkTheme() {
  QString darkStyle = R"(
        QWidget {
            background-color: #2b2b2b;
            color: #f0f0f0; /* Enabled Text */
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
        }
        QWidget:disabled {
            color: #808080; /* Proper Gray for Disabled Text in Dark Mode */
        }

        QPushButton {
            background-color: #444444;
            border: 1px solid #5f5f5f;
            border-radius: 4px;
            padding: 6px 16px;
            color: #ffffff;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #505050;
            border-color: #4da6ff;
        }
        QPushButton:disabled {
            background-color: #333333;
            border: 1px solid #444444;
            color: #808080; /* Unified Disabled Text */
        }
        QPushButton::menu-indicator {
            image: none;
            width: 0px;
        }

        /* Checkbox & Radio - Transparent */
        QCheckBox, QRadioButton {
            background: transparent;
        }

        /* Tab Close Button - Big Orange/Red Box */
        QTabBar::close-button {
            image: url(:/Images/close.svg);
            background-color: #FF9800; /* Orange Default */
            subcontrol-position: right;
            margin: 2px;
            padding: 2px;
            width: 32px;
            height: 32px;
            border-radius: 4px;
        }
        QTabBar::close-button:hover {
            background-color: #FF0000; /* Red Hover */
        }

        /* Message Box */
        QMessageBox {
            background-color: #2b2b2b;
            color: #f0f0f0;
        }
        QMessageBox QLabel {
            color: #f0f0f0;
            background-color: transparent;
        }

        /* Group Boxes */
        QGroupBox {
            border: 1px solid #505050;
            border-radius: 6px;
            margin-top: 24px;
            font-weight: bold;
            background-color: transparent;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: #4da6ff;
            background-color: transparent;
        }

        /* Inputs */
        QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
            background-color: #3b3b3b;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 5px;
            color: #ffffff;
            font-weight: bold;
        }
        QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QPlainTextEdit:disabled, QTextEdit:disabled {
            background-color: #2e2e2e;
            border: 1px solid #444444;
            color: #808080; /* Unified Disabled Text */
        }

        /* Other Controls Disabled */
        QCheckBox:disabled, QRadioButton:disabled, QLabel:disabled {
            color: #808080;
        }
        QTabBar::tab:disabled {
            color: #555555;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 1px;
            border-left-color: #555555;
            border-left-style: solid;
            border-top-right-radius: 4px;
            border-bottom-right-radius: 4px;
        }
        QComboBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 12px;
            height: 12px;
        }

        QSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 16px;
            border-left-width: 1px;
            border-left-color: #555555;
            border-left-style: solid;
            border-top-right-radius: 4px;
            border-bottom-width: 1px;
            border-bottom-color: #555555;
            border-bottom-style: solid;
        }
        QSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 16px;
            border-left-width: 1px;
            border-left-color: #555555;
            border-left-style: solid;
            border-bottom-right-radius: 4px;
        }
        QSpinBox::up-arrow {
            image: url(:/Images/arrow_up.svg);
            width: 10px;
            height: 10px;
        }
        QSpinBox::down-arrow {
            image: url(:/Images/arrow_down.svg);
            width: 10px;
            height: 10px;
        }

        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #444444;
            background: #2b2b2b;
        }
        QTabBar::tab {
            background: #3a3a3a;
            border: 1px solid #444444;
            padding: 8px 24px;
            border-bottom-color: #444444;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            color: #aaaaaa;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background: #2b2b2b;
            border-bottom-color: #2b2b2b;
            color: #ffffff;
            font-weight: bold;
        }

        /* Tables & Headers */
        QHeaderView::section {
            background-color: #3a3a3a;
            color: #ffffff;
            padding: 5px;
            border: 1px solid #505050;
        }
        QTableWidget {
            gridline-color: #505050;
            selection-background-color: #4da6ff;
            selection-color: #ffffff;
        }
        QTableWidget::item {
            padding: 5px;
        }
        QScrollBar:vertical {
            border: none;
            background: #2b2b2b;
            width: 14px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #505050;
            min-height: 20px;
            border-radius: 7px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* Window Controls - Dark Theme */
        QPushButton#btnMinimize, QPushButton#btnMaximize, QPushButton#closeApp {
            background-color: #ffffff;
            color: #333333; /* Force dark text/tint if possible, though icons override */
            border: none;
            border-radius: 4px;
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
            padding: 4px;
        }

        /* Macro Buttons Split Style */
        QPushButton[type="macro_main"] {
            border-top-right-radius: 0px;
            border-bottom-right-radius: 0px;
            border-right: 0px;
            margin-right: 0px;
        }
        QPushButton[type="macro_config"] {
            border-top-left-radius: 0px;
            border-bottom-left-radius: 0px;
            margin-left: 0px;
            border-left: 1px solid #5f5f5f; /* Dark Separator */
        }
        QPushButton#btnMinimize:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QPushButton#btnMaximize:hover {
            background-color: #ff9a00; /* Orange */
        }
        QPushButton#closeApp:hover {
            background-color: #e81123;
            color: white;
        }
    )";
  this->setStyleSheet(darkStyle);
}

/**
 * @brief Handles stay-on-top checkbox toggle.
 * @param checked true to keep window always on top
 */
void MainWindow::on_chkStayOnTop_clicked(bool checked) {
  Qt::WindowFlags flags = this->windowFlags();
  if (checked) {
    flags |= Qt::WindowStaysOnTopHint;
  } else {
    flags &= ~Qt::WindowStaysOnTopHint;
  }

  QByteArray geo = saveGeometry();
  setWindowFlags(flags);
  show();
  restoreGeometry(geo);
}

/**
 * @brief Handles mouse press for window dragging.
 * @param event Mouse event
 */
void MainWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QWidget *child = childAt(event->pos());
    if (qobject_cast<QPushButton *>(child) ||
        qobject_cast<QCheckBox *>(child)) {
      event->ignore();
      return;
    }
    m_dragPosition =
        event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
  }
}

/**
 * @brief Handles mouse move for window dragging.
 * @param event Mouse event
 */
void MainWindow::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    move(event->globalPosition().toPoint() - m_dragPosition);
    event->accept();
  }
}

/**
 * @brief Event filter for middle-click tab closing.
 * @param watched Watched object
 * @param event Event
 * @return true if event was handled
 */
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == ui->mainTabWidget->tabBar() &&
      event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if (mouseEvent->button() == Qt::MiddleButton) {
      int tabIndex = ui->mainTabWidget->tabBar()->tabAt(mouseEvent->pos());
      if (tabIndex >= 0 && ui->mainTabWidget->count() > 1) {
        onTabCloseRequested(tabIndex);
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

/**
 * @brief Handles application close event.
 * @param event Close event
 */
void MainWindow::closeEvent(QCloseEvent *event) {
  for (int i = 0; i < ui->mainTabWidget->count(); i++) {
    if (ConnectionTab *tab =
            qobject_cast<ConnectionTab *>(ui->mainTabWidget->widget(i))) {
      tab->on_btnDisconnect_clicked();
    }
  }

  QSettings settings("PacketForge", "PacketTransmitter");
  settings.setValue("geometry", saveGeometry());
  QWidget::closeEvent(event);
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message,
                             qintptr *result) {
  MSG *msg = static_cast<MSG *>(message);

  switch (msg->message) {
  case WM_NCCALCSIZE: {
    // Returning 0 allows the window to have proper minimize/maximize behavior
    // while still being frameless
    if (msg->wParam == TRUE) {
      *result = 0;
      return true;
    }
    break;
  }
  case WM_NCHITTEST: {
    // Allow window resizing from edges (optional but helpful)
    *result = HTCLIENT;
    const int borderWidth = 5;
    RECT winRect;
    GetWindowRect(msg->hwnd, &winRect);
    long x = GET_X_LPARAM(msg->lParam);
    long y = GET_Y_LPARAM(msg->lParam);

    // Check for resize handles at edges
    if (x >= winRect.left && x < winRect.left + borderWidth &&
        y >= winRect.top && y < winRect.top + borderWidth) {
      *result = HTTOPLEFT;
    } else if (x >= winRect.right - borderWidth && x < winRect.right &&
               y >= winRect.top && y < winRect.top + borderWidth) {
      *result = HTTOPRIGHT;
    } else if (x >= winRect.left && x < winRect.left + borderWidth &&
               y >= winRect.bottom - borderWidth && y < winRect.bottom) {
      *result = HTBOTTOMLEFT;
    } else if (x >= winRect.right - borderWidth && x < winRect.right &&
               y >= winRect.bottom - borderWidth && y < winRect.bottom) {
      *result = HTBOTTOMRIGHT;
    } else if (x >= winRect.left && x < winRect.left + borderWidth) {
      *result = HTLEFT;
    } else if (x >= winRect.right - borderWidth && x < winRect.right) {
      *result = HTRIGHT;
    } else if (y >= winRect.top && y < winRect.top + borderWidth) {
      *result = HTTOP;
    } else if (y >= winRect.bottom - borderWidth && y < winRect.bottom) {
      *result = HTBOTTOM;
    } else {
      return false; // Let Qt handle the rest
    }
    return true;
  }
  }

  return QWidget::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::showUpdateDialog(const QString &version, const QString &url, const QString &releaseNotes) {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Update Available");
    msgBox.setText(QString("A new version (%1) is available.").arg(version));
    msgBox.setInformativeText("Do you want to download and install it now?");
    // Limit release notes length for UI sanity or create a custom dialog if needed
    QString notes = releaseNotes;
    // if (notes.length() > 500) notes = notes.left(500) + "...";
    msgBox.setDetailedText(notes);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    
    // Style the messagebox to match theme if needed, but the stylesheet in MainWindow should handle it if applied globally or to children
    // The existing stylesheet targets QMessageBox so it should be fine.

    if (msgBox.exec() == QMessageBox::Yes) {
        // Show indeterminate progress or simple message
        QMessageBox::information(this, "Downloading", "Downloading update in background. The application will close and run the installer when ready.");
        m_autoUpdater->downloadUpdate(url);
    }
}
