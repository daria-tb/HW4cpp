#pragma once
#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

class c_condition_variable
{
private:
	std::mutex mtx;
	std::atomic < bool> notified = false;
public:
	template<typename Predicate>
	void wait(std::unique_lock<std::mutex>& lock, Predicate pred)
	{
		while (!pred())
		{
			lock.unlock();
			while (!notified.load())  //std::memory_order_acquire
			{
				std::this_thread::sleep_for(std::chrono::seconds(2));
			}
			notified.store(false);  //std::memory_order_acquire
			lock.lock();
		}
	}

	void notify_one()
	{
		notified.store(true);
	}

	void notify_all()
	{
		notified.store(true);
	}
};