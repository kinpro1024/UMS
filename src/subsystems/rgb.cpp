#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <cstring>
#include <condition_variable>
#include "subsystem.hpp"

class RgbFrame : public Subsystem::Frame
{
    public:
        std::vector<uint8_t> qimage_;
        std::chrono::steady_clock::time_point timestamp_;

        RgbFrame()
        : qimage_(691200)
        {}
};

class Rgb : public Subsystem
{
    public:
        int init() override;
        void idle() override;
        void stillCapture() override;
        void videoCapture() override;
        void deinit() override;

        const RgbFrame* requestPreviewFrame() override;

    protected:
        void acquisitionLoop() override;

    private:
        
        int sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
        std::unique_ptr<RgbFrame> latest_frame_  = std::make_unique<RgbFrame>();
        std::condition_variable preview_cv_;
        std::mutex latest_frame_mutex_;
        RgbFrame preview_buffer_;
        bool new_preview_frame_ = false;
};

int Rgb::init()
{
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

    int client = accept(sock_, nullptr, nullptr);
}

void Rgb::acquisitionLoop()
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

const RgbFrame* Rgb::requestPreviewFrame()
{
    {
        std::unique_lock<std::mutex> lock(latest_frame_mutex_);
        preview_cv_.wait(lock, [this] {return new_preview_frame_;});
        memcpy(preview_buffer_.temperatures_.data(), latest_frame_->temperatures_.data(), latest_frame_->temperatures_.size()*sizeof(float));
        new_preview_frame_ = false;
        return &preview_buffer_;
    }
}