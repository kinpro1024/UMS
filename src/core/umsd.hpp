#pragma once

namespace ums
{
    class UmsDaemon
    {
        public:
            enum class State
            {
                IDLE,
                STILL_CAPTURE,
                VIDEO_CAPTURE
            };

        UmsDaemon()
            : state_(State::IDLE)
            {}

        void setState(State new_state);
        State getState() const;

        private:
            State state_;
    };
}