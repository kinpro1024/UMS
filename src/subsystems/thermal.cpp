#include <mutex>
#include <cstring>
#include "subsystem.hpp"
#include "../wrappers/thermal_wrapper.hpp"

class ThermalFrame : public Subsystem::Frame
{
    public:
        std::vector<float> tempratures_;
        std::chrono::steady_clock::time_point timestamp_;

        ThermalFrame()
        : tempratures_(768)
        {}
};

class Thermal : public Subsystem
{
    public:
        int init() override;
        void idle() override;
        void stillCapture() override;
        void videoCapture() override;
        void deinit() override;

        const ThermalFrame* acquire() override;
        void release() override;

    private:
        Mlx90640 thermal_cam_;
        std::unique_ptr<ThermalFrame> latest_frame_  = std::make_unique<ThermalFrame>();
        std::mutex latest_frame_mutex_;
        ThermalFrame preview_buffer_;
};

int Thermal::init()
{
    if (!thermal_cam_.sensorInit(0x07)) //0x07 corresponds to 64Hz refresh rate on the MLX90640 sensor
    {
        return 0;
    }
    return 1;
}

void Thermal::idle()
{
    std::unique_ptr<ThermalWrapperFrame> new_frame = thermal_cam_.requestFullFrame(500);
    if (!new_frame)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(latest_frame_mutex_);
        latest_frame_->tempratures_ = std::move(new_frame->temperatures);
        latest_frame_->timestamp_ = new_frame->timestamp;
    }
}

const ThermalFrame* Thermal::acquire()
{
    if (latest_frame_mutex_.try_lock())
    {
        memcpy(preview_buffer_.tempratures_.data(), latest_frame_->tempratures_.data(), latest_frame_->tempratures_.size());
        return &preview_buffer_;
    }
    else
    {
        return nullptr;
    }
}

void Thermal::release()
{
    latest_frame_mutex_.unlock();
}