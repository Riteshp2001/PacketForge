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

class PlotArea : public QWidget {
    Q_OBJECT
public:
    explicit PlotArea(QWidget* parent = nullptr) : QWidget(parent) {}
    void addSample(quint8 val) {
        data.append(val);
        if (data.size() > maxSamples) data.removeFirst();
        update();
    }
    void setTimebase(int tb) { timebase = tb; update(); }
    void clear() { data.clear(); update(); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);
        
        // Grid
        p.setPen(QPen(QColor(40, 40, 40), 1, Qt::DotLine));
        for(int x=0; x<width(); x+=50) p.drawLine(x, 0, x, height());
        for(int y=0; y<height(); y+=50) p.drawLine(0, y, width(), y);

        if (data.isEmpty()) return;

        // Plot
        p.setPen(QPen(QColor(0, 255, 0), 2));
        p.setRenderHint(QPainter::Antialiasing);

        int w = width();
        int h = height();
        
        // Map 0-255 to h-0
        auto mapY = [h](quint8 v) -> int {
            return h - (int)((v / 255.0) * h);
        };

        // Draw line
        int step = qMax(1, timebase / 10); // px per sample? 
        
        QPoint lastPt;
        bool first = true;
        
        // We draw from right to left
        int x = w;
        for (int i = data.size() - 1; i >= 0; --i) {
            int y = mapY(data[i]);
            QPoint pt(x, y);
            
            if (!first) {
                p.drawLine(lastPt, pt);
            }
            lastPt = pt;
            first = false;
            
            x -= step;
            if (x < 0) break;
        }
    }

private:
    QList<quint8> data;
    int maxSamples = 2000;
    int timebase = 50; 
};

class OscilloscopeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OscilloscopeWidget(QWidget *parent = nullptr);
    ~OscilloscopeWidget();

public slots:
    void addData(bool isTx, const QByteArray &data);

private slots:
    void on_sliderTimebase_valueChanged(int value);
    void on_chkRun_toggled(bool checked);

private:
    Ui::OscilloscopeWidget *ui;
    PlotArea *m_plot;
};

#endif // OSCILLOSCOPEWIDGET_H
