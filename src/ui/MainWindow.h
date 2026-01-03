#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QLabel>
#include <QTimer>
#include <QSerialPortInfo>
#include <QMouseEvent>
#include <QMessageBox>

#include "Paths.h"
#include "AbstractCommunicationHandlerClass.h"
#include "SerialQTClass.h"
#include "TcpClientClass.h"
#include "TcpServer_SingleClientClass.h"
#include "UdpClass.h"
#include "macros.h"
#include "MacroDialog.h"
#include <QMap>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>

namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class
 * 
 * Main application window handling the UI interactions, connection logic,
 * and data display. It manages the communication handler for Serial, TCP,
 * and UDP connections.
 */
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget (usually nullptr for top-level window)
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     * Cleans up the communication handler and UI resources.
     */
    ~MainWindow();

signals:
    /**
     * @brief Signal to request toggling the start/stop state of the logger.
     */
    void toggleStartStopRequested();

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
    void on_closeApp_clicked();

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
    void toggleTheme();

    /**
     * @brief Opens a file dialog to send binary file content.
     */
    void on_btnSendFile_clicked();

    /**
     * @brief Toggles "Always on Top" window state.
     * @param checked True if enabled.
     */
    void on_chkStayOnTop_clicked(bool checked);

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
    // --- Frameless Window Dragging ---
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
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
    void applyDarkTheme();

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
    Ui::MainWindow *ui;
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

};

#endif // MAINWINDOW_H
