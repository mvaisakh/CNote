#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "core/CrashHandler.h"
#include "core/FileManager.h"

int main(int argc, char *argv[])
{
    Cerium::setupCrashHandler();
    QGuiApplication app(argc, argv);

    app.setApplicationName("CeriumNotes");
    app.setOrganizationName("CeriumNotes");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appVersion", APP_VERSION_STR);
    
    QString initialPdf;
    if (argc > 1) {
        initialPdf = QString::fromLocal8Bit(argv[1]);
    }
    QVariantMap initialProperties;
    initialProperties[QStringLiteral("initialPdf")] = initialPdf;
    engine.setInitialProperties(initialProperties);

    const QUrl url(u"qrc:/CeriumNotes/qml/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
