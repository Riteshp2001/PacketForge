/**
 * @file OscilloscopeWidget.cpp
 * @brief Implementation of the oscilloscope display widget.
 * 
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "OscilloscopeWidget.h"
#include "ui_OscilloscopeWidget.h"

OscilloscopeWidget::OscilloscopeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OscilloscopeWidget)
{
    ui->setupUi(this);
    
    m_plot = new PlotArea(this);
    ui->verticalLayout->replaceWidget(ui->plotArea, m_plot);
    delete ui->plotArea;
    
    connect(ui->sliderTimebase, &QSlider::valueChanged, this, &OscilloscopeWidget::on_sliderTimebase_valueChanged);
    connect(ui->chkRun, &QCheckBox::toggled, this, &OscilloscopeWidget::on_chkRun_toggled);
    connect(ui->btnClear, &QPushButton::clicked, this, &OscilloscopeWidget::on_btnClear_clicked);
    connect(ui->cmbChannel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OscilloscopeWidget::on_cmbChannel_currentIndexChanged);
    
    m_plot->setTimebase(ui->sliderTimebase->value());
    ui->lblTimebaseValue->setText(QString::number(ui->sliderTimebase->value()));
    
    on_cmbChannel_currentIndexChanged(0);
}

OscilloscopeWidget::~OscilloscopeWidget()
{
    delete ui;
}

/**
 * @brief Adds incoming data to the oscilloscope display.
 * @param isTx true for transmitted data, false for received data
 * @param data Raw byte data to visualize as waveform
 */
void OscilloscopeWidget::addData(bool isTx, const QByteArray &data)
{
    if (!ui->chkRun->isChecked()) return;
    
    if (!m_hasData && !data.isEmpty()) {
        m_hasData = true;
        ui->lblStatus->setText("📡 Receiving data...");
        ui->lblStatus->setStyleSheet("color: #4CAF50; padding: 5px;");
    }
    
    for (char c : data) {
        if (isTx) {
            m_plot->addSampleTx((quint8)c);
        } else {
            m_plot->addSampleRx((quint8)c);
        }
    }
    
    ui->lblSampleCount->setText(QString("Samples: %1").arg(m_plot->sampleCount()));
}

/**
 * @brief Handles timebase slider changes.
 * @param value New timebase value
 */
void OscilloscopeWidget::on_sliderTimebase_valueChanged(int value)
{
    m_plot->setTimebase(value);
    ui->lblTimebaseValue->setText(QString::number(value));
}

/**
 * @brief Handles Run checkbox toggle.
 * @param checked true to start capturing, false to pause
 */
void OscilloscopeWidget::on_chkRun_toggled(bool checked)
{
    ui->chkRun->setText(checked ? "▶ Run" : "⏸ Paused");
}

/**
 * @brief Clears all data from the oscilloscope.
 */
void OscilloscopeWidget::on_btnClear_clicked()
{
    m_plot->clear();
    ui->lblSampleCount->setText("Samples: 0");
    m_hasData = false;
    ui->lblStatus->setText("💡 Tip: Connect to a serial port or network socket in a Terminal tab, then data will appear here automatically.");
    ui->lblStatus->setStyleSheet("color: #888888; padding: 5px;");
}

/**
 * @brief Handles channel selection changes.
 * @param index 0=RX only, 1=TX only, 2=Both
 */
void OscilloscopeWidget::on_cmbChannel_currentIndexChanged(int index)
{
    switch (index) {
        case 0: m_plot->setChannels(true, false); break;
        case 1: m_plot->setChannels(false, true); break;
        case 2: m_plot->setChannels(true, true); break;
    }
}
