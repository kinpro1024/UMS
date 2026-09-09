
#pragma once

#include "thermal_presenter.hpp"

#include <QApplication>
#include <QQmlApplicationEngine>

namespace ums
{
    class Ui
    {
        public:
            int appStuff(int argc_, char *argv_[], ThermalPresenter& thermal_presenter);
    };
}