
#pragma once

#include <QObject>
#include <QImage>

#include "subsystem.hpp"
#include "thermal.hpp"

namespace ums
{
    class ThermalPresenter : public QObject
    {
        Q_OBJECT
        public:
            ThermalPresenter(Thermal& manager_reference, QObject* parent = nullptr)
                : QObject(parent),
                curr_thermal_reference_(manager_reference)
            {}

            void loop();

        signals:
            void frameReady(QImage image);
        
        private:
            void copy();
            void convertAndEmit();

            Thermal& curr_thermal_reference_;
            Thermal::ThermalFrame presenter_buffer_;
    };
}
