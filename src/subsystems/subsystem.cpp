#include "subsystem.hpp"

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
    if (current_state_.load() == ums::UmsDaemon::State::IDLE && state == ums::UmsDaemon::State::STILL_CAPTURE)
    {
        ;
    }
    else if (current_state_.load() == ums::UmsDaemon::State::STILL_CAPTURE && state == ums::UmsDaemon::State::IDLE)
    {
        ;
    }
    else if (current_state_.load() == ums::UmsDaemon::State::IDLE && state == ums::UmsDaemon::State::VIDEO_CAPTURE)
    {
        writer_thread_(&writerWorker, this);
    }
    else if (current_state_.load() == ums::UmsDaemon::State::VIDEO_CAPTURE && state == ums::UmsDaemon::State::IDLE)
    {
        abort_writer_worker_ = true;
        writer_thread_.join;
    }
}

void ums::Subsystem::setParams(Params params)
{
    writer_buffer_.resize(params.buffer_size_);
    subsystem_params_ = params;
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
    while (!abort_writer_worker_ && buffer_occupancy_ != 0)
    {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        buffer_cv_.wait(lock, [this]{return abort_writer_worker_ || buffer_occupancy_ == 0;});
        if(buffer_occupancy_ == 0 && abort_writer_worker_)
        {
            break;
        }
        saveFrame(std::move(writer_buffer_[head_]));
        head_ = (head_ + 1) % writer_buffer_.size();
        --buffer_occupancy_;
    }
}

void ums::Subsystem::stateExecution()
{
    std::unique_ptr<Frame> frame;
    switch (current_state_.load())
    {
        case ums::UmsDaemon::State::IDLE:
            fillPreview();
            break;

        case ums::UmsDaemon::State::STILL_CAPTURE:
            frame = std::move(latest_frame_);
            saveFrame(std::move(frame));
            break;
        
        case ums::UmsDaemon::State::VIDEO_CAPTURE:
            fillPreview();
            frame = prepareFrame(std::move(latest_frame_));
            bufferEnqueue(std::move(frame));
            break;

        default:
            break;
    }
}