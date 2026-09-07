
#include <iostream>

#include "presenters.hpp"

void ums::ThermalPresenter::copy()
{
    const ums::Subsystem::Frame* latest_preview_frame = curr_thermal_reference_.acquirePreviewFrame();
    const ums::Thermal::ThermalFrame* casted = static_cast<const ums::Thermal::ThermalFrame*>(latest_preview_frame);

    memcpy(presenter_buffer_.temperatures_.data(), casted->temperatures_.data(), casted->temperatures_.size()*sizeof(float));

    curr_thermal_reference_.releasePreviewFrame();
}

void ums::ThermalPresenter::print()
{
    for(int i = 0; i < presenter_buffer_.temperatures_.size(); ++i)
    {
        if ((i + 1) % 32 == 0)
        {
            std::cout << "\n";
        }
        std::cout << presenter_buffer_.temperatures_[i] << " , ";
    }
}

void ums::ThermalPresenter::loop()
{
    while (true)
    {
        copy();
        print();
    }
}