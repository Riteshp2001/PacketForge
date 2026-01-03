# ==============================================================================
# Project: PacketForge (formerly PacketTransmitter)
# Description: multi-protocol packet transmission utility
# ==============================================================================

TARGET = PacketForge
TEMPLATE = app

# --- Compiler Configuration ---
CONFIG += c++26
CONFIG -= app_bundle console

# Enable valid optimizations for Release builds
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE += -O2
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# --- Dependencies ---
QT += core gui serialport network widgets

# --- Output Directories ---
# Keep build artifacts out of the source tree
DESTDIR = $$PWD/bin
MOC_DIR = $$PWD/build/moc
OBJECTS_DIR = $$PWD/build/obj
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui

# --- Macros & Definitions ---
DEFINES += QT_DEPRECATED_WARNINGS

# --- Include Paths ---
INCLUDEPATH += $$PWD/ui \
               $$PWD/network \
               $$PWD/core \
               $$PWD/macros

DEPENDPATH += $$PWD/ui \
              $$PWD/network \
              $$PWD/core \
              $$PWD/macros

# --- Source Files ---
SOURCES += \
    main.cpp \
    ui/MainWindow.cpp \
    network/SerialQTClass.cpp \
    network/TcpClientClass.cpp \
    network/TcpServer_SingleClientClass.cpp \
    network/UdpClass.cpp \
    network/AbstractCommunicationHandlerClass.cpp \
    ui/MacroDialog.cpp

# --- Header Files ---
HEADERS += \
    ui/MainWindow.h \
    network/SerialQTClass.h \
    network/TcpClientClass.h \
    network/TcpServer_SingleClientClass.h \
    network/UdpClass.h \
    network/AbstractCommunicationHandlerClass.h \
    core/Debugger.h \
    core/Paths.h \
    macros/macros.h \
    ui/MacroDialog.h

# --- Forms & Resources ---
FORMS += \
    ui/MainWindow.ui \
    ui/MacroDialog.ui
RESOURCES += ../Files/Resources.qrc

# --- Deployment ---
# Windows specific icon (if available) - uncomment if you have an .rc file
# win32: RC_FILE += icon.rc

# Install rules
target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
