#include "Recorder.hpp"
#include "../core/Log.hpp"

namespace sns {

	Recorder::Recorder()
		:TAG("Recorder"),
		m_accepting(false),
		m_flush_size(1024 * 10),
		m_received_samples(0)
	{

		setSleepMs(500);
	}

	Recorder::~Recorder() {

	}

	void Recorder::startRecording(std::string const& filename) {
		stopRecording();
		m_received_samples = 0;
		m_filename = filename;
		startWorking();
	}

	void Recorder::stopRecording() {
		if (isWorking())
			stopWorking();
	}

	bool Recorder::isRecording() {
		return isWorking();
	}

	uint64_t Recorder::recordedMilliseconds() {
		return audioMilliseconds(m_received_samples);
	}

	bool Recorder::isAccepting() {
		return isWorking() && m_accepting;
	}

	void Recorder::push(float sample) {
		if (m_accepting) {
			std::unique_lock<std::mutex> lock(m_mutex);

			m_write.push_back(sample);
			m_received_samples += 1;

			if (m_write.size() > m_flush_size)
				signalWorkArrived();
		}
	}

	void Recorder::workStep() {
		writeToFile();
	}

	void Recorder::preWork() {
		sns::makeDirectoryForFile(m_filename);

		Log::i(TAG, sfmt("Starting recorder (%s)...", m_filename));
		m_accepting = m_output.open(m_filename);

		if (!m_accepting) {
			Log::e(TAG, sfmt("Failed to start output to (%s)", m_filename));
		}
	}

	void Recorder::postWork() {
		m_accepting = false;

		writeToFile();
		m_output.close();

		Log::i(TAG, "Stopped recorder...");
	}

	void Recorder::writeToFile() {
		m_read.clear();

		{
			std::unique_lock<std::mutex> lock(m_mutex);
			std::swap(m_write, m_read);
		}

		if (!m_read.empty()) {
			//Log::e(TAG, sfmt("About to save %d samples", m_read.size()));
			m_output.write(m_read.data(), (int)m_read.size());
		}
	}
}