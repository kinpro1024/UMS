
#include "tof_presenter.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <thread>

void ums::TofPresenter::copy()
{
    const ums::Subsystem::Frame* latest_preview_frame = curr_tof_reference_.acquirePreviewFrame();
    const ums::Tof::TofFrame* casted = static_cast<const ums::Tof::TofFrame*>(latest_preview_frame);

    memcpy(presenter_buffer_.depth_data_.data(), casted->depth_data_.data(), casted->depth_data_.size()*sizeof(float));

    curr_tof_reference_.releasePreviewFrame();
}

void ums::TofPresenter::convertAndEmit()
{
    const auto& depth_data = presenter_buffer_.depth_data_;

    auto [min_it, max_it] = std::minmax_element(depth_data.begin(), depth_data.end());

    const float min_depth = *min_it;
    const float max_depth = *max_it;

    QImage image(240, 180, QImage::Format_Grayscale8);

    const float range = max_depth - min_depth;

    if (range == 0.0f || !std::isfinite(range))
    {
        image.fill(0);
    }

    else
    {
        for (int i = 0; i < 240*180; ++i)
        {
            float normalised = (depth_data[i] - min_depth) / range;

            normalised = std::clamp(normalised, 0.0f, 1.0f);

            image.bits()[i] = static_cast<uchar>(normalised * 255.0f);
        }
    }

    emit frameReady(image);
}

void ums::TofPresenter::abortWorker()
{
    abort_tof_preview_worker_.store(true);
}

void ums::TofPresenter::loop()
{
    while (!abort_tof_preview_worker_.load())
    {
        copy();
        convertAndEmit();
    }
}

int ums::TofPresenter::frameCounter() const
{
    return m_counter_;
}

QImage ums::TofPresenter::currentImage() const
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    return m_image_;
}

void ums::TofPresenter::updateImage(QImage image)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex_);
        m_image_ = std::move(image);
    }

    ++m_counter_;
    emit frameCounterChanged();
}