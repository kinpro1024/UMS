
#include "core/umsd.hpp"
#include "ui/thermal_presenter.hpp"
#include "core/ui.hpp"
#include "subsystems/subsystem.hpp"

#include <thread>

int main(int argc, char *argv[])
{
    ums::UmsDaemon umsd;

    ums::ThermalPresenter tp(umsd.getThermalRef());

    QObject::connect(&tp, &ums::ThermalPresenter::frameReady,
        [](const QImage& image)
        {
            std::cout << "QImage recvd: " << image.width() << "x" << image.height() << "\n";
        }
    );

    std::thread thermal_preview_worker(&ums::ThermalPresenter::loop, &tp);

    ums::Ui ui;
    int result = ui.appStuff(argc, argv, tp);

    tp.abortWorker();
    thermal_preview_worker.join();

    return result;
}