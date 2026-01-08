#ifndef AUTOUPDATER_H
#define AUTOUPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class AutoUpdater : public QObject
{
    Q_OBJECT
public:
    explicit AutoUpdater(QObject *parent = nullptr);
    void checkForUpdates();
    void downloadUpdate(const QString &url);

signals:
    void updateAvailable(const QString &version, const QString &downloadUrl, const QString &releaseNotes);
    void noUpdateAvailable();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateReady(const QString &filePath);
    void errorOccurred(const QString &error);

private slots:
    void onCheckFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QNetworkAccessManager *m_manager;
    QString m_downloadUrl;
    QFile *m_tempFile;
};

#endif // AUTOUPDATER_H
