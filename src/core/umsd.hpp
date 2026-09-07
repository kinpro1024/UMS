
#pragma once

#include "subsystem_manager.hpp"
#include "state.hpp"

namespace ums
{
    class UmsDaemon
    {
        public:
        UmsDaemon()
        {
            state_ = State::IDLE;
            subsystem_manager_ = std::make_unique<SubsystemManager>();
            subsystem_manager_->setAllSubsystems(state_);
        }

        void setGlobalState(State new_state);
        State getGlobalState() const;
        Thermal& getThermalRefFromManager();

        private:
            State state_;
            std::unique_ptr<SubsystemManager> subsystem_manager_;
    };
}