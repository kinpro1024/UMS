
#include "subsystem_manager.hpp"

//==========================================================================================================================
//MANAGER FUNCTIONS
//==========================================================================================================================

void ums::SubsystemManager::setAllSubsystems(ums::State state)
{
    for (const auto& active_subsystem_ : active_subsystems_)
    {
        active_subsystem_->setState(state);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

ums::Thermal& ums::SubsystemManager::getThermalRef()
{
    ums::Thermal& ref = static_cast<ums::Thermal&>(*active_subsystems_[0]);
    return ref;
}