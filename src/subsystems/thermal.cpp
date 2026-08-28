#include <mutex>
#include "subsystem.hpp"
#include "../wrappers/thermal_wrapper.hpp"

class Thermal : public Subsystem
{
    public:
        int init() override;
        void idle() override;
        void stillCapture() override;
        void videoCapture() override;
        void deinit() override;

        const Subsystem::Frame* acquire() override;
        void release() override;

    private:
        Mlx90640 thermal_cam_;
        std::unique_ptr<ThermalFrame> latest_frame_;
        std::mutex thermal_preview_buffer_mutex_;
        Subsystem::Frame thermal_payload_;
        const uint8_t THERMAL_ID_ = 3;
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
    std::unique_ptr<ThermalFrame> new_frame = thermal_cam_.requestFullFrame(500);
    if (!new_frame)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(thermal_preview_buffer_mutex_);
        latest_frame_ = std::move(new_frame);
    }
}

const Subsystem::Frame* Thermal::acquire()
{
    thermal_preview_buffer_mutex_.lock();
    thermal_payload_.data_ = latest_frame_.get();
    thermal_payload_.subsys_id_ = THERMAL_ID_;
    return &thermal_payload_;
}

void Thermal::release()
{
    thermal_preview_buffer_mutex_.unlock();
}