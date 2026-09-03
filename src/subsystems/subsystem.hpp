#pragma once

#include <mutex>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <condition_variable>
#include <atomic>

#include "umsd.hpp"


namespace ums {
    class Subsystem
    {
        public:
            class Params
            {
                public:
                    bool supports_preview_ = false;
                    bool supports_still_ = false;
                    bool supports_video_ = false;
                    uint16_t buffer_size_;
            };
            class Frame
            {
                public:
                    virtual ~Frame() = default;
            };
            virtual ~Subsystem() = default;
            //ATOMIC: This function is used by UmsDaemon to communicate states to all the
            //Subsystems via an atomic current_state_, also internally calls handleStateTransition()
            //for spinning/joining threads and such and only after handleStateTransition()
            //releases does it update subsystem current_state_.
            void setState(ums::UmsDaemon::State state);
            //FRONTEND: These are the functions used by the preview threads to acquire frames
            //from the Subsystems with synchronization mutexes that NEED TO BE RELEASED.
            //
            //WARN: MUST RUN releasePreviewFrame() AFTER acquirePreviewFrame() so buffer can be reused by Subsystem.
            virtual const Frame* acquirePreviewFrame() = 0;
            //WARN: MUST RUN releasePreviewFrame() AFTER acquirePreviewFrame() so buffer can be reused by Subsystem.
            virtual void releasePreviewFrame() = 0;

        protected:
            //PARAMS: Sets parameters for optional paths as general implementation is universal.
            void setParams(Params params);
            //STATE: called internally by setState() and must block until all internal threads
            //have spun/joined.
            void handleStateTransition(ums::UmsDaemon::State state);
            //INVARIANT: Acqusition runs constantly once constructor initialises its thread and 
            //updates each subsystem's local buffer, i.e. at the end of acquisitionLoop() each
            //class' latest_frame_ of Frame datatype SHOULD HAVE OWNERSHIP.
            //
            //EXCEPTION: Frontend Subsystem copies to a local buffer latest_$subsystem$_frame_
            //before releasing the subsystem buffer but DOES NOT HAVE OWNERSHIP AT ANY POINT.
            virtual void acquisitionLoop() = 0;
            //STATE: stateExecution() is internally called by acquisition loop to proceed to one
            //of 3 paths based on class' current_state_ set via setState():
            //1. IDLE : fillPreview() only.
            //2. STILL_CAPTURE : saveFrame() only with some sync.
            //3. VIDEO_CAPTURE : fillPreview() with writeEnqueue().
            void stateExecution();
            //Copies latest_frame_ data into subsystem managed preview_buffer_.
            //
            //EXCEPTIONS: Does not apply to Frontend and Trigger Subsystems.
            virtual void fillPreview() = 0;
            //Enqueues the latest frame in the subsystem managed ring buffer, .
            //
            //EXCEPTIONS: Does not apply to Frontend and Trigger Subsystems.
            void bufferEnqueue(std::unique_ptr<Frame> prepared_frame);
            //TRANSFORM: Prepares frame for enqueue (any quantisation or transformation) and DESTROYS
            //ORIGINAL FRAME.
            virtual std::unique_ptr<Frame> prepareFrame(std::unique_ptr<Frame> frame) = 0;
            //THREAD: writerWorker() is always an independent thread that dequeues and saves Frame
            //to disk
            //
            //EXCEPTIONS: RGB, Mic, Frontend, and Trigger don't need this.
            void writerWorker();
            //OWNERSHIP: saveFrame() is the implementation of a class writer worker and is
            //responsible for the destruction of the frame recieved as OWNERSHIP IS TRANSFERRED
            //and memory is freed at end of scope.
            //
            //EXCEPTIONS:
            //1. RGB and mic subsystems have their own save implementation in seperate apps.
            //2. Frontend an Trigger subsystems don't need this.
            virtual void saveFrame(std::unique_ptr<Frame> frame) = 0;

        private:
            ums::UmsDaemon::State last_state_;
            std::atomic<UmsDaemon::State> current_state_{UmsDaemon::State::IDLE};
            std::unique_ptr<Frame> latest_frame_  = std::make_unique<Frame>();
            std::condition_variable buffer_cv_;
            std::mutex buffer_mutex_;
            std::vector<std::unique_ptr<Frame>> writer_buffer_;
            uint16_t head_ = 0;
            uint16_t tail_ = 0;
            uint16_t buffer_occupancy_ = 0;
            std::thread writer_thread_;
            Params subsystem_params_;
            bool new_preview_frame_ = false;
            bool abort_writer_worker_ = false;
    };
}