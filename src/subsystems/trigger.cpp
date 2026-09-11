
#include "trigger.hpp"

#include <iostream>

std::unique_ptr<ums::Subsystem::Frame> ums::Trigger::acquireLatestFrame()
{
    gpiod_line_value curr[2];
    gpiod_line_request_get_values(request_, curr);

    //curr is in order the handle put it in
    if (!curr[1])
    {
        button_pressed_ = false;
        curr_press_type_.store(ButtonPressType::FREE);
    }

    else
    {
        button_pressed_ = !curr[0];
    }

    if (!last_button_pressed_ && button_pressed_ && curr_press_type_.load() == ButtonPressType::FREE)
    {
        button_press_start_ = std::chrono::steady_clock::now();
    }

    else if (last_button_pressed_ && button_pressed_)
    {
        button_press_duration_checkpoint_ = std::chrono::steady_clock::now();

        auto elapsed_press_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(button_press_duration_checkpoint_-button_press_start_).count();

        if (elapsed_press_time_ > 500)
        {
            curr_press_type_.store(ButtonPressType::LONG);
        }
    }

    else if (last_button_pressed_ && !button_pressed_)
    {
        button_press_end_ = std::chrono::steady_clock::now();

        auto total_press_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(button_press_duration_checkpoint_-button_press_start_).count();

        if (total_press_time_ <= 500 && curr_press_type_.load() == ButtonPressType::FREE)
        {
            curr_press_type_.store(ButtonPressType::SHORT);
        }

        else if (curr_press_type_.load() == ButtonPressType::LONG)
        {
            curr_press_type_.store(ButtonPressType::FREE);
        }

    }

    std::cout << "pressed: " <<static_cast<int>(curr_press_type_.load()) << std::endl;

    last_button_pressed_ = button_pressed_;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    //let aqThread loop, this function exists so thread ownership is centralised in Subsystem    
    return nullptr;
}

ums::Trigger::ButtonPressType ums::Trigger::getCurrentButtonPressType()
{
    if(curr_press_type_.load() == ButtonPressType::SHORT)
    {
        return curr_press_type_.exchange(ButtonPressType::FREE);
    }

    return curr_press_type_.load();
}

//dummy because pure virtual
void ums::Trigger::copyToPreviewBuffer(Frame* preview) {}