
#pragma once

#include "subsystem.hpp"
#include "thermal.hpp"
namespace ums
{
    class ThermalPresenter
    {
        public:
            ThermalPresenter(Thermal& manager_reference)
                : curr_thermal_reference_(manager_reference)
            {}

            void loop();
        
        private:
            void copy();
            void print();

            Thermal& curr_thermal_reference_;
            Thermal::ThermalFrame presenter_buffer_;
    };
}
