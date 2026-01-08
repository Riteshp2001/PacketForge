/**
 * @file OscilloscopeWidget.h
 * @brief Real-time oscilloscope display for visualizing serial/network data as waveforms.
 * 
 * This widget provides a professional oscilloscope-like interface for visualizing
 * byte streams as analog waveforms. Supports TX/RX channel filtering, adjustable
 * timebase, and real-time plotting.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef OSCILLOSCOPEWIDGET_H
#define OSCILLOSCOPEWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QList>
#include <QMouseEvent>

namespace Ui {
class OscilloscopeWidget;
}

/**
 * @brief Custom widget for rendering the oscilloscope plot area.
 * 
 * Renders a real-time waveform with configurable timebase and grid overlay.
 * Supports dual-channel display (TX in blue, RX in green).
 */
class PlotArea : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs a PlotArea widget.
     * @param parent Parent widget
     */
    explicit PlotArea(QWidget* parent = nullptr) : QWidget(parent) {}
    
    /**
     * @brief Adds a sample to the RX channel.
     * @param val Byte value (0-255)
     */
    void addSampleRx(quint8 val) {
        m_dataRx.append(val);
        if (m_dataRx.size() > m_maxSamples) m_dataRx.removeFirst();
        m_totalSamples++;
        update();
    }
    
    /**
     * @brief Adds a sample to the TX channel.
     * @param val Byte value (0-255)
     */
    void addSampleTx(quint8 val) {
        m_dataTx.append(val);
        if (m_dataTx.size() > m_maxSamples) m_dataTx.removeFirst();
        m_totalSamples++;
        update();
    }
    
    /**
     * @brief Sets the timebase (horizontal scale factor).
     * @param tb Timebase value in pixels per sample
     */
    void setTimebase(int tb) { m_timebase = tb; update(); }
    
    /**
     * @brief Clears all data from both channels.
     */
    void clear() { 
        m_dataRx.clear(); 
        m_dataTx.clear();
        m_totalSamples = 0;
        update(); 
    }
    
    /**
     * @brief Returns total samples received.
     * @return Sample count
     */
    int sampleCount() const { return m_totalSamples; }
    
    /**
     * @brief Sets which channels to display.
     * @param rx Show RX channel
     * @param tx Show TX channel
     */
    void setChannels(bool rx, bool tx) { m_showRx = rx; m_showTx = tx; update(); }

protected:
    /**
     * @brief Paints the oscilloscope display.
     */
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(10, 10, 20));
        
        int w = width();
        int h = height();
        
        // Draw grid
        p.setPen(QPen(QColor(30, 40, 50), 1, Qt::DotLine));
        for (int x = 0; x < w; x += 50) p.drawLine(x, 0, x, h);
        for (int y = 0; y < h; y += 50) p.drawLine(0, y, w, y);
        
        // Draw center line
        p.setPen(QPen(QColor(60, 60, 80), 1, Qt::DashLine));
        p.drawLine(0, h/2, w, h/2);
        
        // Draw Y-axis labels
        p.setPen(Qt::gray);
        p.drawText(5, 15, "255");
        p.drawText(5, h/2 + 5, "128");
        p.drawText(5, h - 5, "0");
        
        p.setRenderHint(QPainter::Antialiasing);
        
        auto mapY = [h](quint8 v) -> int {
            return h - (int)((v / 255.0) * h);
        };
        
        auto drawChannel = [&](const QList<quint8>& data, const QColor& color) {
            if (data.isEmpty()) return;
            
            p.setPen(QPen(color, 2));
            int step = qMax(1, m_timebase / 10);
            QPoint lastPt;
            bool first = true;
            int x = w;
            
            for (int i = data.size() - 1; i >= 0; --i) {
                int y = mapY(data[i]);
                QPoint pt(x, y);
                if (!first) p.drawLine(lastPt, pt);
                lastPt = pt;
                first = false;
                x -= step;
                if (x < 0) break;
            }
        };
        
        if (m_showRx) drawChannel(m_dataRx, QColor(0, 255, 0));
        if (m_showTx) drawChannel(m_dataTx, QColor(80, 180, 255));
        
        // Legend
        int legendY = 20;
        if (m_showRx) {
            p.setPen(QColor(0, 255, 0));
            p.drawText(w - 70, legendY, "● RX");
            legendY += 15;
        }
        if (m_showTx) {
            p.setPen(QColor(80, 180, 255));
            p.drawText(w - 70, legendY, "● TX");
        }
    }

private:
    QList<quint8> m_dataRx;
    QList<quint8> m_dataTx;
    int m_maxSamples = 2000;
    int m_timebase = 50;
    int m_totalSamples = 0;
    bool m_showRx = true;
    bool m_showTx = false;
};

/**
 * @brief Main oscilloscope widget with controls and plot area.
 * 
 * Provides a complete oscilloscope interface for visualizing communication data.
 * Features include timebase control, channel selection, and real-time sample counting.
 */
class OscilloscopeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the OscilloscopeWidget.
     * @param parent Parent widget
     */
    explicit OscilloscopeWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor.
     */
    ~OscilloscopeWidget();

public slots:
    /**
     * @brief Adds data to the oscilloscope display.
     * @param isTx true for transmitted data, false for received
     * @param data Byte array to display
     */
    void addData(bool isTx, const QByteArray &data);

private slots:
    void on_sliderTimebase_valueChanged(int value);
    void on_chkRun_toggled(bool checked);
    void on_btnClear_clicked();
    void on_cmbChannel_currentIndexChanged(int index);

private:
    Ui::OscilloscopeWidget *ui;
    PlotArea *m_plot;
    bool m_hasData = false;
};

#endif // OSCILLOSCOPEWIDGET_H
