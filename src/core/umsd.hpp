
#pragma once

#include "state.hpp"
#include "subsystem.hpp"
#include "thermal.hpp"
#include "tof.hpp"

namespace ums
{
    class UmsDaemon
    {
        public:
        UmsDaemon()
            :state_(State::IDLE)
        {
            //Constructor creates all subsystems and appends them to active_subsystems_
            //any new additions to Subsystem MUST BE REFLECTED HERE
            active_subsystems_.push_back(std::make_unique<Thermal>());
            active_subsystems_.push_back(std::make_unique<Tof>());
            setGlobalState(state_);
        }

        void setGlobalState(State new_state);
        State getGlobalState() const;
        Thermal& getThermalRef();
        Tof& getTofRef();

        private:
            State state_;
            std::vector<std::unique_ptr<Subsystem>> active_subsystems_;
    };
}