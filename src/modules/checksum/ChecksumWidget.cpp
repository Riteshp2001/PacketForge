/**
 * @file ChecksumWidget.cpp
 * @brief Implementation of the checksum calculator widget.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#include "ChecksumWidget.h"
#include "ui_ChecksumWidget.h"

/**
 * @brief Constructor for ChecksumWidget class.
 * @param parent Parent widget
 */
ChecksumWidget::ChecksumWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChecksumWidget)
{
    ui->setupUi(this);

    setAcceptDrops(true);

    setupTable();

    connect(ui->tableWidget, &QTableWidget::cellClicked,
            this, &ChecksumWidget::onTableItemClicked);

    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked,
            this, &ChecksumWidget::onTableItemDoubleClicked);

    connect(ui->checksumCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChecksumWidget::onChecksumTypeChanged);

    currentType = CRC8; // Default
}

/**
 * @brief Destructor for ChecksumWidget class.
 */
ChecksumWidget::~ChecksumWidget()
{
    delete ui;
}

/**
 * @brief Sets up the table widget with proper column configuration.
 */
void ChecksumWidget::setupTable()
{
    ui->tableWidget->setColumnCount(4);

    QStringList headers;
    headers << "Filename" << "Checksum" << "Source Path" << "";
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    ui->tableWidget->setColumnWidth(0, 250);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);

    ui->tableWidget->setColumnWidth(1, 200);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    ui->tableWidget->setColumnWidth(3, 90);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(60);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setFrameShape(QFrame::NoFrame);
    ui->tableWidget->setShowGrid(false);
}

/**
 * @brief Handles checksum type change event.
 * @param index New combo box index
 */
void ChecksumWidget::onChecksumTypeChanged(int index)
{
    switch(index) {
        case 0:
            currentType = CRC8;
            break;
        case 1:
            currentType = CRC16;
            break;
        case 2:
            currentType = CRC32;
            break;
        case 3:
            currentType = ADLER32;
            break;
        case 4:
            currentType = MD5;
            break;
        case 5:
            currentType = SHA1;
            break;
        case 6:
            currentType = SHA256;
            break;
        case 7:
            currentType = SHA384;
            break;
        case 8:
            currentType = SHA512;
            break;
        case 9:
            currentType = SHA3_256;
            break;
        case 10:
            currentType = SHA3_512;
            break;
        default:
            currentType = CRC8;
            break;
    }

    updateChecksums();
}

/**
 * @brief Updates checksums for all files in the table.
 */
void ChecksumWidget::updateChecksums()
{
    for(int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidget->item(row, 0);
        if (item) {
            QString filePath = item->data(Qt::UserRole).toString();
            QString newChecksum = CalculateChecksum(filePath);

            QTableWidgetItem *checksumItem = ui->tableWidget->item(row, 1);
            if (checksumItem) {
                checksumItem->setText(newChecksum);
            }
        }
    }
}

/**
 * @brief Handles drag enter event.
 * @param event Drag enter event
 */
void ChecksumWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

/**
 * @brief Handles drop event.
 * @param event Drop event
 */
void ChecksumWidget::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString localPath = url.toLocalFile();
        if (!localPath.isEmpty()) {
            processFile(localPath);
        }
    }
    event->acceptProposedAction();
}

/**
 * @brief Handles add files button click event.
 */
void ChecksumWidget::on_btnAddFiles_clicked()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        tr("Select Files"),
        "",
        tr("All Files (*.*)")
        );

    if (fileNames.isEmpty()) return;

    for (const QString &fileName : qAsConst(fileNames)) {
        processFile(fileName);
    }
}

/**
 * @brief Handles table item click event (copies checksum to clipboard).
 * @param row Row index
 * @param column Column index
 */
void ChecksumWidget::onTableItemClicked(int row, int column)
{
    if (column == 1) {
        QTableWidgetItem *item = ui->tableWidget->item(row, column);
        if (item) {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(item->text().prepend("0x"));
            QToolTip::showText(QCursor::pos(), "Copied to Clipboard!", this);
        }
    }
}

/**
 * @brief Handles table item double-click event (shows full checksum in popup).
 * @param row Row index
 * @param column Column index
 */
void ChecksumWidget::onTableItemDoubleClicked(int row, int column)
{
    if (column == 1) {
        QTableWidgetItem *item = ui->tableWidget->item(row, column);
        QTableWidgetItem *nameItem = ui->tableWidget->item(row, 0);
        if (item && nameItem) {
            QString checksum = item->text();
            QString filename = nameItem->text();
            
            QDialog dialog(this);
            dialog.setWindowTitle("Full Checksum - " + filename);
            dialog.setMinimumWidth(500);
            
            QVBoxLayout *layout = new QVBoxLayout(&dialog);
            layout->setSpacing(15);
            layout->setContentsMargins(20, 20, 20, 20);
            
            QLabel *titleLabel = new QLabel("Checksum Value:");
            titleLabel->setStyleSheet("font-weight: bold; font-size: 12pt;");
            layout->addWidget(titleLabel);
            
            QTextEdit *checksumText = new QTextEdit();
            checksumText->setPlainText("0x" + checksum);
            checksumText->setReadOnly(true);
            checksumText->setWordWrapMode(QTextOption::WrapAnywhere);
            checksumText->setStyleSheet(
                "QTextEdit { "
                "  font-family: 'Consolas', 'Courier New', monospace; "
                "  font-size: 14pt; "
                "  font-weight: bold; "
                "  color: #0067c0; "
                "  padding: 10px; "
                "  border: 1px solid #555; "
                "  border-radius: 6px; "
                "}"
            );
            checksumText->setMinimumHeight(100);
            layout->addWidget(checksumText);
            
            QHBoxLayout *btnLayout = new QHBoxLayout();
            btnLayout->addStretch();
            
            QPushButton *copyBtn = new QPushButton("Copy to Clipboard");
            copyBtn->setCursor(Qt::PointingHandCursor);
            copyBtn->setMinimumWidth(150);
            connect(copyBtn, &QPushButton::clicked, [checksum, &dialog](){
                QApplication::clipboard()->setText("0x" + checksum);
                QToolTip::showText(QCursor::pos(), "Copied!");
            });
            btnLayout->addWidget(copyBtn);
            
            QPushButton *closeBtn = new QPushButton("Close");
            closeBtn->setCursor(Qt::PointingHandCursor);
            closeBtn->setMinimumWidth(100);
            connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
            btnLayout->addWidget(closeBtn);
            
            layout->addLayout(btnLayout);
            
            dialog.exec();
        }
    }
}

/**
 * @brief Handles clear button click event.
 */
void ChecksumWidget::on_btnClear_clicked()
{
    ui->tableWidget->setRowCount(0);
}

/**
 * @brief Finds row index by file path.
 * @param fullFilePath Full path to the file
 * @return Row index or -1 if not found
 */
int ChecksumWidget::findRowByPath(const QString &fullFilePath)
{
    for(int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidget->item(row, 0);
        if (item && item->data(Qt::UserRole).toString() == fullFilePath) {
            return row;
        }
    }
    return -1;
}

/**
 * @brief Processes a file and adds it to the table.
 * @param fileName Path to the file
 */
void ChecksumWidget::processFile(const QString &fileName)
{
    QFileInfo fi(fileName);

    if (fi.isDir()) return;

    QString resultHex = CalculateChecksum(fileName);

    int row = findRowByPath(fileName);

    if (row == -1) {
        row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
    }

    QTableWidgetItem *nameItem = new QTableWidgetItem(fi.fileName());
    nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameItem->setData(Qt::UserRole, fileName);
    ui->tableWidget->setItem(row, 0, nameItem);

    QTableWidgetItem *crcItem = new QTableWidgetItem(resultHex);
    crcItem->setForeground(QBrush(QColor("#0067c0")));
    QFont largeFont("Segoe UI", 16, QFont::Bold);
    largeFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    crcItem->setFont(largeFont);
    crcItem->setTextAlignment(Qt::AlignCenter);
    crcItem->setToolTip("Click to copy Checksum");
    ui->tableWidget->setItem(row, 1, crcItem);

    QTableWidgetItem *pathItem = new QTableWidgetItem(fi.absolutePath());
    pathItem->setForeground(QBrush(QColor("#666666")));
    pathItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tableWidget->setItem(row, 2, pathItem);

    QPushButton *openBtn = new QPushButton();
    openBtn->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    openBtn->setToolTip("Open File Location");
    openBtn->setObjectName("btnOpenFolder");
    openBtn->setCursor(Qt::PointingHandCursor);

    QString folderPath = fi.absolutePath();
    connect(openBtn, &QPushButton::clicked, [folderPath](){
        QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    });

    ui->tableWidget->setCellWidget(row, 3, openBtn);
}

/**
 * @brief Calculates checksum for given file using current algorithm.
 * @param filePath Path to the file
 * @return Checksum as hex string
 */
QString ChecksumWidget::CalculateChecksum(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return "ERROR";

    QByteArray data = file.readAll();
    file.close();

    unsigned char* memblock = reinterpret_cast<unsigned char*>(data.data());
    qint64 size = data.size();

    QString result;
    switch(currentType) {
        case CRC8: {
            unsigned char checkSumVal = computeCrc8(data);
            result = QString("%1").arg(checkSumVal, 2, 16, QChar('0')).toUpper();
            break;
        }
        case CRC16: {
            unsigned short checkSumVal = computeCrc(memblock, size);
            result = QString("%1").arg(checkSumVal, 4, 16, QChar('0')).toUpper();
            break;
        }
        case CRC32: {
            unsigned int checkSumVal = computeCrc32(data);
            result = QString("%1").arg(checkSumVal, 8, 16, QChar('0')).toUpper();
            break;
        }
        case ADLER32: {
            unsigned int checkSumVal = computeAdler32(data);
            result = QString("%1").arg(checkSumVal, 8, 16, QChar('0')).toUpper();
            break;
        }
        case MD5: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
            result = hash.toHex().toUpper();
            break;
        }
        case SHA1: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
            result = hash.toHex().toUpper();
            break;
        }
        case SHA256: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
            result = hash.toHex().toUpper();
            break;
        }
        case SHA384: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha384);
            result = hash.toHex().toUpper();
            break;
        }
        case SHA512: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha512);
            result = hash.toHex().toUpper();
            break;
        }
        case SHA3_256: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha3_256);
            result = hash.toHex().toUpper();
            break;
        }
        case SHA3_512: {
            QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha3_512);
            result = hash.toHex().toUpper();
            break;
        }
        default:
            result = "UNKNOWN";
    }

    return result;
}

/**
 * @brief Computes CRC-32 value for data.
 * @param data Input data
 * @return CRC-32 value
 */
unsigned int ChecksumWidget::computeCrc32(const QByteArray &data)
{
    // Standard CRC-32 polynomial 0xEDB88320
    static const unsigned int crc32Table[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B530AC, 0x42B2003A, 0xDBBBC180, 0xACBCF116, 0x32D8F4B5, 0x45DFC423, 0xDDD69599, 0xAAD1A50F,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    unsigned int crc = 0xFFFFFFFF;
    const unsigned char *p = (const unsigned char *)data.constData();
    for (int i = 0; i < data.size(); ++i) {
        crc = (crc >> 8) ^ crc32Table[(crc ^ p[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Computes CRC-16 value for data.
 * @param message Data buffer
 * @param blk_len Buffer length
 * @return CRC-16 value
 */
unsigned short ChecksumWidget::computeCrc(unsigned char *message, qint64 blk_len)
{
    unsigned short crctable[256] = {0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202};

    unsigned char index;
    unsigned long crc = 0L;
    unsigned char *blk_adr;

    blk_adr = message;

    while(blk_len != 0)
    {
        index = ((crc >> 8) ^ *blk_adr++) & 0xFFL;
        crc = crctable[index] ^ (crc << 8);
        blk_len--;
    }

    return (static_cast<unsigned short>(crc));
}

/**
 * @brief Computes CRC-8 value for data.
 * @param data Input data
 * @return CRC-8 value
 */
unsigned char ChecksumWidget::computeCrc8(const QByteArray &data)
{
    // CRC-8 polynomial 0x07 (x^8 + x^2 + x^1 + 1)
    static const unsigned char crc8Table[256] = {
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
        0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
        0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
        0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
        0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
        0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
        0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
        0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
        0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
        0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
        0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
    };

    unsigned char crc = 0x00;
    const unsigned char *p = (const unsigned char *)data.constData();
    for (int i = 0; i < data.size(); ++i) {
        crc = crc8Table[crc ^ p[i]];
    }
    return crc;
}

/**
 * @brief Computes Adler-32 checksum for data.
 * @param data Input data
 * @return Adler-32 value
 */
unsigned int ChecksumWidget::computeAdler32(const QByteArray &data)
{
    const unsigned int MOD_ADLER = 65521;
    unsigned int a = 1, b = 0;
    const unsigned char *p = (const unsigned char *)data.constData();
    
    for (int i = 0; i < data.size(); ++i) {
        a = (a + p[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    
    return (b << 16) | a;
}
