/**
 * @file main.cpp
 * @brief Application entry point for PacketForge.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 *
 * @description
 * PacketForge is a professional multi-protocol serial communication tool
 * built with Qt 6. It supports Serial, TCP, UDP connections with advanced
 * features like macro recording, traffic monitoring, and data visualization.
 */

#include <QApplication>
#include "macros.h"

#include "MainWindow.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(APP_VERSION);
    MainWindow w;
    w.show();

    return a.exec();
}
