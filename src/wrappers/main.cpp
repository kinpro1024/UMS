#include "thermal_wrapper.hpp"
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

#define BUFFER_SIZE 120

typedef std::chrono::steady_clock::time_point TimeType;

const int max_width = 32;
const int max_height = 24;
const int max_range = 0;
constexpr int frame_size = max_height*max_width;

static size_t head;
static size_t tail;
static size_t count;

static int requested_frames = 0;
static int acquired_frames = 0;
static int written_frames = 0;

static bool capture_finished = false;

static long long max_capture_time = 0;
static long long min_capture_time = 9999999;
static long long average_capture_time = 0;
static long long total_capture_time = 0;

static long long max_fetch_time = 0;
static long long min_fetch_time = 9999999;
static long long average_fetch_time = 0;
static long long total_fetch_time = 0;

static long long max_write_time = 0;
static long long min_write_time = 9999999;
static long long average_write_time = 0;
static long long total_write_time = 0;

class StoredFrame {
public:
    std::vector<uint16_t> temperatures_quantised;
    TimeType timestamp;
    
    StoredFrame()
    : temperatures_quantised(frame_size)
    {}
};


typedef struct {
    TimeType start_capture;
    TimeType start_write;
    TimeType fetch_flag;
    TimeType finish_write;
    TimeType finish_capture;
} Timing;

std::mutex buffer_mutex;
std::condition_variable buffer_cv;
std::vector<StoredFrame> buffer;
std::vector<Timing> timings;

StoredFrame writer_buffer;

bool continue_condition() {
    return count > 0 || capture_finished;
}

void save_image(uint16_t* image1, int width, int height,  TimeType now)
{
    using namespace std::literals;
    // filename = "temps_$width$_$height$_uint_16_$time.raw"
    long long tstp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    std::string filename1 =
        "temps_" + std::to_string(width) + "_" + std::to_string(height) + "_uint16_" + std::to_string(tstp) + ".raw";
    std::ofstream file1(filename1, std::ios::binary);
    file1.write(reinterpret_cast<char*>(image1), width * height * sizeof(uint16_t));
    file1.close();

    ++written_frames;
}

void writer_worker() {
    while (!capture_finished) {
        std::unique_lock<std::mutex> lock(buffer_mutex);

        buffer_cv.wait(lock, continue_condition);

        if(count == 0 && capture_finished) {
            break;
        }

        writer_buffer = std::move(buffer[head]);

        head = (head + 1) % BUFFER_SIZE;
        --count;

        lock.unlock();
        
        save_image(writer_buffer.temperatures_quantised.data(), max_width, max_height, writer_buffer.timestamp);
    }
}


int main() {

    buffer.resize(BUFFER_SIZE);
    timings.reserve(3000);

    Timing curr;
    TimeType start_comms_init = std::chrono::steady_clock::now(); 

    Mlx90640 thermalcam;
    std::unique_ptr<ThermalWrapperFrame> frame;

    if (thermalcam.sensorInit(0x07)) {
        std::cerr << "Initialisation Failed!!!\n";
    } else {
        std::cout << "Camera Initialised!!\n";
    }

    TimeType finish_comms_init = std::chrono::steady_clock::now();

    std::thread writer(writer_worker);

    for (int i = 0; i < 3000; ++i) {

        curr.start_capture = std::chrono::steady_clock::now();

        frame = thermalcam.requestFullFrame(500);

        ++requested_frames;

        if (!frame) {
            std::cerr << "frame request timed out...";
            return -1;
        }

        ++acquired_frames;

        curr.fetch_flag = std::chrono::steady_clock::now();

        std::vector<uint16_t> temps_quantised(frame_size);

        for (int i = 0; i < frame_size; ++i) {
            temps_quantised[i] = static_cast<uint16_t>(frame->temperatures[i]*100.0f);
        }

        {
            std::unique_lock<std::mutex> lock(buffer_mutex);

            if(count == BUFFER_SIZE) {
                std::cout << "Buffer Full!!\n";
                lock.unlock();
                ;
            } else {
                buffer[tail].temperatures_quantised = std::move(temps_quantised);
                buffer[tail].timestamp =frame->timestamp;

                tail = (tail + 1) % BUFFER_SIZE;
                ++count;

                std::cout << "count: " << count << "\n";

                lock.unlock();
                buffer_cv.notify_one();
            }
        }
        curr.finish_capture = std::chrono::steady_clock::now();

        timings.push_back(curr);
    }

    TimeType start_comms_deinit = std::chrono::steady_clock::now();

    {
        std::unique_lock<std::mutex> lock(buffer_mutex);

        capture_finished = true;
        lock.unlock();
    }

    buffer_cv.notify_one();

    writer.join();

    TimeType finish_comms_deinit = std::chrono::steady_clock::now();

    Timing* t = timings.data();

    for (int i = 0; i < timings.size(); ++i) {

        long long elapsed_capture_time = std::chrono::duration_cast<std::chrono::microseconds>(t[i].finish_capture-t[i].start_capture).count();

        long long elapsed_write_time = std::chrono::duration_cast<std::chrono::microseconds>(t[i].finish_write-t[i].start_write).count();

        long long elapsed_fetch_time = std::chrono::duration_cast<std::chrono::microseconds>(t[i].fetch_flag-t[i].start_capture).count();

        if(i > 300) {
            if (max_capture_time < elapsed_capture_time) {
                max_capture_time = elapsed_capture_time;
            }
            if (min_capture_time > elapsed_capture_time) {
                min_capture_time = elapsed_capture_time;
            }
            if (max_write_time < elapsed_write_time) {
                max_write_time = elapsed_write_time;
            }
            if (min_write_time > elapsed_write_time) {
                min_write_time = elapsed_write_time;
            }
        }

        total_capture_time += elapsed_capture_time;
        total_write_time += elapsed_write_time;
        total_fetch_time += elapsed_fetch_time;

        if(i == timings.size() - 1){
            average_capture_time = total_capture_time/timings.size();
            average_write_time = total_write_time/timings.size();
            average_fetch_time = total_fetch_time/timings.size();
        } 

    }

    long long total_loop_time = std::chrono::duration_cast<std::chrono::microseconds>(start_comms_deinit-finish_comms_init).count();

    float fps = (static_cast<float>(acquired_frames)/total_loop_time)*1000000;

    std::cout << "maxcap: " << max_capture_time << "us\n";
    std::cout << "mincap: " << min_capture_time << "us\n";
    std::cout << "maxwrite: " << max_write_time << "us\n";
    std::cout << "minwrite: " << min_write_time << "us\n";
    std::cout << "avgcap: " << average_capture_time << "us\n";
    std::cout << "avgwrite: " << average_write_time << "us\n";
    std::cout << "avgfetch: " << average_fetch_time << "us\n";
    std::cout << "reqfrm: " << requested_frames << "frames\n";
    std::cout << "acqfrm: " << acquired_frames << "frames\n";
    std::cout << "wrtfrm: " << written_frames << "frames\n";
    std::cout << "sustained throughput: " << fps << "fps\n";

    return 0;
}
