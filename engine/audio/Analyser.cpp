#include "Analyser.hpp"
#include "Audio.hpp"

namespace sns {

	Analyser::Analyser()
		:m_started(false)
	{
		configureGraph(10, 100);
	}

	void Analyser::configureGraph(int points, int duration) {
		m_graph_points = points;

		m_graph_duration = std::min(duration, SamplesDuration);
		m_graph_duration_samples = int(samplesFromMilliseconds(m_graph_duration));
	}

	void Analyser::generateGraph(std::vector<float>& points) {
		if (!m_started)
			return;

		std::unique_lock<std::mutex> lock(m_samples_mutex);

		if (int(points.size()) != m_graph_points)
			points.resize(m_graph_points);

		float increment = float(m_graph_duration_samples) / float(m_graph_points);
		int start_index = int(m_samples.size()) - m_graph_duration_samples;

		if (start_index < 0)
			return;
		
		for (int i = 0; i != m_graph_points; ++i) {
			float factor = float(i) * increment;
			int point_index = int(factor);

			points[i] = m_samples[start_index + point_index];
		}
	}

	void Analyser::start() {
		m_started = true;
	}

	void Analyser::stop() {
		m_started = false;
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