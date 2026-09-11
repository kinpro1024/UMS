
#pragma once

namespace ums
{
    enum class State
    {
        IDLE,
        STILL_CAPTURE, //In hindsight this should have been an event, but hey
        VIDEO_CAPTURE
    };
}