/**
 * @file ConnectionTab.h
 * @brief Single connection tab widget for serial/network communication.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef CONNECTIONTAB_H
#define CONNECTIONTAB_H

// Qt Core
#include <QWidget>
#include <QTimer>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>

// Qt Widgets
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

// Qt Layouts
#include <QGridLayout>

// Qt Serial
#include <QSerialPortInfo>

// Qt Events
#include <QMouseEvent>

// Project Headers
#include "Paths.h"
#include "AbstractCommunicationHandlerClass.h"
#include "SerialQTClass.h"
#include "TcpClientClass.h"
#include "TcpServer_SingleClientClass.h"
#include "UdpClass.h"
#include "macros.h"
#include "MacroDialog.h"

namespace Ui {
class ConnectionTab;
}

/**
 * @brief The MainWindow class
 * 
 * Main application window handling the UI interactions, connection logic,
 * and data display. It manages the communication handler for Serial, TCP,
 * and UDP connections.
 */
class ConnectionTab : public QWidget
{
    Q_OBJECT
    
    // Allow MainWindow to access private members for cleanup (e.g. on_btnDisconnect_clicked)
    friend class MainWindow;

public:
    /**
     * @brief Constructor
     * @param parent Parent widget (usually nullptr for top-level window)
     */
    explicit ConnectionTab(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     * Cleans up the communication handler and UI resources.
     */
    ~ConnectionTab();

signals:
    /**
     * @brief Signal to request toggling the start/stop state of the logger.
     */
    void toggleStartStopRequested();
    void logData(bool isTx, const QByteArray &data);

private slots:
    // --- UI Interaction Slots ---

    /**
     * @brief Handles connection type changes (e.g., Serial vs Network).
     * @param index New index in the combo box.
     */


    /**
     * @brief Initiates a connection based on selected settings.
     */
    void on_btnConnect_clicked();

    /**
     * @brief Disconnects the current active connection.
     */
    void on_btnDisconnect_clicked();

    /**
     * @brief Toggles the auto-send loop.
     */
    void on_chkAutoSend_toggled(bool checked);

    /**
     * @brief Closes the application.
     */


    /**
     * @brief Sends a packet manually based on current input fields.
     */
    void on_btnSend_clicked();

    /**
     * @brief Clears the receive console.
     */
    void on_btnClearRx_clicked();

    /**
     * @brief Clears the transmit input fields.
     */


    /**
     * @brief Refreshes the list of available serial ports.
     */
    void refreshSerialPorts();

    /**
     * @brief Toggles between Light and Dark application themes.
     */


    /**
     * @brief Opens a file dialog to send binary file content.
     */
    void on_btnSendFile_clicked();

    /**
     * @brief Toggles "Always on Top" window state.
     * @param checked True if enabled.
     */


    /**
     * @brief Handles clicks on dynamic macro buttons.
     * @param index Index of the macro button (1-12).
     */
    /**
     * @brief Handles clicks on dynamic macro buttons.
     * @param index Index of the macro button (1-12).
     */


    void onMacroClicked(int index);
    
    /**
     * @brief Opens configuration dialog for a macro.
     * @param index Index of the macro button.
     */
    void configureMacro(int index);

    /**
     * @brief Toggles file logging.
     * @param checked True if enabled.
     */
    void on_chkLogToFile_toggled(bool checked);

    /**
     * @brief Handles updates to the global display format (Hex, ASCII, etc).
     */


    
    // --- Auto-Send Timer ---

    /**
     * @brief Called when the auto-send timer times out.
     */
    void onAutoSendTimerTimeout();
    
    // --- Additional Protocol Slots ---

    /**
     * @brief Handles incoming data from the communication handler.
     * @param data The raw bytes received.
     */
    void onDataReceived(QByteArray data);
    
    // --- Status Logic ---

    /**
     * @brief Updates UI state upon successful connection.
     */
    void onConnected();

    /**
     * @brief Updates UI state upon disconnection.
     */
    void onDisconnected();

    /**
     * @brief Handles connection errors.
     * @param err Error code from the handler.
     */
    void onError(int err);

protected:
    // --- Persistence ---
    void closeEvent(QCloseEvent *event) override;

private:
    /**
     * @brief Sets up default UI values (ports, baud rates).
     */
    void setupUiDefaults();

    /**
     * @brief Applies the dark theme stylesheet to the application.
     */


    /**
     * @brief Sends data using the active communication handler.
     * @param data Optional data to send. If empty, reads from UI inputs.
     */
    void sendPacket(QByteArray data = QByteArray());

    /**
     * @brief Constructs packet data from UI inputs (HEX or Structured).
     * @return QByteArray of the constructed packet.
     */
    QByteArray getPacketData();

    /**
     * @brief Updates the RX/TX byte counters on the UI.
     * @param rx Received bytes count.
     * @param tx Transmitted bytes count.
     */
    void updateCounters(int rx, int tx);

    /**
     * @brief Writes data to the log file if enabled.
     * @param isTx True if transmitting, False if receiving.
     * @param data The data payload.
     */
    void writeLog(bool isTx, const QByteArray &data);


    /**
     * @brief Displays a custom-styled message box.
     * @param title Title of the dialog.
     * @param text Body text.
     * @param isError If true, shows as error icon; otherwise informational.
     */
    void showCustomMessage(const QString &title, const QString &text, bool isError = false);
    
    /**
     * @brief Adds a packet to the Traffic Log table.
     * @param isTx True if transmitting, False if receiving.
     * @param data The raw data bytes.
     */
    void addPacketToTable(bool isTx, const QByteArray &data);
    
private slots:
    void onTableDoubleClicked(int row, int column);

private:
    Ui::ConnectionTab *ui;
    AbstractCommunicationHandler* m_handler; ///< Pointer to valid communication handler (Serial/UDP/TCP)
    QTimer *m_autoSendTimer;                 ///< Timer for auto-repeating packets
    
    bool isConnected;
    bool m_isDarkTheme;                      ///< Tracks current theme state (true=Dark, false=Light)
    long long rxCount = 0;
    long long txCount = 0;
    QPoint m_dragPosition; ///< For handling window drag operations
    
    // Macro Storage
    QMap<int, MacroSettings> m_macros;
    QMap<int, QTimer*> m_macroTimers; // Timers for each macro button

    // --- Logging ---
    QFile m_logFile;
    QTextStream m_logStream;

    // --- Hex/ASCII/Binary Input ---
    QRadioButton *rbInputAscii;
    QRadioButton *rbInputHex;
    QRadioButton *rbInputBinary;
    QButtonGroup *grpInputFormat;

    // --- High Performance Mode (1ms Sending) ---
    struct BufferedPacket {
        bool isTx;
        QByteArray data;
        QString timestamp;
    };
    bool m_isHighPerformanceMode = false;    ///< True when interval < 50ms
    QByteArray m_cachedSendData;             ///< Cached packet data for high-speed repeat sending
    QVector<BufferedPacket> m_packetBuffer;  ///< Buffer for batched UI updates
    QMutex m_bufferMutex;                    ///< Protects m_packetBuffer
    QTimer *m_uiRefreshTimer;                ///< Timer for batched UI updates (every 100ms)
    QElapsedTimer m_perfTimer;               ///< For measuring send performance
    long long m_perfPacketCount = 0;

    /**
     * @brief Converts ASCII to control character mnemonics like <CR>, <LF>, etc.
     * @param data Raw binary data
     * @return QString with mnemonics for non-printable characters
     */
    QString formatAsciiWithMnemonics(const QByteArray &data);

    /**
     * @brief Flushes the packet buffer to the UI table.
     */
    void flushPacketBufferToTable();

    /**
     * @brief Converts the input text between ASCII, Hex, and Binary formats.
     * @param format 0=ASCII, 1=HEX, 2=BINARY
     */
    void convertInputFormat(int format);

    // --- Auto-Answer Triggers  ---
    struct AutoTrigger {
        QString name;           ///< User-friendly name for the trigger
        QByteArray pattern;     ///< Pattern to match in received data
        QByteArray response;    ///< Data to send when pattern is matched
        bool enabled = true;    ///< Whether this trigger is active
        bool matchHex = true;   ///< True if pattern/response are hex, false for ASCII
        int delayMs = 0;        ///< Delay before sending response (ms)
    };
    QVector<AutoTrigger> m_autoTriggers;  ///< List of auto-answer triggers
    QByteArray m_rxAccumulator;           ///< Accumulates RX data for pattern matching

    /**
     * @brief Processes received data against auto-answer triggers.
     * @param data Newly received data
     */
    void processAutoTriggers(const QByteArray &data);

    /**
     * @brief Opens the Auto-Trigger configuration dialog.
     */
    void openTriggerConfigDialog();

    /**
     * @brief Opens the Checksum Calculator dialog.
     */
    void openChecksumCalculator();

    // --- Checksum Calculator ---
    /**
     * @brief Calculates checksum for data using specified algorithm.
     * @param data Input data
     * @param algorithm Algorithm: "CRC16", "CRC32", "SUM8", "XOR", "LRC"
     * @return Calculated checksum as QByteArray
     */
    static QByteArray calculateChecksum(const QByteArray &data, const QString &algorithm);

};


#endif // CONNECTIONTAB_H

