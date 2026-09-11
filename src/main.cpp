
#include "core/umsd.hpp"
#include "ui/thermal_presenter.hpp"
#include "ui/tof_presenter.hpp"
#include "ui/rgb_presenter.hpp"
#include "core/ui.hpp"
#include "subsystems/subsystem.hpp"

#include <thread>

int main(int argc, char *argv[])
{
    ums::UmsDaemon umsd;

    ums::ThermalPresenter tp(umsd.getThermalRef());
    ums::TofPresenter tf(umsd.getTofRef());
    ums::RgbPresenter rgb(umsd.getRgbRef());
/*
    QObject::connect(&tp, &ums::ThermalPresenter::frameReady,
        [](const QImage& image)
        {
            std::cout << "QImage Thermal recvd: " << image.width() << "x" << image.height() << "\n";
        }
    );

    QObject::connect(&tf, &ums::TofPresenter::frameReady,
        [](const QImage& image)
        {
            std::cout << "QImage Tof recvd: " << image.width() << "x" << image.height() << "\n";
        }
    );

    QObject::connect(&rgb, &ums::RgbPresenter::frameReady,
        [](const QImage& image)
        {
            std::cout << "QImage Rgb recvd: " << image.width() << "x" << image.height() << "\n";
        }
    );
*/
    std::thread thermal_preview_worker(&ums::ThermalPresenter::loop, &tp);
    std::thread tof_preview_worker(&ums::TofPresenter::loop, &tf);
    std::thread rgb_preview_worker(&ums::RgbPresenter::loop, &rgb);

    ums::Ui ui;
    int result = ui.appStuff(argc, argv, tp, tf, rgb);

    tf.abortWorker();
    tof_preview_worker.join();

    tp.abortWorker();
    thermal_preview_worker.join();

    rgb.abortWorker();
    rgb_preview_worker.join();

    return result;
}