
#include "presenters.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <QThread>

void ums::ThermalPresenter::copy()
{
    const ums::Subsystem::Frame* latest_preview_frame = curr_thermal_reference_.acquirePreviewFrame();
    const ums::Thermal::ThermalFrame* casted = static_cast<const ums::Thermal::ThermalFrame*>(latest_preview_frame);

    memcpy(presenter_buffer_.temperatures_.data(), casted->temperatures_.data(), casted->temperatures_.size()*sizeof(float));

    curr_thermal_reference_.releasePreviewFrame();
}

void ums::ThermalPresenter::convertAndEmit()
{
    const auto& temperatures = presenter_buffer_.temperatures_;

    auto [min_it, max_it] = std::minmax_element(temperatures.begin(), temperatures.end());

    const float min_temp = *min_it;
    const float max_temp = *max_it;

    QImage image(32, 24, QImage::Format_Grayscale8);

    const float range = max_temp - min_temp;

    if (range == 0.0f || !std::isfinite(range))
    {
        image.fill(0);
    }

    else
    {
        for (int i = 0; i < 768; ++i)
        {
            float normalised = (temperatures[i] - min_temp) / range;

            normalised = std::clamp(normalised, 0.0f, 1.0f);

            image.bits()[i] = static_cast<uchar>(normalised * 255.0f);
        }
    }

    emit frameReady(image);
}

void ums::ThermalPresenter::loop()
{
    while (!QThread::currentThread()->isInterruptionRequested())
    {
        copy();
        convertAndEmit();
    }
}