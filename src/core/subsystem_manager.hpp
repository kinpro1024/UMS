
#pragma once

#include <iostream>
#include "subsystem.hpp"
#include "thermal.hpp"

namespace ums
{
    class SubsystemManager
    {
        public:
            //Constructor creates all subsystems and appends them to active_subsystems_
            //any new additions to Subsystem MUST BE REFLECTED HERE
            SubsystemManager()
            {
                active_subsystems_.push_back(std::make_unique<Thermal>());
                std::cout << "somehow this happened, if this did, do the belly rubbing and head tapping" << std::endl;
            }

            void setAllSubsystems(State state);

            private:
                std::vector<std::unique_ptr<Subsystem>> active_subsystems_;
    };
}
