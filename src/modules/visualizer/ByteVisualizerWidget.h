#ifndef BYTEVISUALIZERWIDGET_H
#define BYTEVISUALIZERWIDGET_H

#include <QWidget>
#include <QPainter>

namespace Ui {
class ByteVisualizerWidget;
}

class LedPanel : public QWidget {
    Q_OBJECT
public:
    explicit LedPanel(QWidget* parent = nullptr) : QWidget(parent), m_val(0) {}
    void setValue(quint8 val) { m_val = val; update(); }
    
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int w = width();
        int h = height();
        int ledSize = qMin(w / 8, h);
        int gap = (w - (ledSize * 8)) / 9;
        
        int y = (h - ledSize) / 2;
        int x = gap;
        
        // Draw 8 bits (MSB left? usually. 7..0)
        for (int i = 7; i >= 0; --i) {
            bool on = (m_val >> i) & 1;
            
            p.setBrush(on ? Qt::green : QColor(0, 50, 0));
            p.setPen(Qt::NoPen);
            p.drawEllipse(x, y, ledSize, ledSize);
            
            // Text 
            p.setPen(Qt::white);
            p.drawText(QRect(x, y, ledSize, ledSize), Qt::AlignCenter, QString::number(i));
            
            x += ledSize + gap;
        }
    }
private:
    quint8 m_val;
};

class ByteVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ByteVisualizerWidget(QWidget *parent = nullptr);
    ~ByteVisualizerWidget();

public slots:
    void setByte(quint8 byte);
    void addData(bool isTx, const QByteArray &data); // For connecting to stream

private:
    Ui::ByteVisualizerWidget *ui;
    LedPanel *m_leds;
};

#endif // BYTEVISUALIZERWIDGET_H
