
#pragma once

#include "presenters.hpp"

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