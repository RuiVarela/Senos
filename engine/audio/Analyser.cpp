#include "Analyser.hpp"
#include "Audio.hpp"

namespace sns {

	Analyser::Analyser() {
		configureGraph(10, 100);
	}

	void Analyser::configureGraph(int points, int duration) {
		m_graph_points = points;

		m_graph_duration = std::min(duration, SamplesDuration);
		m_graph_duration_samples = int(samplesFromMilliseconds(m_graph_duration));


		//std::vector<float> x;
		//generateGraph(x);
	}

	void Analyser::generateGraph(std::vector<float>& points) {
		if (int(points.size()) != m_graph_points)
			points.resize(m_graph_points);

		float increment = float(m_graph_duration_samples) / float(m_graph_points);
		
		for (int i = 0; i != m_graph_points; ++i) {
			float p0 = 0.0f;
			float pf = modf(float(i) * increment, &p0);
			float p1 = p0 + 1.0f;
		}
	}

	bool Analyser::isAccepting() {
		return false;
	}
		
	void Analyser::push(float sample) {
		while (m_samples.size() == m_samples.capacity()) 
			m_samples.pop_front();

		m_samples.push_back(sample);
	}
}