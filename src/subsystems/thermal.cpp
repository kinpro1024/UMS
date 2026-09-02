#include <mutex>
#include <stdexcept>
#include <cstring>
#include <condition_variable>
#include <atomic>
#include "subsystem.hpp"
#include "../wrappers/thermal_wrapper.hpp"

namespace ums
{
    class Thermal : public Subsystem
    {
        public:
            class ThermalFrame : public Subsystem::Frame
            {
                public:
                    std::vector<float> temperatures_;
                    std::chrono::steady_clock::time_point timestamp_;

                    ThermalFrame()
                    : temperatures_(768)
                    {}
            };
            Thermal()
                {
                    if (thermal_cam_.sensorInit(0x07)) //0x07 corresponds to 64Hz refresh rate on the MLX90640 sensor
                    {
                        throw std::runtime_error("THERMAL INIT FAILED");
                    }
                }
            ~Thermal()
            {
                //aqloop join
            }

            void setState(UmsDaemon::State state) override;
            const ThermalFrame* acquirePreviewFrame() override;
            void releasePreviewFrame() override;

        protected:
            void acquisitionLoop() override;
            void fillPreview() override;
            void stateExecution() override;
            void saveFrame(std::unique_ptr<Frame> frame) override;

        private:
            std::atomic<UmsDaemon::State> current_state_;
            Mlx90640 thermal_cam_;
            std::unique_ptr<ThermalFrame> latest_frame_  = std::make_unique<ThermalFrame>();
            std::condition_variable preview_cv_;
            std::mutex preview_mutex_;
            ThermalFrame preview_buffer_;
            bool new_preview_frame_ = false;
    };
}

void ums::Thermal::acquisitionLoop()
{
    std::unique_ptr<ThermalWrapperFrame> new_frame = thermal_cam_.requestFullFrame(500);
    if (!new_frame)
    {
        return;
    }
    latest_frame_->temperatures_ = std::move(new_frame->temperatures);
    latest_frame_->timestamp_ = new_frame->timestamp;
    stateExecution();
}

void ums::Thermal::stateExecution()
{
    std::unique_ptr<Frame> frame;
    switch (current_state_)
    {
        case ums::UmsDaemon::State::IDLE:
            fillPreview();
            break;

        case ums::UmsDaemon::State::STILL_CAPTURE:
            frame = std::move(latest_frame_);
            saveFrame(frame);
            break;
        
        case ums::UmsDaemon::State::VIDEO_CAPTURE:
            fillPreview();
            writeEnqueue();
            break;

        default:
            break;
    }
}

void ums::Thermal::fillPreview()
{
    if (std::unique_lock<std::mutex> lock(preview_mutex_, std::try_to_lock); lock.owns_lock())
    {
        memcpy(preview_buffer_.temperatures_.data(), latest_frame_->temperatures_.data(), latest_frame_->temperatures_.size()*sizeof(float));
        new_preview_frame_ = true;
    }
    else
    {
        return;
    }
    preview_cv_.notify_one();
}

const ums::Thermal::ThermalFrame* ums::Thermal::acquirePreviewFrame()
{
    {
        std::unique_lock<std::mutex> lock(preview_mutex_);
        preview_cv_.wait(lock, [this] {return new_preview_frame_;});
        new_preview_frame_ = false;
        //HERE ENDS UNIQUE LOCK SCOPE
    }
    //THIS LOCKS THE ACTUAL BUFFER FOR FRONTEND until releasePreviewFrame() runs and unlocks it
    preview_mutex_.lock();
    return &preview_buffer_;
}

void ums::Thermal::releasePreviewFrame()
{
    preview_mutex_.unlock();
}

void ums::Thermal::saveFrame(std::unique_ptr<Frame> frame)
{
    
}