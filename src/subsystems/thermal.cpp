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
                aq_thread_ = std::thread(&acquisitionLoop, this);
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
            std::unique_ptr<Frame> acquireLatestFrame() override;
            Subsystem::Frame* copyToBuffer(Frame* preview) override;
            void saveFrame(std::unique_ptr<Frame> frame) override;

        private:
            std::thread aq_thread_;
            bool abort_acquisition_loop_ = false;
            ThermalFrame thermal_preview_buffer_;
            Mlx90640 thermal_cam_;
            bool new_preview_frame_ = false;
            std::mutex preview_mutex_;
            std::condition_variable preview_cv_;
    };
}

std::unique_ptr<ums::Subsystem::Frame> ums::Thermal::acquireLatestFrame()
{
    std::unique_ptr<ThermalFrame> latest_thermal_frame_  = std::make_unique<ThermalFrame>();
    std::unique_ptr<ThermalWrapperFrame> new_frame = thermal_cam_.requestFullFrame(500);
    if (!new_frame)
    {
        return nullptr;
    }
    latest_thermal_frame_->temperatures_ = std::move(new_frame->temperatures);
    latest_thermal_frame_->timestamp_ = new_frame->timestamp;
    return latest_thermal_frame_;
}

ums::Subsystem::Frame* ums::Thermal::copyToBuffer(ums::Subsystem::Frame* frame)
{
    ThermalFrame* thframe = static_cast<ThermalFrame*>(frame);
    memcpy(thermal_preview_buffer_.temperatures_.data(), thframe->temperatures_.data(), thframe->temperatures_.size()*sizeof(float));
    return &thermal_preview_buffer_;
}

void ums::Thermal::saveFrame(std::unique_ptr<Frame> frame)
{
    //Use this for the save and let both get destroyed at end of scope as unique_ptr is destroyed.
    ThermalFrame* save_data = static_cast<ThermalFrame*>(frame.get());
    // filename = "temps_$width$_$height$_uint_16_$time.raw"
    long long tstp = std::chrono::duration_cast<std::chrono::microseconds>(save_data->timestamp_.time_since_epoch()).count();
    std::string filename1 = "temps_" + std::to_string(32) + "_" + std::to_string(24) + "_float_" + std::to_string(tstp) + ".raw";
    std::ofstream file1(filename1, std::ios::binary);
    file1.write(reinterpret_cast<char*>(save_data->temperatures_.data()),save_data->temperatures_.size()*sizeof(float));
    file1.close();
}