#include "TrafficMonitorWidget.h"
#include "ui_TrafficMonitorWidget.h"
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>

TrafficMonitorWidget::TrafficMonitorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TrafficMonitorWidget)
{
    ui->setupUi(this);
    
    ui->tableLog->setColumnWidth(0, 100);
    ui->tableLog->setColumnWidth(1, 50);
    ui->tableLog->setColumnWidth(2, 300);
    ui->tableLog->horizontalHeader()->setStretchLastSection(true);
}

TrafficMonitorWidget::~TrafficMonitorWidget()
{
    delete ui;
}

void TrafficMonitorWidget::appendData(bool isTx, const QByteArray &data)
{
    if (!ui->chkCapture->isChecked()) return;

    QString timeStr = QTime::currentTime().toString("HH:mm:ss.zzz");
    QString dirStr = isTx ? "TX" : "RX";
    QString hexStr = data.toHex(' ').toUpper();
    QString asciiStr = QString::fromLatin1(data);
    
    // Sanitize ASCII
    for (int i = 0; i < asciiStr.length(); ++i) {
        if (asciiStr[i].unicode() < 32 || asciiStr[i].unicode() > 126) asciiStr[i] = '.';
    }

    int row = ui->tableLog->rowCount();
    ui->tableLog->insertRow(row);
    ui->tableLog->setItem(row, 0, new QTableWidgetItem(timeStr));
    ui->tableLog->setItem(row, 1, new QTableWidgetItem(dirStr));
    ui->tableLog->setItem(row, 2, new QTableWidgetItem(hexStr));
    ui->tableLog->setItem(row, 3, new QTableWidgetItem(asciiStr));
    ui->tableLog->scrollToBottom();
    
    // Store for export
    m_logs.append({timeStr, isTx, data});
}

void TrafficMonitorWidget::on_btnClear_clicked()
{
    ui->tableLog->setRowCount(0);
    m_logs.clear();
}

void TrafficMonitorWidget::on_btnExportTxt_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export TXT", "", "Text Files (*.txt)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "Time\tDir\tData(HEX)\tData(ASCII)\n";
    for (const auto &log : m_logs) {
        QString hexStr = log.data.toHex(' ').toUpper();
        QString asciiStr = QString::fromLatin1(log.data);
         for (int i = 0; i < asciiStr.length(); ++i) {
            if (asciiStr[i].unicode() < 32 || asciiStr[i].unicode() > 126) asciiStr[i] = '.';
        }
        out << log.time << "\t" << (log.isTx ? "TX" : "RX") << "\t" << hexStr << "\t" << asciiStr << "\n";
    }
}

void TrafficMonitorWidget::on_btnExportPcap_clicked()
{
    // Basic PCAP Export (LinkType 147 - USER0 or similar for raw)
    // Actually, serial capture usually uses DLT_USER0 (147) or LINUX_SLL.
    // Let's use a simple Global Header + Packet Headers.
    // Magic: A1B2C3D4, Major: 2, Minor: 4, Zone: 0, SigFigs: 0, SnapLen: 65535, Network: 147 (USER0)
    
    QString fileName = QFileDialog::getSaveFileName(this, "Export PCAP", "", "PCAP Files (*.pcap)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;

    // Global Header
    struct pcap_hdr_t {
        quint32 magic_number = 0xa1b2c3d4;
        quint16 version_major = 2;
        quint16 version_minor = 4;
        qint32  thiszone = 0;
        quint32 sigfigs = 0;
        quint32 snaplen = 65535;
        quint32 network = 147; // DLT_USER0
    } global_hdr;

    file.write(reinterpret_cast<const char*>(&global_hdr), sizeof(global_hdr));

    for (const auto &log : m_logs) {
        // Packet Header
        // ts_sec, ts_usec, incl_len, orig_len
        // DLT_USER0 payload: could include direction byte? 
        // Let's just dump raw data for now.
        
        QByteArray payload = log.data;
        
        // Let's prepend a "direction" byte: 0x00=RX, 0x01=TX
        QByteArray pcapPayload;
        pcapPayload.append(log.isTx ? (char)0x01 : (char)0x00);
        pcapPayload.append(payload);
        
        // qint64 ms = QTime::currentTime().msecsSinceStartOfDay(); // Approximate relative time
        // Better: use current time
        qint64 epoch = QDateTime::currentMSecsSinceEpoch();
        
        struct pcaprec_hdr_t {
            quint32 ts_sec;
            quint32 ts_usec;
            quint32 incl_len;
            quint32 orig_len;
        } pkt_hdr;
        
        pkt_hdr.ts_sec = epoch / 1000;
        pkt_hdr.ts_usec = (epoch % 1000) * 1000;
        pkt_hdr.incl_len = pcapPayload.size();
        pkt_hdr.orig_len = pcapPayload.size();
        
        file.write(reinterpret_cast<const char*>(&pkt_hdr), sizeof(pkt_hdr));
        file.write(pcapPayload);
    }
}
