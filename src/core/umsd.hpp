
#pragma once

#include "state.hpp"
#include "subsystem.hpp"
#include "thermal.hpp"
#include "tof.hpp"
#include "rgb.hpp"
#include "trigger.hpp"

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
                //
                //Try to keep the sluggish systems at the back otherwise the still_capture_done_spinlock_
                //in Subsystem may create a potential halt, although since all subsystems are on independent
                //threads the slowest wait will dominate and this comment is more for a future debug.
                active_subsystems_.push_back(std::make_unique<Rgb>());
                active_subsystems_.push_back(std::make_unique<Tof>());
                active_subsystems_.push_back(std::make_unique<Thermal>());
                active_subsystems_.push_back(std::make_unique<Trigger>());
                setGlobalState(state_);
            }

            //BECAUSE IT SEES ALL AND COMMANDEERS ITS SUBSYSTEM ARMY
            //
            //It calls getCurrentButtonPressType() and updates all the states
            //during runtime.
            void sauron();
            void abortSauron();

            void setGlobalState(State new_state);
            State getGlobalState() const;
            Thermal& getThermalRef();
            Tof& getTofRef();
            Rgb& getRgbRef();

        private:
            State state_;
            std::vector<std::unique_ptr<Subsystem>> active_subsystems_;
            std::atomic<bool> abort_sauron_{false};
    };
}