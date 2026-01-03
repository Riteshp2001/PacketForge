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
}

ByteVisualizerWidget::~ByteVisualizerWidget()
{
    delete ui;
}

void ByteVisualizerWidget::setByte(quint8 byte)
{
    if (ui->chkPause->isChecked()) return;
    
    m_leds->setValue(byte);
    ui->lblHex->setText("0x" + QString::number(byte, 16).toUpper().rightJustified(2, '0'));
}

void ByteVisualizerWidget::addData(bool isTx, const QByteArray &data)
{
    Q_UNUSED(isTx);
    if (data.isEmpty()) return;
    // Show last byte
    setByte((quint8)data.at(data.size() - 1));
}
