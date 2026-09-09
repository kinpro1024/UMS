
#include "umsd.hpp"

//==========================================================================================================================
//MASTER FUNCTIONS
//==========================================================================================================================

void ums::UmsDaemon::setGlobalState(ums::State new_state)
{
    for (const auto& active_subsystem_ : active_subsystems_)
    {
        active_subsystem_->setState(new_state);
    }

    state_ = new_state;
}

//--------------------------------------------------------------------------------------------------------------------------

ums::State ums::UmsDaemon::getGlobalState() const
{
    return state_;
}

//--------------------------------------------------------------------------------------------------------------------------

ums::Thermal& ums::UmsDaemon::getThermalRef()
{
    //Because std::vector<std::unique_ptr<ums::Subsystem>>, it upcasts to Subsystem&
    ums::Thermal& ref = static_cast<ums::Thermal&>(*active_subsystems_[0]);
    return ref;
}

//--------------------------------------------------------------------------------------------------------------------------

ums::Tof& ums::UmsDaemon::getTofRef()
{
    //Because std::vector<std::unique_ptr<ums::Subsystem>>, it upcasts to Subsystem&
    ums::Tof& ref = static_cast<ums::Tof&>(*active_subsystems_[1]);
    return ref;
}

//--------------------------------------------------------------------------------------------------------------------------