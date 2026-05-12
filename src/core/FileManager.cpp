#include "FileManager.h"
#include <QUuid>
#include <QFileInfo>
#include "Trace.h"

FileManager::FileManager(QObject *parent) : QObject(parent)
{
    m_basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/vault";
    QDir().mkpath(m_basePath);
}

QString FileManager::importPdf(const QUrl &fileUrl)
{
    QString sourcePath = fileUrl.toLocalFile();
    if (sourcePath.isEmpty()) return QString();

    QFileInfo info(sourcePath);
    if (!info.exists()) return QString();

    // Create a unique name to avoid collisions
    QString targetName = QUuid::createUuid().toString(QUuid::WithoutBraces) + "_" + info.fileName();
    QString targetPath = m_basePath + "/" + targetName;

    CN_TRACE("Importing PDF: %s -> %s", sourcePath.toLocal8Bit().constData(), targetPath.toLocal8Bit().constData());

    if (QFile::copy(sourcePath, targetPath)) {
        return targetPath;
    }

    return QString();
}

QString FileManager::createNewNote(const QString &name)
{
    QString cleanName = name.isEmpty() ? "Untitled" : name;
    QString targetName = QUuid::createUuid().toString(QUuid::WithoutBraces) + "_" + cleanName + ".note";
    QString targetPath = m_basePath + "/" + targetName;
    
    // Create the empty anchor file
    QFile file(targetPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.close();
        return targetPath;
    }
    return QString();
}

QString FileManager::getStoragePath() const
{
    return m_basePath;
}

QStringList FileManager::getImportedFiles() const
{
    QDir dir(m_basePath);
    QStringList filters;
    filters << "*.pdf" << "*.note";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Time);
    for (int i = 0; i < files.size(); ++i) {
        files[i] = m_basePath + "/" + files[i];
    }
    return files;
}
