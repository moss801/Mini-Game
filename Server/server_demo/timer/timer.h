#pragma once
#include <functional>
#include <chrono>
#include <thread>
#include <iostream>

class Timer {
public:
    Timer(std::chrono::milliseconds interval, std::function<void(float)> callback, float param);

    void Start();
    void Stop();
    float ElapsedMilliseconds();
    float ElapsedSeconds();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_point;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time_point;
    std::chrono::milliseconds interval_; // —”≥Ÿ ±º‰
    std::function<void(float)> callback_; 
    float param_;
    bool is_running;
    std::thread timerThread_;

    void run();
};