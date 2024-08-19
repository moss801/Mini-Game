#include "timer.h"

Timer::Timer(std::chrono::milliseconds interval, std::function<void(float)> callback, float param)
	: interval_(interval), callback_(callback), param_(param), is_running(false) {}

void Timer::Start()
{
	if (is_running) return; 
	start_time_point = std::chrono::high_resolution_clock::now();
	is_running = true;
	timerThread_ = std::thread(&Timer::run, this);
	std::cout << "game start" << std::endl;
}

void Timer::Stop()
{
	end_time_point = std::chrono::high_resolution_clock::now();
	is_running = false;
	if (timerThread_.joinable()) {
		timerThread_.join(); // 等待线程结束
	}
}

float Timer::ElapsedMilliseconds()
{
	if (is_running) 
	{
		auto current_time_point = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<float, std::milli>(current_time_point - start_time_point).count();
	}
	else
	{
		return std::chrono::duration<float, std::milli>(end_time_point - start_time_point).count();
	}
}

float Timer::ElapsedSeconds()
{
	return ElapsedMilliseconds() / 1000.0;
}

void Timer::run()
{
	std::this_thread::sleep_for(interval_);
	if (is_running) 
	{
		callback_(param_);
	}
}
