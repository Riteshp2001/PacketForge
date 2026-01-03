#ifndef PATHS_H
#define PATHS_H

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

/** 
 * @file Paths.h
 * @brief Global path definitions and versioning info.
 */

#define LOGGER_VERSION_NO              1.0
#define LOGGER_VERSION_DATE            "30/12/2025"

/**
 * @brief Dynamic root path determination.
 * Computes the parent directory of the current executable.
 */
#define ELS_FCC_ROOT_PATH [](void)->QString{ \
    QDir exePath = QDir::current(); \
    exePath.cdUp(); \
    return exePath.absolutePath(); \
}()

#define FILES_FOLDER_PATH                   ELS_FCC_ROOT_PATH + "/Files"
#define LOG_FOLDER_PATH                     FILES_FOLDER_PATH + "/Logs/"
#define STYLE_SHEET_FILE_PATH               FILES_FOLDER_PATH + "/Stylesheet/StyleSheet.qss"
#define GLOBAL_VARIABLES_FILE_PATH          FILES_FOLDER_PATH + "/InterfaceDetails.txt"
#define APPLICATION_NAME_WITH_EXT           QString(QFileInfo(QCoreApplication::applicationFilePath()).fileName())

#endif // PATHS_H
