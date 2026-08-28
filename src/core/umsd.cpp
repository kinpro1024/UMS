#include "umsd.hpp"

void ums::UmsDaemon::setState(State new_state)
{
    state_ = new_state;
}

ums::UmsDaemon::State ums::UmsDaemon::getState() const
{
    return state_;
}