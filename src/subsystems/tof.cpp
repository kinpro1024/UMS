
#include "tof.hpp"

//==========================================================================================================================
//THERMAL - STANDARD PIPELINES
//==========================================================================================================================

std::unique_ptr<ums::Subsystem::Frame> ums::Tof::acquireLatestFrame()
{
    std::unique_ptr<TofFrame> latest_tof_frame_  = std::make_unique<TofFrame>();

    Arducam::ArducamFrameBuffer* new_frame = tof_.requestFrame(200);
    std::chrono::steady_clock::time_point new_timestamp = std::chrono::steady_clock::now();

    if (new_frame == nullptr)
    {
        return nullptr;
    }

    float* depth_ptr = (float*)new_frame->getData(Arducam::FrameType::DEPTH_FRAME);
    float* confidence_ptr = (float*)new_frame->getData(Arducam::FrameType::CONFIDENCE_FRAME);

    memcpy(latest_tof_frame_->depth_data_.data(), depth_ptr, FRAME_SIZE_*sizeof(float));
    memcpy(latest_tof_frame_->confidence_data_.data(), confidence_ptr, FRAME_SIZE_*sizeof(float));
    latest_tof_frame_->timestamp_ = new_timestamp;

    tof_.releaseFrame(new_frame);

    return latest_tof_frame_;
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Tof::copyToPreviewBuffer(ums::Subsystem::Frame* frame)
{
    TofFrame* tofframe = static_cast<TofFrame*>(frame);
    memcpy(tof_preview_buffer_.depth_data_.data(), tofframe->depth_data_.data(), tofframe->depth_data_.size()*sizeof(float));
}

//--------------------------------------------------------------------------------------------------------------------------

std::unique_ptr<ums::Subsystem::Frame> ums::Tof::prepareFrame(std::unique_ptr<Frame> frame)
{
    TofFrame* working_frame = static_cast<TofFrame*>(frame.get());
    std::unique_ptr<TofQuantFrame> quantised_frame = std::make_unique<TofQuantFrame>();

    for (int i = 0; i < FRAME_SIZE_; ++i)
    {
        quantised_frame->qdepth_data_[i] = static_cast<uint16_t>(working_frame->depth_data_[i]*10.0f);
        quantised_frame->qconfidence_data_[i] = static_cast<uint16_t>(working_frame->confidence_data_[i]*10.0f);
    }
        
    quantised_frame->qtimestamp_ = working_frame->timestamp_;

    return quantised_frame;
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Tof::saveFrame(std::unique_ptr<Frame> frame, ums::State state)
{
    if (state == State::VIDEO_CAPTURE)
    {
        //Use this for the save and let both get destroyed at end of scope as unique_ptr is destroyed.
        TofQuantFrame* save_dataq = static_cast<TofQuantFrame*>(frame.get());

        // filename = "depth_$width$_$height$_uint_16_$time.raw"
        long long tstp = std::chrono::duration_cast<std::chrono::microseconds>(save_dataq->qtimestamp_.time_since_epoch()).count();

        std::string filename1 =
            "depth_" + std::to_string(MAX_WIDTH_) + "_" + std::to_string(MAX_HEIGHT_) + "_uint16_depth_" + std::to_string(tstp) + ".raw";
        std::ofstream file1(filename1, std::ios::binary);
        file1.write(reinterpret_cast<char*>(save_dataq->qdepth_data_.data()), MAX_WIDTH_ * MAX_HEIGHT_ * sizeof(uint16_t));
        file1.close();

        std::string filename2 =
            "confidence_" + std::to_string(MAX_WIDTH_) + "_" + std::to_string(MAX_HEIGHT_) + "_uint_16_confidence_" + std::to_string(tstp) + ".raw";
        std::ofstream file2(filename2, std::ios::binary);
        file2.write(reinterpret_cast<char*>(save_dataq->qconfidence_data_.data()), MAX_WIDTH_ * MAX_HEIGHT_ * sizeof(uint16_t));
        file2.close();
    }

    else if (state == State::STILL_CAPTURE)
    {
        //Use this for the save and let both get destroyed at end of scope as unique_ptr is destroyed.
        TofFrame* save_data = static_cast<TofFrame*>(frame.get());

        // filename = "depth_$width$_$height$_float_$time.raw"
        long long tstp = std::chrono::duration_cast<std::chrono::microseconds>(save_data->timestamp_.time_since_epoch()).count();

        std::string filename1 =
            "depth_" + std::to_string(MAX_WIDTH_) + "_" + std::to_string(MAX_HEIGHT_) + "_float_depth_" + std::to_string(tstp) + ".raw";
        std::ofstream file1(filename1, std::ios::binary);
        file1.write(reinterpret_cast<char*>(save_data->depth_data_.data()), MAX_WIDTH_ * MAX_HEIGHT_ * sizeof(float));
        file1.close();

        std::string filename2 =
            "confidence_" + std::to_string(MAX_WIDTH_) + "_" + std::to_string(MAX_HEIGHT_) + "_float_confidence_" + std::to_string(tstp) + ".raw";
        std::ofstream file2(filename2, std::ios::binary);
        file2.write(reinterpret_cast<char*>(save_data->confidence_data_.data()), MAX_WIDTH_ * MAX_HEIGHT_ * sizeof(float));
        file2.close();
    }

}

//--------------------------------------------------------------------------------------------------------------------------