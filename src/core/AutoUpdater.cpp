#include "AutoUpdater.h"
#include "macros.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

AutoUpdater::AutoUpdater(QObject *parent) : QObject(parent), m_tempFile(nullptr)
{
    m_manager = new QNetworkAccessManager(this);
}

void AutoUpdater::checkForUpdates()
{
    QNetworkRequest request(QUrl("https://api.github.com/repos/Riteshp2001/PacketForge/releases/latest"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "PacketForge-AutoUpdater");
    
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onCheckFinished(reply); });
}

void AutoUpdater::onCheckFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Network error: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (!obj.contains("tag_name")) {
        emit errorOccurred("Invalid response from GitHub API");
        return;
    }

    QString latestTag = obj["tag_name"].toString();
    QString currentTag = APP_VERSION;

    // Remove 'v' prefix for comparison if present
    if (latestTag.startsWith("v", Qt::CaseInsensitive)) latestTag.remove(0, 1);
    if (currentTag.startsWith("v", Qt::CaseInsensitive)) currentTag.remove(0, 1);

    QVersionNumber latestVersion = QVersionNumber::fromString(latestTag);
    QVersionNumber currentVersion = QVersionNumber::fromString(currentTag);

    if (latestVersion > currentVersion) {
        QString downloadUrl;
        if (obj.contains("assets")) {
            QJsonArray assets = obj["assets"].toArray();
            for (const QJsonValue &value : assets) {
                QJsonObject asset = value.toObject();
                QString name = asset["name"].toString();
                // Prioritize .exe or installer
                if (name.endsWith(".exe", Qt::CaseInsensitive) || name.endsWith(".msi", Qt::CaseInsensitive)) {
                    downloadUrl = asset["browser_download_url"].toString();
                    break;
                }
            }
        }
        
        QString fullChangelog = obj["body"].toString();
        QString releaseNotes;
        
        // precise mapping to "## [1.0.3]"  or "## v1.0.3" or "## [v1.0.3]"
        // We try to find the header for the latest tag
        QString headerSearch = QString("## [%1]").arg(latestTag);
        if (!fullChangelog.contains(headerSearch)) {
             // Try check without brackets or with/without v
             headerSearch = QString("## %1").arg(latestTag);
        }
        
        if (fullChangelog.contains(headerSearch)) {
            int start = fullChangelog.indexOf(headerSearch);
            int end = fullChangelog.indexOf("## [", start + headerSearch.length());
            if (end == -1) end = fullChangelog.indexOf("## ", start + headerSearch.length()); // Check for next header without brackets
            
            if (end != -1) {
                releaseNotes = fullChangelog.mid(start, end - start).trimmed();
            } else {
                releaseNotes = fullChangelog.mid(start).trimmed();
            }
        } else {
            // Fallback: if we can't parse it, just show the whole thing or the first 500 chars as before
             releaseNotes = fullChangelog;
        }

        emit updateAvailable(obj["tag_name"].toString(), downloadUrl, releaseNotes);
    } else {
        emit noUpdateAvailable();
    }
}

void AutoUpdater::downloadUpdate(const QString &url)
{
    if (url.isEmpty()) {
        emit errorOccurred("Invalid download URL");
        return;
    }

    m_downloadUrl = url;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "PacketForge-AutoUpdater");
    
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString fileName = url.section('/', -1);
    if (fileName.isEmpty()) fileName = "update.exe";
    
    QString filePath = tempPath + "/" + fileName;
    
    m_tempFile = new QFile(filePath);
    if (!m_tempFile->open(QIODevice::WriteOnly)) {
        emit errorOccurred("Could not open temporary file for writing");
        delete m_tempFile;
        m_tempFile = nullptr;
        return;
    }

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &AutoUpdater::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply, filePath]() { 
        if (reply->error() == QNetworkReply::NoError) {
             m_tempFile->write(reply->readAll());
             m_tempFile->close();
             emit updateReady(filePath);
        } else {
             m_tempFile->close();
             m_tempFile->remove();
             emit errorOccurred("Download failed: " + reply->errorString());
        }
        m_tempFile->deleteLater();
        m_tempFile = nullptr;
        reply->deleteLater();
    });
}

void AutoUpdater::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}
