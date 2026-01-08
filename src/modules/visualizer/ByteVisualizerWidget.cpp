/**
 * @file ByteVisualizerWidget.cpp
 * @brief Implementation of the byte visualizer display widget.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "ByteVisualizerWidget.h"
#include "ui_ByteVisualizerWidget.h"

ByteVisualizerWidget::ByteVisualizerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ByteVisualizerWidget)
{
    ui->setupUi(this);
    
    m_leds = new LedPanel(this);
    ui->verticalLayout->replaceWidget(ui->ledArea, m_leds);
    delete ui->ledArea;
    
    connect(ui->btnClear, &QPushButton::clicked, this, &ByteVisualizerWidget::on_btnClear_clicked);
}

ByteVisualizerWidget::~ByteVisualizerWidget()
{
    delete ui;
}

/**
 * @brief Converts byte to printable ASCII character.
 * @param byte Input byte
 * @return Printable character or '.' for non-printable
 */
static QString byteToAscii(quint8 byte) {
    if (byte >= 32 && byte < 127) {
        return QString(QChar(byte));
    }
    
    // Common control characters
    switch (byte) {
        case 0x00: return "NUL";
        case 0x0A: return "LF";
        case 0x0D: return "CR";
        case 0x09: return "TAB";
        case 0x1B: return "ESC";
        case 0x7F: return "DEL";
        default: return ".";
    }
}

/**
 * @brief Displays a byte value in all formats.
 * @param byte Byte value to display
 */
void ByteVisualizerWidget::setByte(quint8 byte)
{
    if (ui->chkPause->isChecked()) return;
    
    if (!m_hasData) {
        m_hasData = true;
        ui->lblStatus->setText("📡 Receiving data...");
        ui->lblStatus->setStyleSheet("color: #4CAF50; padding: 5px;");
    }
    
    m_leds->setValue(byte);
    m_byteCount++;
    
    // Update displays
    ui->lblHex->setText("0x" + QString::number(byte, 16).toUpper().rightJustified(2, '0'));
    ui->lblDecimal->setText(QString::number(byte));
    ui->lblBinary->setText(QString("%1").arg(byte, 8, 2, QChar('0')));
    ui->lblAscii->setText(byteToAscii(byte));
    ui->lblByteCount->setText(QString("Bytes: %1").arg(m_byteCount));
    
    updateHistory(byte);
}

/**
 * @brief Updates the byte history display.
 * @param byte New byte to add to history
 */
void ByteVisualizerWidget::updateHistory(quint8 byte)
{
    m_history.append(byte);
    if (m_history.size() > 16) {
        m_history.removeFirst();
    }
    
    QString historyStr;
    for (int i = 0; i < m_history.size(); ++i) {
        if (i > 0) historyStr += " ";
        historyStr += QString::number(m_history[i], 16).toUpper().rightJustified(2, '0');
    }
    ui->lblHistory->setText(historyStr);
}

/**
 * @brief Handles incoming data from communication stream.
 * @param isTx true for transmitted data, false for received
 * @param data Byte array containing data
 */
void ByteVisualizerWidget::addData(bool isTx, const QByteArray &data)
{
    Q_UNUSED(isTx);
    if (data.isEmpty()) return;
    
    // Display last byte
    setByte((quint8)data.at(data.size() - 1));
}

/**
 * @brief Clears all displayed data and resets counters.
 */
void ByteVisualizerWidget::on_btnClear_clicked()
{
    m_leds->setValue(0);
    m_history.clear();
    m_byteCount = 0;
    m_hasData = false;
    
    ui->lblHex->setText("0x00");
    ui->lblDecimal->setText("0");
    ui->lblBinary->setText("00000000");
    ui->lblAscii->setText(".");
    ui->lblHistory->setText("--");
    ui->lblByteCount->setText("Bytes: 0");
    ui->lblStatus->setText("💡 Tip: Connect to a serial port or network socket in a Terminal tab. Each received byte will be displayed here in real-time.");
    ui->lblStatus->setStyleSheet("color: #888888; padding: 5px;");
}
