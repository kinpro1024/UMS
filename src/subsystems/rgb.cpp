
#include "rgb.hpp"


std::unique_ptr<ums::Subsystem::Frame> ums::Rgb::acquireLatestFrame()
{
    std::unique_ptr<ums::Subsystem::Frame> dummy_rgb_frame_ = std::make_unique<ums::Subsystem::Frame>();

    recvAll(client_, &rgb_recieve_buffer_.timestamp_, sizeof(std::chrono::steady_clock::time_point));
    recvAll(client_, rgb_recieve_buffer_.qimage_.data(), 691200);

    //nonzero return ensures stateExecution() runs
    return dummy_rgb_frame_;
}

bool ums::Rgb::recvAll(int client, void* buffer, size_t size)
{
    size_t recieved = 0;

    while (recieved < size)
    {
        ssize_t n = recv(client, static_cast<char*>(buffer) + recieved, size - recieved, 0);

        if (n < 0)
        {
            return false;
        }

        recieved+=n;
    }

    return true;
}

void ums::Rgb::copyToPreviewBuffer(ums::Subsystem::Frame* frame)
{
    //Preview Buffer has a mutex contract in subsystem base class
    //hence, we have an extra buffer for contract preservation
    memcpy(rgb_preview_buffer_.qimage_.data(), rgb_recieve_buffer_.qimage_.data(), 691200);
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Rgb::customVideoPipelineStart(){}
void ums::Rgb::customVideoPipelineStop(){}
void ums::Rgb::customStillPipelineTrigger(){}
