
#include "ui.hpp"

#include <QQmlContext>
#include <QQuickWindow>

//==========================================================================================================================
//UI BULLSHIT
//==========================================================================================================================

int ums::Ui::appStuff(int argc_, char *argv_[], ums::ThermalPresenter& thermal_presenter, ums::TofPresenter& tof_presenter, ums::RgbPresenter& rgb_presenter)
{
    QApplication app(argc_, argv_);

    QQmlApplicationEngine engine;

    engine.addImageProvider("thermal", new ums::ThermalImageProvider(&thermal_presenter));
    engine.addImageProvider("tof", new ums::TofImageProvider(&tof_presenter));
    engine.addImageProvider("rgb", new ums::RgbImageProvider(&rgb_presenter));

    engine.rootContext()->setContextProperty("thermalPresenter", &thermal_presenter);
    engine.rootContext()->setContextProperty("tofPresenter", &tof_presenter);
    engine.rootContext()->setContextProperty("rgbPresenter", &rgb_presenter);

    engine.loadFromModule("UmsContent", "App");

    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }

    /*
    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first()))
    {
        window->showFullScreen();
    }
    */

    return app.exec();
}

//--------------------------------------------------------------------------------------------------------------------------