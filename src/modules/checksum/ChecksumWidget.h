/**
 * @file ChecksumWidget.h
 * @brief Checksum calculator widget supporting multiple hash algorithms.
 *
 * This widget provides a file checksum calculator interface supporting
 * CRC-16, CRC-32, MD5, SHA-1, and SHA-256 algorithms. Features include
 * drag & drop file import, copy-to-clipboard, and batch file processing.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef CHECKSUMWIDGET_H
#define CHECKSUMWIDGET_H

#include <QWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QStyle>
#include <QPushButton>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QToolTip>
#include <QComboBox>
#include <QCryptographicHash>
#include <QLineEdit>
#include <QLabel>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTextOption>

QT_BEGIN_NAMESPACE
namespace Ui { class ChecksumWidget; }
QT_END_NAMESPACE

/**
 * @brief The ChecksumWidget class
 *
 * File checksum calculator supporting multiple algorithms.
 * Integrated as a module in PacketForge Tools menu.
 */
class ChecksumWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the ChecksumWidget.
     * @param parent Parent widget
     */
    explicit ChecksumWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ChecksumWidget();

    /**
     * @brief Calculates checksum for a file using current algorithm.
     * @param filePath Path to the file
     * @return Checksum as hex string
     */
    QString CalculateChecksum(const QString &filePath);

    /**
     * @brief Computes CRC-16 value.
     * @param message Data buffer
     * @param blk_len Buffer length
     * @return CRC-16 value
     */
    unsigned short computeCrc(unsigned char *message, qint64 blk_len);

    /**
     * @brief Supported checksum algorithms.
     */
    enum ChecksumType {
        CRC8,
        CRC16,
        CRC32,
        ADLER32,
        MD5,
        SHA1,
        SHA256,
        SHA384,
        SHA512,
        SHA3_256,
        SHA3_512
    };

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void on_btnAddFiles_clicked();
    void on_btnClear_clicked();
    void onTableItemClicked(int row, int column);
    void onTableItemDoubleClicked(int row, int column);
    void onChecksumTypeChanged(int index);

private:
    Ui::ChecksumWidget *ui;
    ChecksumType currentType;

    void setupTable();
    int findRowByPath(const QString &fullFilePath);
    void processFile(const QString &filePath);
    void updateChecksums();
    unsigned char computeCrc8(const QByteArray &data);
    unsigned int computeCrc32(const QByteArray &data);
    unsigned int computeAdler32(const QByteArray &data);
};

#endif // CHECKSUMWIDGET_H
