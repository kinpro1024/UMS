#pragma once

#include "umsd.hpp"


namespace ums {
    class Subsystem
    {
        public:
            class Frame
            {
                public:
                    virtual ~Frame() = default;
            };
            virtual ~Subsystem() = default;
            //This function is used by UmsDaemon to communicate events to
            //all the Subsystems
            virtual void setState(ums::UmsDaemon::State) = 0;
            //This is the function used by the preview thread to acquire
            //frames from the subsystems with synchronization, didn't feel
            //the need to implement a timeout as well, maybe should have...
            virtual const Frame* acquirePreviewFrame() = 0;

        protected:
            //INVARIANT: Acqusition runs constantly once constructor initialises its thread and 
            //updates each subsystem's local buffer, i.e. at the end of acquisitionLoop() each 
            //class' latest_frame_ of $Subsystem$Frame datatype SHOULD HAVE OWNERSHIP.
            //
            //EXCEPTION: Frontend Subsystem copies to a local buffer latest_$subsystem$_frame_
            //before releasing the subsystem buffer but DOES NOT HAVE OWNERSHIP AT ANY POINT.
            virtual void acquisitionLoop() = 0;
            //STATE: stateExecution() is internally called by acquisition loop to proceed to one
            //3 paths:
            //1. IDLE : fillPreview() only.
            //2. STILL_CAPTURE : saveFrame() only with some sync.
            //3. VIDEO_CAPTURE : fillPreview() with quantisedEnqueue() and thread(writerWorker) spun up.
            virtual void stateExecution(ums::UmsDaemon::State state) = 0;
            //Copies latest_frame_ data into subsystem managed preview_buffer_.
            //
            //EXCEPTIONS: Does not apply to Frontend and Trigger Subsystems.
            virtual void fillPreview() = 0;
            //Quantises and enqueues the latest frame in the subsystem managed ring buffer.
            //
            //EXCEPTIONS: Does not apply to Frontend and Trigger Subsystems.
            virtual void quantisedEnqueue() = 0;
            //GENERAL: saveFrame() is the generalised implementation of a common worker and
            //is responsible for the destruction of the frame recieved as OWNERSHIP IS
            //TRANSFERRED and memory is freed at end of scope.
            //
            //EXCEPTIONS:
            //1. RGB and mic subsystems have their own save implementation in seperate apps.
            //2. Frontend an Trigger subsystems don't need this.
            void saveFrame(std::string file_path, std::unique_ptr<Frame> frame);
    };
}