#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <csignal>
#include <atomic>
#include <thread>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

const char* app_name = "CameraStart.exe";
int RECORD_TIME = 0;

struct running_cam
{
    std::string stream_link;
    std::string cam_name;
    PROCESS_INFORMATION proc_info;
};

std::vector<running_cam> currently_recording;
std::atomic<bool> stop_requested(false);

BOOL WINAPI console_handler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT)
        stop_requested.store(true);
    cout << "Controller starting shut down process" << endl;
    return TRUE;
}


void start_cam(const json& it)
{
    std::string stream_link = it["stream"];
    std::string cam_name = it["name"];

    std::string command_line = "CameraStart.exe \"" + stream_link + "\" \"" + cam_name + "\" \"" + to_string(RECORD_TIME) + "\"";

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessA(app_name, &command_line[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        std::cout << "ERROR: Failed to start process for " << cam_name << std::endl;
        return;
    }

    running_cam cam;
    cam.stream_link = stream_link;
    cam.cam_name = cam_name;
    cam.proc_info = pi;

    currently_recording.push_back(cam);
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

int main()
{
    std::cout << "Start Controller" << std::endl;

    SetConsoleCtrlHandler(console_handler, TRUE);

    std::ifstream f("./CamInfo.json");
    json data = json::parse(f);
    //std::cout << data.dump(4) << std::endl;
    
    RECORD_TIME = data["info"]["video_length_min"];

    for (auto& it : data["cameras"])
    {
        if (it["active"] != 1)
            continue;
        start_cam(it);
    }

    while (!currently_recording.empty() && !stop_requested)
    {
        for (auto it = currently_recording.begin(); it != currently_recording.end(); )
        {
            DWORD wait_result = WaitForSingleObject(it->proc_info.hProcess, 0);

            if (wait_result == WAIT_OBJECT_0)
            {
                CloseHandle(it->proc_info.hProcess);
                CloseHandle(it->proc_info.hThread);

                std::string nme = it->cam_name;
                it = currently_recording.erase(it);

                // Restart
                if (!stop_requested)
                {
                    for (auto& cam : data["cameras"])
                    {
                        if (cam["name"] == nme)
                        {
                            start_cam(cam);
                            break;
                        }
                    }
                }
                break;
            }
            else
            {
                it++;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    while (!currently_recording.empty())
    {
        TerminateProcess(currently_recording.back().proc_info.hProcess, 0);
        CloseHandle(currently_recording.back().proc_info.hProcess);
        CloseHandle(currently_recording.back().proc_info.hThread);
        currently_recording.pop_back();
    }

    return 0;
}
