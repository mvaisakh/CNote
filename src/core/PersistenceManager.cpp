#include "PersistenceManager.h"
#include <QFile>
#include <QDataStream>
#include "Trace.h"

QDataStream &operator<<(QDataStream &out, const InkPoint &p) {
    out << p.x << p.y << p.pressure << p.tilt << (qint64)p.timestamp;
    return out;
}

QDataStream &operator>>(QDataStream &in, InkPoint &p) {
    in >> p.x >> p.y >> p.pressure >> p.tilt >> (qint64&)p.timestamp;
    return in;
}

QDataStream &operator<<(QDataStream &out, const Stroke &s) {
    out << s.color << s.width << (quint32)s.points.size();
    for (const auto &p : s.points) out << p;
    return out;
}

QDataStream &operator>>(QDataStream &in, Stroke &s) {
    quint32 pointCount;
    in >> s.color >> s.width >> pointCount;
    s.points.resize(pointCount);
    for (quint32 i = 0; i < pointCount; ++i) in >> s.points[i];
    return in;
}

bool PersistenceManager::save(const QString &path, const CanvasState &state)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);

    // Magic number and version
    out << (quint32)0xCE71011 // "CE-RI-UM"
        << (quint32)2         // File format version
        << state.zoomLevel
        << state.panOffset
        << (quint32)state.strokes.size()
        << (quint32)state.pageCount;

    for (const auto &s : state.strokes) {
        out << s;
    }

    CN_TRACE("Saved %d strokes to %s", (int)state.strokes.size(), path.toLocal8Bit().constData());
    return true;
}

bool PersistenceManager::load(const QString &path, CanvasState &state)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    quint32 magic, version, strokeCount;
    in >> magic >> version;

    if (magic != 0xCE71011) return false;

    in >> state.zoomLevel >> state.panOffset >> strokeCount;
    if (version >= 2) {
        quint32 pgCount;
        in >> pgCount;
        state.pageCount = pgCount;
    } else {
        state.pageCount = 1;
    }

    state.strokes.resize(strokeCount);

    for (quint32 i = 0; i < strokeCount; ++i) {
        in >> state.strokes[i];
    }

    CN_TRACE("Loaded %d strokes from %s", (int)strokeCount, path.toLocal8Bit().constData());
    return true;
}
