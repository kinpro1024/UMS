
#include "rgb_presenter.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <thread>

void ums::RgbPresenter::copy()
{
    const ums::Subsystem::Frame* latest_preview_frame = curr_rgb_reference_.acquirePreviewFrame();
    const ums::Rgb::RgbFrame* casted = static_cast<const ums::Rgb::RgbFrame*>(latest_preview_frame);

    memcpy(presenter_buffer_.qimage_.data(), casted->qimage_.data(), 691200);

    curr_rgb_reference_.releasePreviewFrame();
}

void ums::RgbPresenter::convertAndEmit()
{
    const auto& qimage_ = presenter_buffer_.qimage_;

    QImage image(qimage_.data(), 640, 360, 640*3, QImage::Format_RGB888);

    emit frameReady(image);
}

void ums::RgbPresenter::abortWorker()
{
    abort_rgb_preview_worker_.store(true);
}

void ums::RgbPresenter::loop()
{
    while (!abort_rgb_preview_worker_.load())
    {
        copy();
        convertAndEmit();
    }
}

int ums::RgbPresenter::frameCounter() const
{
    return m_counter_;
}

QImage ums::RgbPresenter::currentImage() const
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    return m_image_;
}

void ums::RgbPresenter::updateImage(QImage image)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex_);
        m_image_ = std::move(image);
    }

    ++m_counter_;
    emit frameCounterChanged();
}