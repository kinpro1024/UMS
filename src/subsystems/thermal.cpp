#include <mutex>
#include <cstring>
#include <condition_variable>
#include "subsystem.hpp"
#include "../wrappers/thermal_wrapper.hpp"

class ThermalFrame : public Subsystem::Frame
{
    public:
        std::vector<float> temperatures_;
        std::chrono::steady_clock::time_point timestamp_;

        ThermalFrame()
        : temperatures_(768)
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

        const ThermalFrame* requestPreviewFrame() override;

    protected:
        void acquisitionLoop() override;

    private:
        Mlx90640 thermal_cam_;
        std::unique_ptr<ThermalFrame> latest_frame_  = std::make_unique<ThermalFrame>();
        std::condition_variable preview_cv_;
        std::mutex latest_frame_mutex_;
        ThermalFrame preview_buffer_;
        bool new_preview_frame_ = false;
};

int Thermal::init()
{
    if (!thermal_cam_.sensorInit(0x07)) //0x07 corresponds to 64Hz refresh rate on the MLX90640 sensor
    {
        return 0;
    }
    return 1;
}

void Thermal::acquisitionLoop()
{
    std::unique_ptr<ThermalWrapperFrame> new_frame = thermal_cam_.requestFullFrame(500);
    if (!new_frame)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(latest_frame_mutex_);
        latest_frame_->temperatures_ = std::move(new_frame->temperatures);
        latest_frame_->timestamp_ = new_frame->timestamp;
        new_preview_frame_ = true;
        preview_cv_.notify_one();
    }
}

const ThermalFrame* Thermal::requestPreviewFrame()
{
    {
        std::unique_lock<std::mutex> lock(latest_frame_mutex_);
        preview_cv_.wait(lock, [this] {return new_preview_frame_;});
        memcpy(preview_buffer_.temperatures_.data(), latest_frame_->temperatures_.data(), latest_frame_->temperatures_.size()*sizeof(float));
        new_preview_frame_ = false;
        return &preview_buffer_;
    }
}