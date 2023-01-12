#include "Worker.hpp"

namespace sns {

	WaitEvent::WaitEvent(bool set)
		:m_mutex(),
		m_condition(),
		m_set_state(set),
		m_canceled(false)
	{
	}

	WaitEvent::~WaitEvent() noexcept {
		cancel();
	}


	void WaitEvent::set() {
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_canceled)
			return;

		m_set_state = true;
		m_condition.notify_all();
	}

	bool WaitEvent::waitAndReset(int max_period_ms) {
		std::unique_lock<std::mutex> lock(m_mutex);

		if (!m_set_state && !m_canceled) {
			auto wait = std::chrono::milliseconds(max_period_ms);
			m_condition.wait_for(lock, wait, [this]() { return m_set_state || m_canceled; });
		}

		bool state = m_set_state;
		m_set_state = false;
		return state;
	}

	void WaitEvent::cancel() {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_canceled = true;
		m_condition.notify_all();
	}



	Worker::Worker()
		:m_working(false)
	{
		setSleepMs(1000);
	}

	Worker::~Worker() {
		stopWorking();
	}

	void Worker::setSleepMs(int millis) {
		m_sleep_milliseconds = millis;
	}

	void Worker::startWorking() {
		std::unique_lock<std::recursive_mutex> lock(m_runner_mutex);

		stopWorking();

		m_working = true;
		m_runner = std::thread(&Worker::pump, this);
	}

	void Worker::stopWorking() {
		std::unique_lock<std::recursive_mutex> lock(m_runner_mutex);

		signalWorkEnd();

		if (m_runner.joinable()) {
			signalWorkArrived();
			m_runner.join();
		}
	}

	bool Worker::isWorking() {
		return m_working;
	}

	void Worker::signalWorkEnd() {
		m_working = false;
	}

	void Worker::signalWorkArrived() {
		m_runner_event.set();
	}

	void Worker::pump() {
		preWork();

		while (isWorking()) {

			workStep();

			m_runner_event.waitAndReset(m_sleep_milliseconds);
		}

		postWork();
	}

	void Worker::workStep() { }
	void Worker::preWork() { }
	void Worker::postWork() { }


	void Worker::threadSleep(const int ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

	void Worker::threadSpin() {
		threadSleep(0);
	}
}
