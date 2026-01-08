# ==============================================================================
# Project: PacketForge (formerly PacketTransmitter)
# Description: multi-protocol packet transmission utility
# ==============================================================================

TARGET = PacketForge
TEMPLATE = app

# --- Compiler Configuration ---
CONFIG += c++17
CONFIG -= console

# Platform-specific app bundle settings
macx {
    CONFIG += app_bundle
    # macOS specific settings
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0
    ICON = Files/Images/app_icon.icns
    QMAKE_INFO_PLIST = Files/Info.plist
    
    # Bundle identifier
    QMAKE_TARGET_BUNDLE_PREFIX = com.packetforge
}

win32 {
    CONFIG -= app_bundle
    # Windows icon
    RC_ICONS = Files/Images/app_icon.ico
}

unix:!macx {
    CONFIG -= app_bundle
}

# Enable valid optimizations for Release builds
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE += -O2
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# --- Dependencies ---
QT += core gui serialport network widgets serialbus

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
INCLUDEPATH += $$PWD/src/ui \
               $$PWD/src/network \
               $$PWD/src/core \
               $$PWD/src/macros \
               $$PWD/src/modules/modbus \
               $$PWD/src/modules/traffic \
               $$PWD/src/modules/oscilloscope \
               $$PWD/src/modules/visualizer

DEPENDPATH += $$PWD/src/ui \
              $$PWD/src/network \
              $$PWD/src/core \
              $$PWD/src/macros

# --- Source Files ---
SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/ui/ConnectionTab.cpp \
    src/network/SerialQTClass.cpp \
    src/network/TcpClientClass.cpp \
    src/network/TcpServer_SingleClientClass.cpp \
    src/network/UdpClass.cpp \
    src/network/AbstractCommunicationHandlerClass.cpp \
    src/core/AutoUpdater.cpp \
    src/ui/MacroDialog.cpp \
    src/modules/modbus/ModbusClientWidget.cpp \
    src/modules/traffic/TrafficMonitorWidget.cpp \
    src/modules/oscilloscope/OscilloscopeWidget.cpp \
    src/modules/visualizer/ByteVisualizerWidget.cpp

# --- Header Files ---
HEADERS += \
    src/ui/MainWindow.h \
    src/ui/ConnectionTab.h \
    src/network/SerialQTClass.h \
    src/network/TcpClientClass.h \
    src/network/TcpServer_SingleClientClass.h \
    src/network/UdpClass.h \
    src/network/AbstractCommunicationHandlerClass.h \
    src/core/Debugger.h \
    src/core/Paths.h \
    src/core/AutoUpdater.h \
    src/macros/macros.h \
    src/ui/MacroDialog.h \
    src/modules/modbus/ModbusClientWidget.h \
    src/modules/traffic/TrafficMonitorWidget.h \
    src/modules/oscilloscope/OscilloscopeWidget.h \
    src/modules/visualizer/ByteVisualizerWidget.h

# --- Forms & Resources ---
FORMS += \
    src/ui/MainWindow.ui \
    src/ui/ConnectionTab.ui \
    src/ui/MacroDialog.ui \
    src/modules/modbus/ModbusClientWidget.ui \
    src/modules/traffic/TrafficMonitorWidget.ui \
    src/modules/oscilloscope/OscilloscopeWidget.ui \
    src/modules/visualizer/ByteVisualizerWidget.ui
RESOURCES += Files/Resources.qrc

# --- Deployment ---
# Install rules
target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# --- Other Files ---
OTHER_FILES += \
    scripts/deploy.bat \
    scripts/PacketForge_Installer.bat \
    scripts/PacketForge.iss \
    scripts/create_setup_legacy.bat

