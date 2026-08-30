#include <mutex>
#include <condition_variable>
#include "subsystem.hpp"
#include "ArducamTOFCamera.hpp"
#include <cstring>

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

class Tof : public Subsystem
{
    public:
        int init() override;
        void idle() override;
        void stillCapture() override;
        void videoCapture() override;
        void deinit() override;

        const TofFrame* requestPreviewFrame() override;

    protected:
        void acquisitionLoop() override;

    private:
        Arducam::ArducamTOFCamera tof_;
        std::unique_ptr<TofFrame> latest_frame_ = std::make_unique<TofFrame>();
        std::condition_variable preview_cv_;
        std::mutex latest_frame_mutex_;
        TofFrame preview_buffer_;
        bool new_preview_frame_ = false;
        const int MAX_DISTANCE_ = 4000;
        const int MAX_WIDTH_ = 240;
        const int MAX_HEIGHT_ = 180;
        int max_range_ = 0;
        const int FRAME_SIZE_ = MAX_HEIGHT_ * MAX_WIDTH_;
};

int Tof::init()
{
    if (tof_.open(Arducam::Connection::CSI, 8)) //8 when both RGB camera and ToF are connected via CSI
    {
        return 1;
    }
    if (tof_.start(Arducam::FrameType::DEPTH_FRAME))
    {
        return 1;
    }
    tof_.setControl(Arducam::Control::RANGE, MAX_DISTANCE_);
    tof_.getControl(Arducam::Control::RANGE, &max_range_);
    return 0;
}

void Tof::acquisitionLoop()
{
    Arducam::ArducamFrameBuffer* new_frame = tof_.requestFrame(200);
    std::chrono::steady_clock::time_point new_timestamp = std::chrono::steady_clock::now();
    if (new_frame == nullptr)
    {
        return;
    }
    float* depth_ptr = (float*)new_frame->getData(Arducam::FrameType::DEPTH_FRAME);
    float* confidence_ptr = (float*)new_frame->getData(Arducam::FrameType::CONFIDENCE_FRAME);
    //Because the bloody function returns a pointer to a buffer, I need 
    //to copy to a persistent data structure so I can release the buffer
    std::vector<float> temp_depth_buffer(FRAME_SIZE_);
    std::vector<float> temp_confidence_buffer(FRAME_SIZE_);
    memcpy(temp_depth_buffer.data(), depth_ptr, FRAME_SIZE_*sizeof(float));
    memcpy(temp_confidence_buffer.data(), confidence_ptr, FRAME_SIZE_*sizeof(float));
    {
        std::lock_guard<std::mutex> lock(latest_frame_mutex_);
        latest_frame_->depth_data_ = std::move(temp_depth_buffer);
        latest_frame_->confidence_data_ = std::move(temp_confidence_buffer);
        latest_frame_->timestamp_ = new_timestamp;
        new_preview_frame_ = true;
    }
    preview_cv_.notify_one();
    tof_.releaseFrame(new_frame);
}

const TofFrame* Tof::requestPreviewFrame()
{
    {
        std::unique_lock<std::mutex> lock(latest_frame_mutex_);
        preview_cv_.wait(lock, [this] {return new_preview_frame_;});
        //preview does not need anything except depth data
        memcpy(preview_buffer_.depth_data_.data(), latest_frame_->depth_data_.data(), latest_frame_->depth_data_.size()*sizeof(float));
        new_preview_frame_ = false;
        return &preview_buffer_;
    }
}