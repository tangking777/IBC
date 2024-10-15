#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>

#include <QSharedMemory>
#include <QMessageBox>

#include "./src/FileFuncs.h"
#include "./src/IbpicpDataControl.h"
#include "./src/qmlplot.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    app.setWindowIcon(QIcon("qrc:/Res/logo.ico"));
    qmlRegisterType<FileFuncs>("FileFuncs", 1, 0, "FileFuncs");
    qmlRegisterType<IbpicpDataControl>("IbpicpDataControl", 1, 0, "IbpicpDataControl");
    qmlRegisterType<CustomPlotItem>("CustomPlot", 1, 0, "CustomPlotItem");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
