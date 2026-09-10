
#include "subsystem.hpp"

//==========================================================================================================================
//STATE
//==========================================================================================================================

void ums::Subsystem::setState(ums::State state)
{
    if (current_state_.load() == state)
    {
        return;
    }
    handleStateTransition(state);
    current_state_.store(state);
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::handleStateTransition(ums::State new_state)
{
    ums::State curr_state_ = current_state_.load();

    if (curr_state_ == ums::State::IDLE && new_state == ums::State::STILL_CAPTURE)
    {
        if (subsystem_params_.supports_still_)
        {
            ; //handled by stateExecution()
        }
        else
        {
            customStillPipelineTrigger();
        }
    }

    else if (curr_state_ == ums::State::STILL_CAPTURE && new_state == ums::State::IDLE)
    {
        ;
    }

    else if (curr_state_ == ums::State::IDLE && new_state == ums::State::VIDEO_CAPTURE)
    {
        if (subsystem_params_.supports_video_)
        {
            abort_writer_worker_.store(false);
            writer_thread_ = std::thread(&ums::Subsystem::writerWorker, this);
        }
        else
        {
            customVideoPipelineStart();
        }
    }

    else if (curr_state_ == ums::State::VIDEO_CAPTURE && new_state == ums::State::IDLE)
    {
        if (subsystem_params_.supports_video_)
        {
            abort_writer_worker_.store(true);
            writer_thread_.join();
        }
        else
        {
            customVideoPipelineStop();
        }
    }
}

//==========================================================================================================================
//SETTERS (USED BY CONSTRUCTORS)
//==========================================================================================================================

void ums::Subsystem::setParams(Params params)
{
    writer_buffer_.resize(params.buffer_size_);
    subsystem_params_ = params;
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::startAcquisitionMachinery()
{
    //As I write this I am not sure if RGB will start/stop aqMachinery in the
    //same session, it just sprung to my mind and therefore the explicit false is set
    //here, else only stopAq time a flag change is warranted.
    //
    //Update: RGB uses aqMachinery to run the socket listener when custom functions are
    //responsible for switching b/w preview-only and video mode cleanly. Keeping old comment
    //for context.
    abort_acquisition_loop_.store(false);
    aq_thread_ = std::thread(&ums::Subsystem::acquisitionLoop, this);
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::stopAcquisitionMachinery()
{
    abort_acquisition_loop_.store(true);
    aq_thread_.join();
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::setPreviewBufferAddress(Frame* preview_buffer)
{
    preview_buffer_ = preview_buffer;
}

//==========================================================================================================================
//FRAME ACQUISITION MACHINERY
//==========================================================================================================================

void ums::Subsystem::acquisitionLoop()
{
    while (!abort_acquisition_loop_.load())
    {
        latest_frame_ = acquireLatestFrame();

        if (!latest_frame_)
        {
            continue;
        }

        stateExecution(current_state_.load());
    }
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::stateExecution(State state)
{
    std::unique_ptr<Frame> frame;

    switch (state)
    {
        case ums::State::IDLE:
            fillPreview(latest_frame_.get());
            break;

        case ums::State::STILL_CAPTURE:

            if (subsystem_params_.supports_still_)
            {
                frame = std::move(latest_frame_);
                saveFrame(std::move(frame), state);
            }

            break;

        case ums::State::VIDEO_CAPTURE:
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

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::fillPreview(ums::Subsystem::Frame* preview)
{
    {
        std::unique_lock<std::mutex> lock(preview_mutex_, std::try_to_lock);

        if (lock.owns_lock())
        {
            copyToPreviewBuffer(preview);
            new_preview_frame_ = true;
        }
    }

    preview_cv_.notify_one();
}

//==========================================================================================================================
//FRONTEND INTERFACE
//==========================================================================================================================

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

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::releasePreviewFrame()
{
    preview_mutex_.unlock();
}

//==========================================================================================================================
//WRITER MACHINERY
//==========================================================================================================================

std::unique_ptr<ums::Subsystem::Frame> ums::Subsystem::prepareFrame(std::unique_ptr<ums::Subsystem::Frame> frame)
{
    return frame;
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::bufferEnqueue(std::unique_ptr<Frame> prepared_frame)
{
    {
        std::unique_lock<std::mutex> lock(buffer_mutex_);

        if (buffer_occupancy_ == writer_buffer_.size())
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

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::writerWorker()
{
    while (!abort_writer_worker_)
    {
        {
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            buffer_cv_.wait(lock, [this]{return abort_writer_worker_ || buffer_occupancy_ != 0;});

            if (buffer_occupancy_ == 0 && abort_writer_worker_)
            {
                break;
            }

            writer_worker_buffer_ = std::move(writer_buffer_[head_]);
            head_ = (head_ + 1) % writer_buffer_.size();
            --buffer_occupancy_;
        }

        saveFrame(std::move(writer_worker_buffer_), State::VIDEO_CAPTURE);
    }
}

//==========================================================================================================================
//DEFAULTS FOR NON-PURE VIRTUAL FUNCTION
//==========================================================================================================================

void ums::Subsystem::saveFrame(std::unique_ptr<Frame> frame, State state) {}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::customStillPipelineTrigger() {}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::customVideoPipelineStart() {}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Subsystem::customVideoPipelineStop() {}

//--------------------------------------------------------------------------------------------------------------------------