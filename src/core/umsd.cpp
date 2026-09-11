
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
    ums::Thermal& ref = static_cast<ums::Thermal&>(*active_subsystems_[2]);
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

ums::Rgb& ums::UmsDaemon::getRgbRef()
{
    //Because std::vector<std::unique_ptr<ums::Subsystem>>, it upcasts to Subsystem&
    ums::Rgb& ref = static_cast<ums::Rgb&>(*active_subsystems_[0]);
    return ref;
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::UmsDaemon::sauron()
{
    //Because std::vector<std::unique_ptr<ums::Subsystem>>, it upcasts to Subsystem&
    ums::Trigger& trig = static_cast<ums::Trigger&>(*active_subsystems_[3]);

    while (!abort_sauron_.load())
    {
        ums::Trigger::ButtonPressType this_press = trig.getCurrentButtonPressType();

        switch (this_press)
        {
            case ums::Trigger::ButtonPressType::FREE:
                setGlobalState(ums::State::IDLE);
                break;

            case ums::Trigger::ButtonPressType::SHORT:
                setGlobalState(ums::State::STILL_CAPTURE);
                break;

            case ums::Trigger::ButtonPressType::LONG:
                setGlobalState(ums::State::VIDEO_CAPTURE);
                break;
            
        
            default:
                break;
        };

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::UmsDaemon::abortSauron()
{
    abort_sauron_.store(true);
}