#include "OscilloscopeWidget.h"
#include "ui_OscilloscopeWidget.h"

OscilloscopeWidget::OscilloscopeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OscilloscopeWidget)
{
    ui->setupUi(this);
    
    // Replace placeholder with actual PlotArea
    m_plot = new PlotArea(this);
    ui->verticalLayout->replaceWidget(ui->plotArea, m_plot);
    delete ui->plotArea; // Remove placeholder
    
    // Connect Signals
    connect(ui->sliderTimebase, &QSlider::valueChanged, this, &OscilloscopeWidget::on_sliderTimebase_valueChanged);
    connect(ui->chkRun, &QCheckBox::toggled, this, &OscilloscopeWidget::on_chkRun_toggled);
    
    m_plot->setTimebase(ui->sliderTimebase->value());
}

OscilloscopeWidget::~OscilloscopeWidget()
{
    delete ui;
}

void OscilloscopeWidget::addData(bool isTx, const QByteArray &data)
{
    Q_UNUSED(isTx);
    if (!ui->chkRun->isChecked()) return;
    
    // Add each byte
    for(char c : data) {
        m_plot->addSample((quint8)c);
    }
}

void OscilloscopeWidget::on_sliderTimebase_valueChanged(int value)
{
    m_plot->setTimebase(value);
}

void OscilloscopeWidget::on_chkRun_toggled(bool checked)
{
    Q_UNUSED(checked);
    // potentially pause updates
}
