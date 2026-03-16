//#ifdef _DEBUG
//#pragma comment(lib, "opencv_world4110d.lib")
//#else
//#pragma comment(lib, "opencv_world4110.lib")
//#endif

#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <filesystem>
#include <ctime>
#include <windows.h>

using namespace cv;
using namespace std;

// Configuration
int RECORD_SECONDS = 0;
string STREAM_URL;
string NAME;
string VIDEO_LENGTH_MIN;
const string OUTPUT_DIR = "saves\\";
const int MAX_QUEUE_SIZE = 60;
const int DEFAULT_FPS = 25;
bool MULTITHREAD = false;

// Codec settings
const string CODEC = "mp4v";
const string CONTAINER = ".mp4";

// Thread control
atomic<bool> running(true);
queue<Mat> frame_buffer;
mutex buffer_mutex;

std::atomic<bool> stop_requested(false);

BOOL WINAPI console_handler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
        stop_requested.store(true);
    cout << NAME << ": starting shut down process" << endl;
    return TRUE;
}

void frame_producer(VideoCapture& cap)
{
    Mat frame;
    while (running.load())
    {
        if (!cap.read(frame) || frame.empty())
        {
            cout << NAME << ": ERROR! Frame capture failed." << endl;
            break;
        }

        {
            lock_guard<mutex> lock(buffer_mutex);
            if (frame_buffer.size() >= MAX_QUEUE_SIZE)
                frame_buffer.pop();
            frame_buffer.push(frame.clone());
        }
    }
}

void frame_consumer(VideoWriter& writer)
{
    Mat frame;
    while (running.load() || !frame_buffer.empty())
    {
        {
            lock_guard<mutex> lock(buffer_mutex);
            if (!frame_buffer.empty())
            {
                frame = frame_buffer.front();
                frame_buffer.pop();
            }
        }

        if (!frame.empty())
        {
            writer.write(frame);
            frame.release();
        }
        else
        {
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    }
}

void record_video(const string& filename)
{
    VideoCapture cap(STREAM_URL, CAP_FFMPEG);
    if (!cap.isOpened())
    {
        cout << NAME << ": ERROR! Failed to open stream." << endl;
        return;
    }

    // Check FPS
    double fps = cap.get(CAP_PROP_FPS);
    cout << NAME << ": FPS: " << fps << endl;

    if (fps <= 0 || fps > 100)
    {
        cout << NAME << ": Invalid FPS (" << fps << "), using default: " << DEFAULT_FPS << endl;
        fps = DEFAULT_FPS;
    }

    // Check resolution
    int w = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int h = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
    cout << NAME << ": Resolution: " << w << "x" << h << endl;
    if (w <= 0 || h <= 0)
    {
        cout << NAME << ": Invalid resolution: " << w << "x" << h << endl;
        return;
    }

    // Writer configuration
    VideoWriter writer;
    bool init_writer = writer.open(
        filename,
        VideoWriter::fourcc(CODEC[0], CODEC[1], CODEC[2], CODEC[3]),
        fps,
        Size(w, h)
    );

    if (!init_writer)
    {
        cout << NAME << ": ERROR! Failed to initialize video writer." << endl;
        return;
    }

    cout << endl;

    if (!MULTITHREAD)
    {
        int frame_count = 0;
        int max_frames = static_cast<int>(fps * RECORD_SECONDS);
        Mat frame;
        while (frame_count < max_frames && !stop_requested)
        {
            cap >> frame;

            if (frame.empty())
            {
                cout << NAME << " error: Failed to grab frame." << endl;
                break;
                // frame = cv::Mat::zeros(h, w, CV_8UC3);
            }

            writer.write(frame);
            frame_count++;
        }
    }
    else
    {
        running.store(true);
        thread producer_thread(frame_producer, ref(cap));
        thread consumer_thread(frame_consumer, ref(writer));

        this_thread::sleep_for(chrono::seconds(RECORD_SECONDS));
        running.store(false);
        producer_thread.join();
        consumer_thread.join();
    }

    cap.release();
    writer.release();
}

int main(int argc, char* argv[])
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT); // turn off cv output

    SetConsoleCtrlHandler(console_handler, TRUE);

    STREAM_URL = argv[1];
    NAME = argv[2];
    VIDEO_LENGTH_MIN = argv[3];
    RECORD_SECONDS = stoi((VIDEO_LENGTH_MIN)) * 60;
    //cout << VIDEO_LENGTH_MIN<< " <-min | sec-> " << RECORD_SECONDS << endl;

    cout << NAME << ": Start setup" << endl;
    if (argc < 3)
    {
        cout << NAME << "Usage: CameraWorker.exe <stream_url> <camera_name> <video_length_min>" << endl;
        return 1;
    }

    std::filesystem::create_directories(OUTPUT_DIR + NAME + "\\");

    // Create filename
    time_t now = time(nullptr);
    struct tm tim;
    localtime_s(&tim, &now);
    char fname[256];
    strftime(fname, sizeof(fname), "%Y%m%d_%H%M%S", &tim);
    const string output_path = OUTPUT_DIR + NAME + "\\" + NAME + "_" + fname + CONTAINER;

    cout << NAME << ": start recording." << endl;
    record_video(output_path);
    cout << NAME << ": end recording." << endl << endl;

    return 0;
}
