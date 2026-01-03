#ifndef TRAFFICMONITORWIDGET_H
#define TRAFFICMONITORWIDGET_H

#include <QWidget>
#include <QTime>

namespace Ui {
class TrafficMonitorWidget;
}

class TrafficMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrafficMonitorWidget(QWidget *parent = nullptr);
    ~TrafficMonitorWidget();

public slots:
    void appendData(bool isTx, const QByteArray &data);

private slots:
    void on_btnClear_clicked();
    void on_btnExportTxt_clicked();
    void on_btnExportPcap_clicked();

private:
    Ui::TrafficMonitorWidget *ui;
    
    // Struct to hold log data for export
    struct LogEntry {
        QString time;
        bool isTx;
        QByteArray data;
    };
    QList<LogEntry> m_logs;
};

#endif // TRAFFICMONITORWIDGET_H
