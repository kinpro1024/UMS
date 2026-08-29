#pragma once

#include <chrono>

class Subsystem
{
    public:
        class Frame
        {
            public:
                virtual ~Frame() = default;
        };
        virtual ~Subsystem() = default;
        //These functions define how each subsystem behaves when running
        virtual int init() = 0;
        virtual void idle() = 0;
        virtual void stillCapture() = 0;
        virtual void videoCapture() = 0;
        virtual void deinit() = 0;
        //This is the function used by the preview thread to acquire
        //frames from the subsystems with synchronization, didn't feel
        //the need to implement a timeout as well, maybe should have...
        virtual const Frame* requestPreviewFrame() = 0;

    protected:
        //INVARIANT: Acqusition runs constantly and updates each subsystem's local buffer,
        //i.e. at the end of acquisitionLoop() each class' latest_frame_ of $Subsystem$Frame 
        //datatype SHOULD HAVE OWNERSHIP.
        //
        //EXCEPTION: Frontend Subsystem copies to a local buffer latest_$subsystem$_frame_
        //before releasing the subsystem buffer but DOES NOT HAVE OWNERSHIP AT ANY POINT.
        virtual void acquisitionLoop() = 0;
        //GENERAL saveFrame() is the generalised implementation of a common worker and
        //is responsible for the destruction of the frame recieved as OWNERSHIP IS
        //TRANSFERRED and memory is freed at end of scope.
        //
        //EXCEPTIONS:
        //1. RGB subsystem has its own save implementation in rpicam-vid.
        //2. Frontend an Trigger subsystems don't need this.
        void saveFrame(std::string file_path, std::unique_ptr<Frame> frame);
};