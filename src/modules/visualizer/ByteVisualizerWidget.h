/**
 * @file ByteVisualizerWidget.h
 * @brief Byte visualizer widget for displaying data in multiple formats.
 * 
 * This widget provides a professional byte visualization interface showing
 * incoming data in HEX, Decimal, Binary, and ASCII formats simultaneously.
 * Features include LED-style bit indicators and a byte history display.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef BYTEVISUALIZERWIDGET_H
#define BYTEVISUALIZERWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QList>

namespace Ui {
class ByteVisualizerWidget;
}

/**
 * @brief LED panel widget displaying 8 bits as illuminated circles.
 * 
 * Renders a row of 8 LED-style indicators representing each bit of a byte.
 * Green LEDs indicate '1' bits, dim green indicates '0' bits.
 */
class LedPanel : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs a LedPanel widget.
     * @param parent Parent widget
     */
    explicit LedPanel(QWidget* parent = nullptr) : QWidget(parent), m_val(0) {}
    
    /**
     * @brief Sets the byte value to display.
     * @param val Byte value (0-255)
     */
    void setValue(quint8 val) { m_val = val; update(); }
    
protected:
    /**
     * @brief Paints the LED panel.
     */
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        int ledSize = qMin(w / 10, h - 30);
        int gap = (w - (ledSize * 8)) / 9;
        
        int y = (h - ledSize - 20) / 2;
        int x = gap;
        
        for (int i = 7; i >= 0; --i) {
            bool on = (m_val >> i) & 1;
            
            // LED glow effect
            if (on) {
                QRadialGradient glow(x + ledSize/2, y + ledSize/2, ledSize);
                glow.setColorAt(0, QColor(0, 255, 0, 200));
                glow.setColorAt(0.5, QColor(0, 200, 0, 100));
                glow.setColorAt(1, Qt::transparent);
                p.setBrush(glow);
                p.setPen(Qt::NoPen);
                p.drawEllipse(x - 5, y - 5, ledSize + 10, ledSize + 10);
            }
            
            // LED body
            QRadialGradient ledGrad(x + ledSize/3, y + ledSize/3, ledSize);
            if (on) {
                ledGrad.setColorAt(0, QColor(150, 255, 150));
                ledGrad.setColorAt(0.3, QColor(0, 255, 0));
                ledGrad.setColorAt(1, QColor(0, 180, 0));
            } else {
                ledGrad.setColorAt(0, QColor(40, 60, 40));
                ledGrad.setColorAt(1, QColor(20, 40, 20));
            }
            
            p.setBrush(ledGrad);
            p.setPen(QPen(QColor(30, 30, 30), 2));
            p.drawEllipse(x, y, ledSize, ledSize);
            
            // Bit number label
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(QRect(x, y + ledSize + 5, ledSize, 15), Qt::AlignCenter, QString("D%1").arg(i));
            
            x += ledSize + gap;
        }
    }
    
private:
    quint8 m_val;
};

/**
 * @brief Main byte visualizer widget with multi-format display.
 * 
 * Provides a comprehensive view of incoming bytes in multiple formats:
 * - Hexadecimal (0x00 - 0xFF)
 * - Decimal (0 - 255)
 * - Binary (00000000 - 11111111)
 * - ASCII character representation
 * - LED-style bit visualization
 * - Recent byte history
 */
class ByteVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the ByteVisualizerWidget.
     * @param parent Parent widget
     */
    explicit ByteVisualizerWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor.
     */
    ~ByteVisualizerWidget();

public slots:
    /**
     * @brief Sets and displays a single byte value.
     * @param byte Byte value to display
     */
    void setByte(quint8 byte);
    
    /**
     * @brief Adds data from the communication stream.
     * @param isTx true for transmitted data, false for received
     * @param data Byte array to process (last byte is displayed)
     */
    void addData(bool isTx, const QByteArray &data);

private slots:
    void on_btnClear_clicked();

private:
    Ui::ByteVisualizerWidget *ui;
    LedPanel *m_leds;
    QList<quint8> m_history;
    int m_byteCount = 0;
    bool m_hasData = false;
    
    void updateHistory(quint8 byte);
};

#endif // BYTEVISUALIZERWIDGET_H
