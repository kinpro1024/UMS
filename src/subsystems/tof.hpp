
#pragma once

#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <iostream>

#include "subsystem.hpp"
#include "ArducamTOFCamera.hpp"

namespace ums
{
    class Tof : public Subsystem
    {
        public:
            class TofFrame : public Subsystem::Frame
            {
                public:
                    std::vector<float> depth_data_;
                    std::vector<float> confidence_data_;
                    std::chrono::steady_clock::time_point timestamp_;

                    TofFrame()
                    : depth_data_(240*180),
                    confidence_data_(240*180)
                    {}
            };

            class TofQuantFrame : public Subsystem::Frame
            {
                public:
                    std::vector<uint16_t> qdepth_data_;
                    std::vector<uint16_t> qconfidence_data_;
                    std::chrono::steady_clock::time_point qtimestamp_;

                    TofQuantFrame()
                    : qdepth_data_(240*180),
                    qconfidence_data_(240*180)
                    {}
            };

            Tof()
            {
                Params tof_params_{true, true, true, 120};
                setParams(tof_params_);
                setPreviewBufferAddress(&tof_preview_buffer_);

                //8 when both RGB camera and ToF are connected via CSI
                if (tof_.open(Arducam::Connection::CSI, 8))
                {
                    throw std::runtime_error("Tof Camera connection failed");
                }

                if (tof_.start(Arducam::FrameType::DEPTH_FRAME))
                {
                    throw std::runtime_error("Tof Camera could not be started in DEPTH MODE");
                }

                tof_.setControl(Arducam::Control::RANGE, MAX_DISTANCE_);
                tof_.getControl(Arducam::Control::RANGE, &max_range_);

                std::cout << "\n\n\nyayyyyyyyyy\n\n\n" << std::endl;

                startAcquisitionMachinery();
            }

            ~Tof()
            {
                stopAcquisitionMachinery();

                tof_.stop();
                tof_.close();
            }

        protected:
            std::unique_ptr<Frame> acquireLatestFrame() override;
            void copyToPreviewBuffer(Frame* preview) override;
            std::unique_ptr<Frame> prepareFrame(std::unique_ptr<Frame> frame) override;
            void saveFrame(std::unique_ptr<Frame> frame, State state) override;

        private:
            TofFrame tof_preview_buffer_;
            Arducam::ArducamTOFCamera tof_;

            const int MAX_DISTANCE_ = 4000;
            const int MAX_WIDTH_ = 240;
            const int MAX_HEIGHT_ = 180;
            int max_range_ = 0;
            const int FRAME_SIZE_ = MAX_HEIGHT_ * MAX_WIDTH_;
    };
}