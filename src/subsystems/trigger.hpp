
#pragma once

#include <gpiod.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

#include "subsystem.hpp"

namespace ums
{
    class Trigger : public Subsystem
    {
        public:
            enum class ButtonPressType
            {
                FREE,
                SHORT,
                LONG
            };

            Trigger()
            {
                Params trigger_params_{false, false, false, 0};
                setParams(trigger_params_);

                std::cout << "TRIGGER______" << std::endl;

                gpiod_chip* chip = gpiod_chip_open("/dev/gpiochip0");
                gpiod_line_settings* settings = gpiod_line_settings_new();

                gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
                gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

                unsigned int offset[] = {trigger_pin_, safety_pin_};

                gpiod_line_config* config = gpiod_line_config_new();

                gpiod_line_config_add_line_settings(config, offset, 2, settings);

                request_ = gpiod_chip_request_lines(chip, nullptr, config);

                startAcquisitionMachinery();
            }

            ~Trigger()
            {
                stopAcquisitionMachinery();
            }

            ButtonPressType getCurrentButtonPressType();

        protected:
            std::unique_ptr<Frame> acquireLatestFrame() override;
            void copyToPreviewBuffer(Frame* preview) override;

        private:
            unsigned int trigger_pin_ = 23;
            unsigned int safety_pin_ = 24;

            gpiod_line_request* request_;

            std::chrono::steady_clock::time_point button_press_start_;
            std::chrono::steady_clock::time_point button_press_end_;
            std::chrono::steady_clock::time_point button_press_duration_checkpoint_;

            std::atomic<ButtonPressType> curr_press_type_{ButtonPressType::FREE};
            std::atomic<bool> flip_flag_{false};

            bool last_flip_flag_ = false;
            bool button_pressed_ = false;
            bool last_button_pressed_ = false;
    };
}