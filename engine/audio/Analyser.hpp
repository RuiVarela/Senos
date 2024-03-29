#pragma once

#include "../core/Lang.hpp"
#include "Audio.hpp"
#include "CircularBuffer.hpp"
#include "RunningStats.hpp"

#include <vector>
#include <mutex>

namespace sns {

	enum class AnalyserSync {
		None,
		RiseZero,
		FallZero
	};

	class Analyser {
	public:
		Analyser();

		void start(std::string const& key);
		void stop(std::string const& key);
		bool isAccepting();

		void push(float sample);

		void configureGraph(int points, int duration, float offset_factor, AnalyserSync sync);
		void generateGraph(std::vector<float>& points);

		float peak();

	private:
		static constexpr int SamplesDuration = (int)audioMilliseconds(SampleRate);
		const std::string TAG = "Analyser";
		AnalyserSync m_sync;


		int m_graph_points; // the number of graphed samples
		int m_graph_duration;
		int m_graph_duration_samples;
		int m_graph_offset;

		CircularBuffer<float, SampleRate> m_samples;
		std::mutex m_samples_mutex;

		bool m_started;
		std::vector<std::string> m_keys;
	};

	std::string toString(AnalyserSync kind);

}