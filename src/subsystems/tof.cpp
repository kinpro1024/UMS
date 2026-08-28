#include <mutex>
#include "subsystem.hpp"
#include "ArducamTOFCamera.hpp"
#include <cstring>

//The TofFrame class only exists because ArducamFrameBuffer* does not have a timestamp,
//the class is merely an envelope for consitency and coherence

class TofFrame
{
    public:
        std::vector<float> depth_data;
        std::vector<float> confidence_data;
        std::chrono::steady_clock::time_point timestamp;

        TofFrame()
        : depth_data(240*180),
          confidence_data(240*180)
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

        const Subsystem::Frame* acquire() override;
        void release() override;

    private:
        Arducam::ArducamTOFCamera tof_;
        std::unique_ptr<TofFrame> latest_frame_;
        std::mutex tof_preview_buffer_mutex_;
        Subsystem::Frame tof_payload_;
        const uint8_t TOF_ID_ = 3;
        const int MAX_DISTANCE_ = 4000;
        const int MAX_WIDTH_ = 240;
        const int MAX_HEIGHT_ = 180;
        int max_range_ = 0;
        const int FRAME_SIZE_ = MAX_HEIGHT_ * MAX_WIDTH_;
};

int Tof::init()
{
    if (tof_.open(Arducam::Connection::CSI, 8))
    //8 when both RGB camera and ToF are connected via CSI
    {
        return 1;
    }
    if (tof_.start(Arducam::FrameType::DEPTH_FRAME))
    {
        return 1;
    }
    tof_.setControl(Arducam::Control::RANGE, MAX_DISTANCE_);
    tof_.getControl(Arducam::Control::RANGE, &max_range_);
}

void Tof::idle()
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
        std::lock_guard<std::mutex> lock(tof_preview_buffer_mutex_);
        latest_frame_->depth_data = std::move(temp_depth_buffer);
        latest_frame_->confidence_data = std::move(temp_confidence_buffer);
        latest_frame_->timestamp = new_timestamp;
        tof_.releaseFrame(new_frame);
    }
}

const Subsystem::Frame* Tof::acquire()
{
    tof_preview_buffer_mutex_.lock();
    tof_payload_.data_ = latest_frame_.get();
    tof_payload_.subsys_id_ = TOF_ID_;
    return &tof_payload_;
}

void Tof::release()
{
    tof_preview_buffer_mutex_.unlock();
}