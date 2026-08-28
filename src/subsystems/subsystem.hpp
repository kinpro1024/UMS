#pragma once

#include <chrono>

class Subsystem
{
    public:
        class Frame
        {
            public:
                void* data_;
                std::chrono::steady_clock::time_point timestamp_;
                uint8_t subsys_id_;
                //1)RGB 2)ToF 3)Thermal 4)IMU 5)Mic
        };

        virtual ~Subsystem() = default;
        //These functions define how each subsystem behaves when running
        virtual int init() = 0;
        virtual void idle() = 0;
        virtual void stillCapture() = 0;
        virtual void videoCapture() = 0;
        virtual void deinit() = 0;
        //These are functions used by the preview thread to acquire
        //frames from the subsystems with synchronization, didn't feel
        //the need to implement timeouts as well, maybe should have...
        virtual Frame* acquire() = 0;
        virtual void release() = 0;
};