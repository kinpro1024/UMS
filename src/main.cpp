
#include "core/umsd.hpp"
#include "ui/thermal_presenter.hpp"
#include "core/ui.hpp"
#include "subsystems/subsystem.hpp"

#include <QThread>

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

    QThread* worker = QThread::create([&tp]()
    {
        tp.loop();
    });

    worker->start();

    ums::Ui ui;
    int result = ui.appStuff(argc, argv, tp);

    worker->requestInterruption();
    worker->wait();

    delete worker;
    return result;
}