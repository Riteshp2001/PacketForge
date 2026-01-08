/**
 * @file Paths.h
 * @brief Global path definitions and versioning info.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 *
 * @description
 * This file contains path macros and version information for the
 * PacketForge application. Paths are dynamically resolved relative
 * to the executable location.
 */

#ifndef PATHS_H
#define PATHS_H

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

// =============================================================================
// Path Definitions
//=============================================================================

/**
 * @brief Dynamic root path determination.
 * Returns the directory containing the application executable.
 * Files folder should be at the same level as the executable.
 */
#define PACKET_FORGE_ROOT_PATH QCoreApplication::applicationDirPath()

#define FILES_FOLDER_PATH                   PACKET_FORGE_ROOT_PATH + "/Files"
#define LOG_FOLDER_PATH                     FILES_FOLDER_PATH + "/Logs/"
#define STYLE_SHEET_FILE_PATH               FILES_FOLDER_PATH + "/Stylesheet/StyleSheet.qss"
#define GLOBAL_VARIABLES_FILE_PATH          FILES_FOLDER_PATH + "/InterfaceDetails.txt"
#define APPLICATION_NAME_WITH_EXT           QString(QFileInfo(QCoreApplication::applicationFilePath()).fileName())

#endif // PATHS_H
