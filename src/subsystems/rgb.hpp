
#pragma once

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <cstring>
#include <thread>
#include <stdexcept>

#include "subsystem.hpp"



namespace ums
{
    class Rgb : public Subsystem
    {
        public:
            class RgbFrame : public Subsystem::Frame
            {
                public:
                    std::vector<uint8_t> qimage_;
                    std::chrono::steady_clock::time_point timestamp_;

                    RgbFrame()
                    : qimage_(691200)
                    {}
            };

            Rgb()
            {
                Params rgb_params_{true, false, false, 0};
                setParams(rgb_params_);
                setPreviewBufferAddress(&rgb_preview_buffer_);

                rpicam_pid_ = fork();

                if (rpicam_pid_ < 0)
                {
                    throw std::runtime_error("could not spawn rpicam process");
                }

                else if (rpicam_pid_ == 0)
                {
                    execl("/usr/local/bin/rpicam-hello", "rpicam-hello", "--timeout", "0",
                           "--preview-backend", "umsd", "--preview-libs",
                           "/home/kinpro1024/hijinks/rpicam-apps/build/preview",
                           (char*)nullptr);
                }

                std::cout << "started with pid: " << rpicam_pid_ << std::endl;

                //Rgb does not interact with an API, rpicam-vid is forced to use
                //a custom --umsd-preview-backend which opens a socket to address
                ///tmp/frame.sock and is structured as a timepoint data followed
                //by an RGB888 frame
                sockaddr_un addr_{};
                addr_.sun_family = AF_UNIX;
                strcpy(addr_.sun_path, "/tmp/frametime.sock");
                unlink("/tmp/frametime.sock");
                bind(sock_, (sockaddr*)&addr_, sizeof(addr_));
                listen(sock_, 1);

                client_ = accept(sock_, nullptr, nullptr);

                std::cout << "somehow this happened, if this did, dance baby dance" << std::endl;

                startAcquisitionMachinery();
            }

            ~Rgb()
            {
                stopAcquisitionMachinery();
            }

        protected:
            std::unique_ptr<Frame> acquireLatestFrame() override;
            void copyToPreviewBuffer(Frame* preview) override;

            void customVideoPipelineStart() override;
            void customVideoPipelineStop() override;
            void customStillPipelineTrigger() override;

        private:
            int sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
            int client_;

            RgbFrame rgb_recieve_buffer_;
            RgbFrame rgb_preview_buffer_;

            pid_t rpicam_pid_;

            bool recvAll(int client, void* buffer, size_t size);
    };
}
