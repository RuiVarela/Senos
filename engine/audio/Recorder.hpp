#pragma once

#include "../core/Worker.hpp"
#include "Wav.hpp"

namespace sns {

	class Recorder : private Worker {
	public:
		Recorder();
		~Recorder() override;

		void startRecording(std::string const& filename);
		void stopRecording();
		bool isRecording();

		uint64_t recordedMilliseconds();

		bool isAccepting();
		void push(float sample);

	protected:
		void workStep() override;
		void preWork() override;
		void postWork() override;

	private:
		std::string TAG;

		std::mutex m_mutex;
		std::vector<float> m_write;
		std::vector<float> m_read;
		size_t m_flush_size;
		uint64_t m_received_samples;

		Wav m_output;
		std::string m_filename;
		bool m_accepting;

		void writeToFile();
	};

}