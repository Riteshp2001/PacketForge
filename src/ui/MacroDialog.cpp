#include "MacroDialog.h"
#include "MacroDialog.h"
#include "ui_MacroDialog.h"
#include "macros.h"

MacroDialog::MacroDialog(const MacroSettings& currentSettings, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MacroDialog)
    , m_settings(currentSettings)
{
    ui->setupUi(this);

    // Load Settings
    ui->lineEdit_Name->setText(m_settings.name);
    ui->comboBox_Mode->setCurrentIndex(m_settings.packetMode);
    
    ui->lineEdit_Sof->setText(m_settings.sof);
    ui->lineEdit_Eof->setText(m_settings.eof);
    
    ui->lineEdit_Data->setText(m_settings.data);
    
    ui->checkBox_AutoSend->setChecked(m_settings.autoSend);
    ui->spinBox_Interval->setValue(m_settings.intervalMs);
    ui->spinBox_Interval->setEnabled(m_settings.autoSend);
    ui->spinBox_Interval->setRange(OFFSET_ONE, MAX_INTERVAL_MS);

    // Logic
    connect(ui->comboBox_Mode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MacroDialog::onModeChanged);
            
    connect(ui->checkBox_AutoSend, &QCheckBox::toggled, ui->spinBox_Interval, &QWidget::setEnabled);

    // Trigger initial state
    onModeChanged(ui->comboBox_Mode->currentIndex());
}

MacroDialog::~MacroDialog()
{
    delete ui;
}

MacroSettings MacroDialog::getSettings() const
{
    MacroSettings s;
    s.name = ui->lineEdit_Name->text();
    s.data = ui->lineEdit_Data->text();
    s.autoSend = ui->checkBox_AutoSend->isChecked();
    s.intervalMs = ui->spinBox_Interval->value();
    
    s.packetMode = ui->comboBox_Mode->currentIndex();
    s.sof = ui->lineEdit_Sof->text();
    s.eof = ui->lineEdit_Eof->text();
    
    return s;
}

void MacroDialog::onModeChanged(int index)
{
    // 0=Structured, 1=Raw Hex
    bool isStructured = (index == 0);
    ui->lineEdit_Sof->setEnabled(isStructured);
    ui->lineEdit_Eof->setEnabled(isStructured);
    
    if (isStructured) {
        ui->lineEdit_Sof->setPlaceholderText("SOF (Hex)");
        ui->lineEdit_Eof->setPlaceholderText("EOF (Hex)");
    } else {
        ui->lineEdit_Sof->setPlaceholderText("N/A");
        ui->lineEdit_Eof->setPlaceholderText("N/A");
    }
}
