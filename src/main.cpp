
#include "core/umsd.hpp"
#include "subsystems/subsystem.hpp"
#include "core/presenters.hpp"

int main(int argc, char *argv[])
{
    ums::UmsDaemon umsd;

    ums::ThermalPresenter tp(umsd.getThermalRefFromManager());

    tp.loop();

}