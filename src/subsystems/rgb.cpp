
#include "rgb.hpp"


std::unique_ptr<ums::Subsystem::Frame> ums::Rgb::acquireLatestFrame()
{
    std::unique_ptr<ums::Subsystem::Frame> dummy_rgb_frame_ = std::make_unique<ums::Subsystem::Frame>();

    int client_now = client_.load();

    recvAll(client_now, &rgb_recieve_buffer_.timestamp_, sizeof(std::chrono::steady_clock::time_point));
    recvAll(client_now, rgb_recieve_buffer_.qimage_.data(), 691200);

    //nonzero return ensures stateExecution() runs
    return dummy_rgb_frame_;
}

bool ums::Rgb::recvAll(int client, void* buffer, size_t size)
{
    size_t recieved = 0;

    while (recieved < size)
    {
        ssize_t n = recv(client, static_cast<char*>(buffer) + recieved, size - recieved, 0);

        if (n <= 0)
        {
            return false;
        }

        recieved+=n;
    }

    return true;
}

void ums::Rgb::copyToPreviewBuffer(ums::Subsystem::Frame* frame)
{
    //Preview Buffer has a mutex contract in subsystem base class
    //hence, we have an extra buffer for contract preservation
    memcpy(rgb_preview_buffer_.qimage_.data(), rgb_recieve_buffer_.qimage_.data(), 691200);
}

//--------------------------------------------------------------------------------------------------------------------------

void ums::Rgb::customVideoPipelineStart()
{
    if (rpicam_pid_ > 0)
    {
        kill(rpicam_pid_, SIGUSR2);
        waitpid(rpicam_pid_, nullptr, 0);
    }

    close(client_.load());

    rpicam_pid_ = fork();

    if (rpicam_pid_ == 0)
    {
        execl("/usr/local/bin/rpicam-vid", "rpicam-vid", "--signal",
                "--width", "1280", "--height", "720", "--buffer", "20",
                "--mode", "2304:1296", "--framerate", "30",
                "--preview-backend", "umsd", "--preview-libs",
                "/home/kinpro1024/hijinks/rpicam-apps/build/preview/",
                "--codec", "h264", "--bitrate", "32000000", "-o",
                timestampedFilename(".mp4").c_str(), "-t", "0", (char*)nullptr);
    }

    client_.store(accept(sock_, nullptr, nullptr));
}

void ums::Rgb::customVideoPipelineStop()
{
    if (rpicam_pid_ > 0)
    {
        kill(rpicam_pid_, SIGUSR1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        kill(rpicam_pid_, SIGUSR2);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        kill(rpicam_pid_, SIGKILL); //there was some weird orphan process otherwise
        waitpid(rpicam_pid_, nullptr, 0);
    }

    close(client_.load());

    rpicam_pid_ = fork();

    if (rpicam_pid_ == 0)
    {
        execl("/usr/local/bin/rpicam-hello", "rpicam-hello", "--timeout", "0",
                "--preview-backend", "umsd", "--preview-libs",
                "/home/kinpro1024/hijinks/rpicam-apps/build/preview",
                (char*)nullptr);
    }

    client_.store(accept(sock_, nullptr, nullptr));
}

void ums::Rgb::customStillPipelineTrigger()
{
    // Stop current rpicam-hello.
    if (rpicam_pid_ > 0)
    {
        kill(rpicam_pid_, SIGKILL);
        waitpid(rpicam_pid_, nullptr, 0);
    }

    close(client_.load());

    // Launch one-shot still capture.
    rpicam_pid_ = fork();

    if (rpicam_pid_ == 0)
    {
        execl("/usr/local/bin/rpicam-still", "rpicam-still",
              "--preview-backend", "umsd",
              "--preview-libs",
              "/home/kinpro1024/hijinks/rpicam-apps/build/preview",
              "-o", timestampedFilename(".jpg").c_str(),
              (char*)nullptr);

        _exit(1);
    }

    // Wait for rpicam-still to connect to the preview socket.
    client_.store(accept(sock_, nullptr, nullptr));

    // Wait for the still process to finish.
    waitpid(rpicam_pid_, nullptr, 0);

    close(client_.load());

    // Restore continuous preview.
    rpicam_pid_ = fork();

    if (rpicam_pid_ == 0)
    {
        execl("/usr/local/bin/rpicam-hello", "rpicam-hello",
              "--timeout", "0",
              "--preview-backend", "umsd",
              "--preview-libs",
              "/home/kinpro1024/hijinks/rpicam-apps/build/preview",
              (char*)nullptr);

        _exit(1);
    }

    client_.store(accept(sock_, nullptr, nullptr));
}