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
            //ATOMIC: This function is used by UmsDaemon to communicate states to all the
            //Subsystems via a subsystem created BUT NOT MUTATED atomic current_state_, also
            //internally calls handleStateTransition() for spinning/joining threads and such
            //and only after handleStateTransition() releases does it update subsystem current_state_.
            virtual void setState(ums::UmsDaemon::State state) = 0;
            //FRONTEND: These are the functions used by the preview threads to acquire frames
            //from the Subsystems with synchronization mutexes that NEED TO BE RELEASED.
            //
            //WARN: MUST RUN releasePreviewFrame() AFTER acquirePreviewFrame() so buffer can be reused by Subsystem.
            virtual const Frame* acquirePreviewFrame() = 0;
            //WARN: MUST RUN releasePreviewFrame() AFTER acquirePreviewFrame() so buffer can be reused by Subsystem.
            virtual void releasePreviewFrame() = 0;

        protected:
            //STATE: called internally by setState() and must block until all internal threads
            //have spun/joined
            virtual void handleStateTransition(ums::UmsDaemon::State state);
            //INVARIANT: Acqusition runs constantly once constructor initialises its thread and 
            //updates each subsystem's local buffer, i.e. at the end of acquisitionLoop() each
            //class' latest_frame_ of $Subsystem$Frame datatype SHOULD HAVE OWNERSHIP.
            //
            //EXCEPTION: Frontend Subsystem copies to a local buffer latest_$subsystem$_frame_
            //before releasing the subsystem buffer but DOES NOT HAVE OWNERSHIP AT ANY POINT.
            virtual void acquisitionLoop() = 0;
            //STATE: stateExecution() is internally called by acquisition loop to proceed to one
            //of 3 paths based on class' current_state_ set via setState():
            //1. IDLE : fillPreview() only.
            //2. STILL_CAPTURE : saveFrame() only with some sync.
            //3. VIDEO_CAPTURE : fillPreview() with writeEnqueue().
            virtual void stateExecution() = 0;
            //Copies latest_frame_ data into subsystem managed preview_buffer_.
            //
            //EXCEPTIONS: Does not apply to Frontend and Trigger Subsystems.
            virtual void fillPreview() = 0;
            //Enqueues the latest frame in the subsystem managed ring buffer, Quantises if needed.
            //
            //EXCEPTIONS: Does not apply to Frontend and Trigger Subsystems.
            virtual void writeEnqueue() = 0;
            //THREAD: writerWorker() is always an independent thread that dequeues and saves Frame
            //to disk
            //
            //EXCEPTIONS: RGB, Mic, Frontend, and Trigger don't need this.
            virtual void writerWorker() = 0;
            //OWNERSHIP: saveFrame() is the implementation of a class writer worker and is
            //responsible for the destruction of the frame recieved as OWNERSHIP IS TRANSFERRED
            //and memory is freed at end of scope.
            //
            //EXCEPTIONS:
            //1. RGB and mic subsystems have their own save implementation in seperate apps.
            //2. Frontend an Trigger subsystems don't need this.
            virtual void saveFrame(std::unique_ptr<Frame> frame) = 0;
    };
}