#include <mutex>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <condition_variable>
#include <atomic>
#include <thread>
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
                Params thermal_params_{true, true, true, 120};
                setParams(thermal_params_);
                if (thermal_cam_.sensorInit(0x07)) //0x07 corresponds to 64Hz refresh rate on the MLX90640 sensor
                {
                    throw std::runtime_error("THERMAL INIT FAILED");
                }
                std::thread aq_thread_(&acquisitionLoop, this);
            }
            ~Thermal()
            {
                abort_acquisition_loop_ = true;
                aq_thread_.join();
            }

            void setState(UmsDaemon::State state);
            const ThermalFrame* acquirePreviewFrame();
            void releasePreviewFrame();

        protected:
            void acquisitionLoop() override;
            void fillPreview() override;
            void saveFrame(std::unique_ptr<Frame> frame) override;

        private:
            std::thread aq_thread_;
            bool abort_acquisition_loop_ = false;
            ThermalFrame preview_buffer_;
            Mlx90640 thermal_cam_;
            std::unique_ptr<ThermalFrame> latest_frame_  = std::make_unique<ThermalFrame>();
            ThermalFrame preview_buffer_;
            bool new_preview_frame_ = false;
            std::mutex preview_mutex_;
            std::condition_variable preview_cv_;
    };
}

const ums::Thermal::ThermalFrame* ums::Thermal::acquirePreviewFrame()
{
    {
        std::unique_lock<std::mutex> lock(preview_mutex_);
        preview_cv_.wait(lock, [this] {return new_preview_frame_;});
        new_preview_frame_ = false;
        //HERE ENDS UNIQUE LOCK SCOPE!!!!
    }
    //THIS LOCKS THE ACTUAL BUFFER FOR FRONTEND until releasePreviewFrame() runs and unlocks it.
    preview_mutex_.lock();
    return &preview_buffer_;
}

void ums::Thermal::releasePreviewFrame()
{
    preview_mutex_.unlock();
}

void ums::Thermal::acquisitionLoop()
{
    while(!abort_acquisition_loop_)
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
}

void ums::Thermal::fillPreview()
{
    std::unique_lock<std::mutex> lock(preview_mutex_, std::try_to_lock);
    if (lock.owns_lock())
    {
        memcpy(preview_buffer_.temperatures_.data(), latest_frame_->temperatures_.data(), latest_frame_->temperatures_.size()*sizeof(float));
        new_preview_frame_ = true;
    }
}

void ums::Thermal::saveFrame(std::unique_ptr<Frame> frame)
{
    //Use this for the save and let both get destroyed at end of scope as unique_ptr is destroyed.
    ThermalFrame* save_data = static_cast<ThermalFrame*>(frame.get());
    // filename = "temps_$width$_$height$_uint_16_$time.raw"
    long long tstp = std::chrono::duration_cast<std::chrono::microseconds>(save_data->timestamp_.time_since_epoch()).count();
    std::string filename1 = "temps_" + std::to_string(32) + "_" + std::to_string(24) + "_float_" + std::to_string(tstp) + ".raw";
    std::ofstream file1(filename1, std::ios::binary);
    file1.write(reinterpret_cast<char*>(save_data->temperatures_.data()),sizeof(save_data->temperatures_.size()*sizeof(float)));
    file1.close();
}