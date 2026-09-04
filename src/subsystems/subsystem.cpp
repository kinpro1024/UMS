#include "subsystem.hpp"


                                                                                    /*PUBLIC STARTS HERE*/

void ums::Subsystem::setState(ums::UmsDaemon::State state)
{
    if (current_state_.load() == state)
    {
        return;
    }
    handleStateTransition(state);
    last_state_ = current_state_.exchange(state);
}

void ums::Subsystem::handleStateTransition(ums::UmsDaemon::State state)
{
    ums::UmsDaemon::State curr_state_ = current_state_.load();
    if (curr_state_ == ums::UmsDaemon::State::IDLE && state == ums::UmsDaemon::State::STILL_CAPTURE)
    {
        if (subsystem_params_.supports_still_)
        {
            ;
        }
        else
        {
            customStillPipelineTrigger();
        }
    }
    else if (curr_state_ == ums::UmsDaemon::State::STILL_CAPTURE && state == ums::UmsDaemon::State::IDLE)
    {
        ;
    }
    else if (curr_state_ == ums::UmsDaemon::State::IDLE && state == ums::UmsDaemon::State::VIDEO_CAPTURE)
    {
        if (subsystem_params_.supports_video_)
        {
            abort_writer_worker_ = false;
            writer_thread_ = std::thread(&writerWorker, this);
        }
        else
        {
            customVideoPipelineStart();
        }
    }
    else if (curr_state_ == ums::UmsDaemon::State::VIDEO_CAPTURE && state == ums::UmsDaemon::State::IDLE)
    {
        if (subsystem_params_.supports_video_)
        {
            abort_writer_worker_ = true;
            writer_thread_.join();
        }
        else
        {
            customVideoPipelineStop();
        }
    }
}

const ums::Subsystem::Frame* ums::Subsystem::acquirePreviewFrame()
{
    {
        std::unique_lock<std::mutex> lock(preview_mutex_);
        preview_cv_.wait(lock, [this] {return new_preview_frame_;});
        new_preview_frame_ = false;
        //HERE ENDS UNIQUE LOCK SCOPE!!!!
    }
    //THIS LOCKS THE ACTUAL BUFFER FOR FRONTEND until releasePreviewFrame() runs and unlocks it.
    preview_mutex_.lock();
    return preview_buffer_;
}

void ums::Subsystem::releasePreviewFrame()
{
    preview_mutex_.unlock();
}

                                                                                    /*PROTECTED STARTS HERE*/

void ums::Subsystem::setParams(Params params)
{
    writer_buffer_.resize(params.buffer_size_);
    subsystem_params_ = params;
}

void ums::Subsystem::acquisitionLoop()
{
    while(!abort_acquisition_loop_)
    {
        latest_frame_ = acquireLatestFrame();
        if(!latest_frame_)
        {
            continue;
        }
        stateExecution();
    }
}

std::unique_ptr<ums::Subsystem::Frame> ums::Subsystem::prepareFrame(std::unique_ptr<ums::Subsystem::Frame> frame)
{
    return frame;
}

void ums::Subsystem::bufferEnqueue(std::unique_ptr<Frame> prepared_frame)
{
    {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        if(buffer_occupancy_ == writer_buffer_.size())
        {
            ; //Drop frame
        }
        else
        {
            writer_buffer_[tail_] = std::move(prepared_frame);
            tail_ = (tail_ + 1) % writer_buffer_.size();
            ++buffer_occupancy_;
        }
    }
    buffer_cv_.notify_one();
}

void ums::Subsystem::writerWorker()
{
    while (!abort_writer_worker_)
    {
        {
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            buffer_cv_.wait(lock, [this]{return abort_writer_worker_ || buffer_occupancy_ != 0;});
            if(buffer_occupancy_ == 0 && abort_writer_worker_)
            {
                break;
            }
            writer_worker_buffer_ = std::move(writer_buffer_[head_]);
            head_ = (head_ + 1) % writer_buffer_.size();
            --buffer_occupancy_;
        }
        saveFrame(std::move(writer_worker_buffer_));
    }
}

void ums::Subsystem::fillPreview(ums::Subsystem::Frame* preview)
{
    {
        std::unique_lock<std::mutex> lock(preview_mutex_, std::try_to_lock);
        if (lock.owns_lock())
        {
            preview_buffer_ = copyToBuffer(preview);
            new_preview_frame_ = true;
        }
    }
    preview_cv_.notify_one();
}

void ums::Subsystem::stateExecution()
{
    std::unique_ptr<Frame> frame;
    switch (current_state_.load())
    {
        case ums::UmsDaemon::State::IDLE:
            fillPreview(latest_frame_.get());
            break;

        case ums::UmsDaemon::State::STILL_CAPTURE:
            if (subsystem_params_.supports_still_)
            {
                frame = std::move(latest_frame_);
                saveFrame(std::move(frame));
            }
            break;

        case ums::UmsDaemon::State::VIDEO_CAPTURE:
            fillPreview(latest_frame_.get());
            if (subsystem_params_.supports_video_)
            {
                frame = prepareFrame(std::move(latest_frame_));
                bufferEnqueue(std::move(frame));
            }
            break;

        default:
            break;
    }
}