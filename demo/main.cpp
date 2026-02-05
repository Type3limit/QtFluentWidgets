#include <QApplication>

#include "Fluent/FluentTheme.h"

#include "DemoWindow.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    Demo::DemoWindow w;
    w.resize(1260, 780);
    w.show();
    return app.exec();
}
