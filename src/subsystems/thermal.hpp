
#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

#include "subsystem.hpp"
#include "thermal_wrapper.hpp"

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
                setPreviewBufferAddress(&thermal_preview_buffer_);

                std::cout << "somehow this happened, if this did, dance baby dance" << std::endl;

                //0x07 corresponds to 64Hz refresh rate on the MLX90640 sensor
                if (thermal_cam_.sensorInit(0x07))
                {
                    throw std::runtime_error("THERMAL INIT FAILED");
                }

                startAcquisitionMachinery();
            }

            ~Thermal()
            {
                stopAcquisitionMachinery();
            }

        protected:
            std::unique_ptr<Frame> acquireLatestFrame() override;
            void copyToPreviewBuffer(Frame* preview) override;
            void saveFrame(std::unique_ptr<Frame> frame, State state) override;

        private:
            ThermalFrame thermal_preview_buffer_;
            Mlx90640 thermal_cam_;
    };
}