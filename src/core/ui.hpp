
#pragma once

#include "thermal_presenter.hpp"
#include "tof_presenter.hpp"
#include "rgb_presenter.hpp"

#include <QApplication>
#include <QQmlApplicationEngine>

namespace ums
{
    class Ui
    {
        public:
            int appStuff(int argc_, char *argv_[], ThermalPresenter& thermal_presenter, TofPresenter& tof_presenter, RgbPresenter& rgb_presenter);
    };
}