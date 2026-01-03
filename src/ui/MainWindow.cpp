#include "MainWindow.h"
#include "ConnectionTab.h"
#include "modules/modbus/ModbusClientWidget.h"
#include "modules/traffic/TrafficMonitorWidget.h"
#include "modules/oscilloscope/OscilloscopeWidget.h"
#include "modules/visualizer/ByteVisualizerWidget.h"
#include "modules/visualizer/ByteVisualizerWidget.h"
#include "ui_MainWindow.h"
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
    , m_isDarkTheme(true)
{
    ui->setupUi(this);

    // Window Setup
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    resize(1200, 800);

    // Persistence
    QSettings settings("PacketForge", "PacketTransmitter");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }

    // Connect Global Buttons
    connect(ui->closeApp, &QPushButton::clicked, this, &MainWindow::on_closeApp_clicked);
    connect(ui->btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(ui->chkStayOnTop, &QCheckBox::clicked, this, &MainWindow::on_chkStayOnTop_clicked);

    QPushButton *btnTools = new QPushButton("Tools  ", this); // Add space for offset
    btnTools->setIcon(QIcon(":/Images/arrow_down.svg"));
    btnTools->setIconSize(QSize(10, 10)); // Smaller icon
    btnTools->setLayoutDirection(Qt::RightToLeft);
    btnTools->setCursor(Qt::PointingHandCursor);
    ui->headerLayout->insertWidget(3, btnTools);
    
    QMenu *toolsMenu = new QMenu(this);
    btnTools->setMenu(toolsMenu);
    
    // Tools -> New Terminal
    QAction *actNewTerm = toolsMenu->addAction("New Terminal");
    connect(actNewTerm, &QAction::triggered, this, &MainWindow::createNewTab);
    
    // Tools -> New Modbus
    QAction *actModbus = toolsMenu->addAction("New Modbus Client");
    connect(actModbus, &QAction::triggered, this, [this](){
        ModbusClientWidget *tab = new ModbusClientWidget(this);
        int index = ui->mainTabWidget->addTab(tab, "Modbus " + QString::number(ui->mainTabWidget->count() + 1));
        ui->mainTabWidget->setCurrentIndex(index);
    });
    
    toolsMenu->addSeparator();
    
    // Tools -> Traffic Monitor
    QAction *actTraffic = toolsMenu->addAction("Traffic Monitor");
    connect(actTraffic, &QAction::triggered, this, [this](){
        // Check if exists
        for(int i=0; i<ui->mainTabWidget->count(); i++) {
            if (qobject_cast<TrafficMonitorWidget*>(ui->mainTabWidget->widget(i))) {
                ui->mainTabWidget->setCurrentIndex(i);
                return;
            }
        }
        // Create new
        TrafficMonitorWidget *traffic = new TrafficMonitorWidget(this);
        ui->mainTabWidget->addTab(traffic, "Traffic Monitor");
        ui->mainTabWidget->setCurrentWidget(traffic);
        
        // Re-connect existing terminals? 
        // Iterate all tabs, if ConnectionTab, connect logData
       for(int i=0; i<ui->mainTabWidget->count(); i++) {
            if(ConnectionTab* tab = qobject_cast<ConnectionTab*>(ui->mainTabWidget->widget(i))) {
                 connect(tab, &ConnectionTab::logData, traffic, &TrafficMonitorWidget::appendData);
            }
        }
    });

    // Tools -> Oscilloscope
    QAction *actScope = toolsMenu->addAction("Oscilloscope");
    connect(actScope, &QAction::triggered, this, [this](){
        for(int i=0; i<ui->mainTabWidget->count(); i++) {
            if (qobject_cast<OscilloscopeWidget*>(ui->mainTabWidget->widget(i))) {
                ui->mainTabWidget->setCurrentIndex(i);
                return;
            }
        }
        OscilloscopeWidget *scope = new OscilloscopeWidget(this);
        ui->mainTabWidget->addTab(scope, "Oscilloscope");
        ui->mainTabWidget->setCurrentWidget(scope);
        
        for(int i=0; i<ui->mainTabWidget->count(); i++) {
            if(ConnectionTab* tab = qobject_cast<ConnectionTab*>(ui->mainTabWidget->widget(i))) {
                 connect(tab, &ConnectionTab::logData, scope, &OscilloscopeWidget::addData);
            }
        }
    });

    // Tools -> Visualizer
    QAction *actViz = toolsMenu->addAction("Byte Visualizer");
    connect(actViz, &QAction::triggered, this, [this](){
        for(int i=0; i<ui->mainTabWidget->count(); i++) {
            if (qobject_cast<ByteVisualizerWidget*>(ui->mainTabWidget->widget(i))) {
                ui->mainTabWidget->setCurrentIndex(i);
                return;
            }
        }
        ByteVisualizerWidget *viz = new ByteVisualizerWidget(this);
        ui->mainTabWidget->addTab(viz, "Byte Visualizer");
        ui->mainTabWidget->setCurrentWidget(viz);
        
        for(int i=0; i<ui->mainTabWidget->count(); i++) {
            if(ConnectionTab* tab = qobject_cast<ConnectionTab*>(ui->mainTabWidget->widget(i))) {
                 connect(tab, &ConnectionTab::logData, viz, &ByteVisualizerWidget::addData);
            }
        }
    });

    // --- Tab Widget Setup ---
    ui->mainTabWidget->setTabsClosable(true);
    ui->mainTabWidget->setMovable(true); // Enable Drag & Drop Reordering
    connect(ui->mainTabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(ui->mainTabWidget, &QTabWidget::tabBarDoubleClicked, this, [this](int index){
        if (index < 0) return;
        bool ok;
        QString oldName = ui->mainTabWidget->tabText(index);
        QString newName = QInputDialog::getText(this, "Rename Tab", "Enter new name:", QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.isEmpty()) {
            ui->mainTabWidget->setTabText(index, newName);
        }
    });
    
    // Icon
    QIcon appIcon(":/Images/app_icon.png");
    setWindowIcon(appIcon);

    // Initial Theme
    applyDarkTheme();

    // Create Initial Tab
    createNewTab();
    
    // Create Traffic Monitor Tab
    TrafficMonitorWidget *traffic = new TrafficMonitorWidget(this);
    ui->mainTabWidget->addTab(traffic, "Traffic Monitor");
    
    // Create Oscilloscope Tab
    OscilloscopeWidget *scope = new OscilloscopeWidget(this);
    ui->mainTabWidget->addTab(scope, "Oscilloscope");
    
    // Create Visualizer Tab
    ByteVisualizerWidget *viz = new ByteVisualizerWidget(this);
    ui->mainTabWidget->addTab(viz, "Byte Visualizer");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_closeApp_clicked()
{
    close();
}

void MainWindow::createNewTab()
{
    ConnectionTab *tab = new ConnectionTab(this);
    int index = ui->mainTabWidget->insertTab(ui->mainTabWidget->count()-1, tab, "Terminal " + QString::number(ui->mainTabWidget->count())); 
    // Insert before Traffic Monitor? Or just add.
    // If we want Traffic Monitor to be last, we use insertTab.
    // But currently addTab adds to end. If Traffic Monitor is already there, it adds after.
    // Let's keep it simple: Just addTab. If user wants to reorder, QTabWidget supports movability if enabled.
    // Reverting to addTab for simplicity, but we need to find the TrafficMonitor instance to connect signals.
    
    // Find TrafficMonitor
    TrafficMonitorWidget* monitor = nullptr;
    for(int i=0; i<ui->mainTabWidget->count(); i++) {
        monitor = qobject_cast<TrafficMonitorWidget*>(ui->mainTabWidget->widget(i));
        if(monitor) break;
    }
    
    if (monitor) {
        connect(tab, &ConnectionTab::logData, monitor, &TrafficMonitorWidget::appendData);
    }
    
    // Connect Scope and Viz
    OscilloscopeWidget* scope = nullptr;
    ByteVisualizerWidget* viz = nullptr;
    for(int i=0; i<ui->mainTabWidget->count(); i++) {
        QWidget* w = ui->mainTabWidget->widget(i);
        if(!scope) scope = qobject_cast<OscilloscopeWidget*>(w);
        if(!viz) viz = qobject_cast<ByteVisualizerWidget*>(w);
    }
    
    if (scope) connect(tab, &ConnectionTab::logData, scope, &OscilloscopeWidget::addData);
    if (viz) connect(tab, &ConnectionTab::logData, viz, &ByteVisualizerWidget::addData);
    
    ui->mainTabWidget->setCurrentIndex(index);
}

void MainWindow::on_btnNewTab_clicked()
{
    createNewTab();
}

void MainWindow::onTabCloseRequested(int index)
{
    // Don't close the last tab? Or do? Let's allow it, but if 0 left, create one?
    // For now, if > 1 close it.
    if (ui->mainTabWidget->count() > 1) {
        QWidget* widget = ui->mainTabWidget->widget(index);
        ui->mainTabWidget->removeTab(index);
        delete widget;
    }
}

void MainWindow::toggleTheme()
{
    m_isDarkTheme = !m_isDarkTheme;
    
    if (m_isDarkTheme) {
        applyDarkTheme();
    } else {
        // Light Theme (Simplified for Container)
        QString lightStyle = R"(
        QWidget {
            background-color: #f3f3f3;
            color: #333333; /* Enabled Text */
            font-family: 'Segoe UI', sans-serif;
            font-size: 10pt;
        }
        QWidget:disabled {
            color: #787878; /* Proper Dark Gray for Disabled Text in Light Mode */
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
            color: #787878; /* Unified Disabled Text */
        }
        QPushButton::menu-indicator {
            image: none;
            width: 0px;
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
            color: #787878; /* Unified Disabled Text */
        }
        
        /* Other Controls Disabled */
        QCheckBox:disabled, QRadioButton:disabled, QLabel:disabled {
            color: #787878;
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
        )";
        this->setStyleSheet(lightStyle);
    }
}

void MainWindow::applyDarkTheme()
{
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
    )";
    this->setStyleSheet(darkStyle);
}

void MainWindow::on_chkStayOnTop_clicked(bool checked)
{
    Qt::WindowFlags flags = this->windowFlags();
    if (checked) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    
    // Re-applying flags causes the window to hide/show, losing position if not careful
    // But for a Frameless window, it's tricky.
    // Often best to save geometry, set flags, restore geometry.
    QByteArray geo = saveGeometry();
    setWindowFlags(flags);
    show();
    restoreGeometry(geo);
}

// Window Dragging
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("PacketForge", "PacketTransmitter");
    settings.setValue("geometry", saveGeometry());
    QWidget::closeEvent(event);
}
