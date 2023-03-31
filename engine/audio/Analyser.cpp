#include "Analyser.hpp"
#include "Audio.hpp"

#include "../core/Log.hpp"


namespace sns {

	std::string toString(AnalyserSync kind) {
		switch (kind) {
		case AnalyserSync::None: return "None";
		case AnalyserSync::RiseZero: return "RiseZero";
		case AnalyserSync::FallZero: return "FallZero";
		default: break;
		}
		return "[AnalyserSync NOT_SET]";
	}


	Analyser::Analyser()
		:m_started(false)
	{
		configureGraph(10, 100, 0.0f, AnalyserSync::None);
	}

	void Analyser::configureGraph(int points, int duration, float offset_factor, AnalyserSync sync) {
		m_sync = sync;

		offset_factor = clampTo(offset_factor, 0.0f, 1.0f);

		m_graph_points = points;

		m_graph_duration = std::min(duration, SamplesDuration);
		m_graph_duration_samples = int(samplesFromMilliseconds(m_graph_duration));

		m_graph_offset = int(float(m_samples.capacity()) * offset_factor);
		Log::d(TAG, sfmt("points=%d duration=%d graph_offset=%d sync=%s", 
						 points, duration, m_graph_offset, toString(sync)));
	}

	void Analyser::generateGraph(std::vector<float>& points) {
		std::unique_lock<std::mutex> lock(m_samples_mutex);

		if (int(points.size()) != m_graph_points)
			points.resize(m_graph_points);
		
		// we still did not receive enough samples
		if (int(m_samples.size()) < m_graph_duration_samples)
			return;

		float increment = float(m_graph_duration_samples) / float(m_graph_points);
		int start_index = int(m_samples.size()) - m_graph_duration_samples;
		start_index = clampAbove(start_index, 0);

		if (m_sync != AnalyserSync::None) {
			int index = start_index;

			if (m_sync == AnalyserSync::RiseZero) {
				if (m_samples[index] < 0.0f)
					while (index && m_samples[index] <= 0.0f) --index;

				if (m_samples[index] > 0.0f)
					while (index && m_samples[index] > 0.0f) --index;
			} else if (m_sync == AnalyserSync::FallZero) {
				if (m_samples[index] > 0.0f)
					while (index && m_samples[index] > 0.0f) --index;

				if (m_samples[index] < 0.0f)
					while (index && m_samples[index] <= 0.0f) --index;
			}

			if (index > 0)
				start_index = index;
		}


		start_index = clampAbove(start_index - m_graph_offset, 0);

		
		for (int i = 0; i != m_graph_points; ++i) {
			float factor = float(i) * increment;
			int point_index = int(factor);

			points[i] = m_samples[start_index + point_index];
		}
	}

	float Analyser::peak() {
		std::unique_lock<std::mutex> lock(m_samples_mutex);
		constexpr int sample_count = int(samplesFromMilliseconds(150L));
		float max = 0.0f;

		if (int(m_samples.size()) <= sample_count)
			return max;
		
		int counter = 0;
		while (counter != sample_count) {
			const int index = int(m_samples.size()) - 1 - counter;
			const float v = std::abs(m_samples[index]);
			if (v > max)
				max = v;
			++counter;
		}
		return max;
	}

	void Analyser::start(std::string const& key) {
		bool already_started = m_started;

		remove(m_keys, key);
		m_keys.push_back(key);
		m_started = true;

		if (!already_started)
			Log::d(TAG, "start");
	}

	void Analyser::stop(std::string const& key) {
		if (m_started) {
			remove(m_keys, key);
			m_started = !m_keys.empty();
			if (!m_started)
				Log::d(TAG, "stop");
		}
	}

	bool Analyser::isAccepting() {
		return m_started;
	}
		
	void Analyser::push(float sample) {
		std::unique_lock<std::mutex> lock(m_samples_mutex);

		while (m_samples.size() == m_samples.capacity()) 
			m_samples.pop_front();

		m_samples.push_back(sample);
	}
}