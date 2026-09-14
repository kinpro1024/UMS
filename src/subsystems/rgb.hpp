
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

#include <chrono>
#include <iomanip>
#include <sstream>

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

                //technically this belongs in acquisition but that thread is managed by subsystem
                client_ = accept(sock_, nullptr, nullptr);

                startAcquisitionMachinery();
            }

            ~Rgb()
            {
                stopAcquisitionMachinery();

                if (rpicam_pid_ > 0)
                {
                    kill(rpicam_pid_, SIGKILL);
                    waitpid(rpicam_pid_, nullptr, 0);
                }

                close(client_.load());
                unlink("/tmp/frametime.sock"); //My mom doesn't pick up after me, I do
            }

        protected:
            std::unique_ptr<Frame> acquireLatestFrame() override;
            void copyToPreviewBuffer(Frame* preview) override;

            void customVideoPipelineStart() override;
            void customVideoPipelineStop() override;
            void customStillPipelineTrigger() override;

            std::string timestampedFilename(const std::string& extension)
            {
                const auto now = std::chrono::system_clock::now();
                const std::time_t t = std::chrono::system_clock::to_time_t(now);

                std::tm local_time{};
                localtime_r(&t, &local_time);

                std::ostringstream filename;
                filename << std::put_time(&local_time, "%Y%m%d_%H%M%S")
                        << extension;

                return filename.str();
            }

        private:
            int sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
            std::atomic<int> client_;

            RgbFrame rgb_recieve_buffer_;
            RgbFrame rgb_preview_buffer_;

            pid_t rpicam_pid_;

            bool recvAll(int client, void* buffer, size_t size);
    };
}
