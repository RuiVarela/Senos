#pragma once

#include "Lang.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

namespace sns {
	class WaitEvent {
	public:
		WaitEvent(bool set = false);

		WaitEvent(const WaitEvent& other) = delete;
		WaitEvent(WaitEvent&& other) = delete;
		WaitEvent& operator=(const WaitEvent& other) = delete;
		WaitEvent& operator=(WaitEvent&& other) = delete;

		// don't delete this without making sure thread have left the building
		~WaitEvent() noexcept;

		void set();
		bool waitAndReset(int max_period_ms = 1000);
		void cancel();
	private:
		std::mutex m_mutex;
		std::condition_variable m_condition;
		bool m_set_state;
		bool m_canceled;
	};


	class Worker {
	public:
		Worker();

		Worker(const Worker& other) = delete;
		Worker(Worker&& other) = delete;
		Worker& operator=(const Worker& other) = delete;
		Worker& operator=(Worker&& other) = delete;

		virtual ~Worker();

		void setSleepMs(int millis);

		void startWorking();
		void stopWorking();
		bool isWorking();

		void signalWorkEnd();
		void signalWorkArrived();

		static void threadSleep(int const ms);
		static void threadSpin();

	protected:
		// override these methods to add work on the thread
		virtual void workStep();
		virtual void preWork();
		virtual void postWork();

	private:
		std::recursive_mutex m_runner_mutex;
		std::thread m_runner;
		WaitEvent m_runner_event;
		int m_sleep_milliseconds;
		bool m_working;

		virtual void pump();
	};


}


