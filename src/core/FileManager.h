#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QQmlEngine>

class FileManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit FileManager(QObject *parent = nullptr);

    Q_INVOKABLE QString importPdf(const QUrl &fileUrl);
    Q_INVOKABLE QString getStoragePath() const;
    Q_INVOKABLE QStringList getImportedFiles() const;

private:
    QString m_basePath;
};
