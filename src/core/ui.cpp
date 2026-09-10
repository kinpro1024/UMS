
#include "ui.hpp"

#include <QQmlContext>

//==========================================================================================================================
//UI BULLSHIT
//==========================================================================================================================

int ums::Ui::appStuff(int argc_, char *argv_[], ums::ThermalPresenter& thermal_presenter, ums::TofPresenter& tof_presenter)
{
    QApplication app(argc_, argv_);

    QQmlApplicationEngine engine;

    engine.addImageProvider("thermal", new ums::ThermalImageProvider(&thermal_presenter));
    engine.addImageProvider("tof", new ums::TofImageProvider(&tof_presenter));

    engine.rootContext()->setContextProperty("thermalPresenter", &thermal_presenter);
    engine.rootContext()->setContextProperty("tofPresenter", &tof_presenter);

    engine.loadFromModule("UmsContent", "App");

    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }

    return app.exec();
}

//--------------------------------------------------------------------------------------------------------------------------