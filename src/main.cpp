
#include "core/umsd.hpp"
#include "subsystems/subsystem.hpp"
#include "core/presenters.hpp"
#include "core/ui.hpp"

int main(int argc, char *argv[])
{
    ums::UmsDaemon umsd;


    ums::ThermalPresenter tp(umsd.getThermalRefFromManager());

    QObject::connect(&tp, &ums::ThermalPresenter::frameReady,
        [](const QImage& image)
        {
            std::cout << "QImage recvd: " << image.width() << "x" << image.height() << "\n";
        }
    );

    tp.loop();

    ums::Ui ui;
    ui.appStuff(argc, argv);
}