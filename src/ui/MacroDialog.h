#ifndef MACRODIALOG_H
#define MACRODIALOG_H

#include <QDialog>

namespace Ui {
class MacroDialog;
}

struct MacroSettings {
    QString name;
    QString data; // Hex string or Text
    bool autoSend;
    int intervalMs;
    
    // Protocol Extensions
    int packetMode; // 0=Structured, 1=Raw Hex
    QString sof;
    QString eof;
    
    MacroSettings() : name("Macro"), data(""), autoSend(false), intervalMs(1000), packetMode(1) {}
};

class MacroDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MacroDialog(const MacroSettings& currentSettings, QWidget *parent = nullptr);
    ~MacroDialog(); // Destructor needed for UI

    MacroSettings getSettings() const;

private slots:
    void onModeChanged(int index);

private:
    Ui::MacroDialog *ui;
    MacroSettings m_settings;
};

#endif // MACRODIALOG_H
