#ifndef MODBUSCLIENTWIDGET_H
#define MODBUSCLIENTWIDGET_H

#include <QWidget>
#include <QModbusClient>
#include <QModbusTcpClient>
#include <QModbusRtuSerialClient>
#include <QTimer>

namespace Ui {
class ModbusClientWidget;
}

class ModbusClientWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModbusClientWidget(QWidget *parent = nullptr);
    ~ModbusClientWidget();

private slots:
    void on_btnConnect_clicked();
    void on_btnRead_clicked();
    void on_chkAutoRead_toggled(bool checked);
    void on_comboMode_currentIndexChanged(int index);
    
    void onStateChanged(QModbusDevice::State state);
    void onErrorOccurred(QModbusDevice::Error error);
    void onReadReady();

private:
    void refreshSerialPorts();

    Ui::ModbusClientWidget *ui;
    QModbusClient *m_modbusDevice;
    QTimer *m_scanTimer;
};

#endif // MODBUSCLIENTWIDGET_H
