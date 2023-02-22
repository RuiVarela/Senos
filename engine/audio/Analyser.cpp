#include "Analyser.hpp"
#include "Audio.hpp"

namespace sns {

	Analyser::Analyser() {
		configureGraph(10, 100);
	}

	void Analyser::configureGraph(int points, int duration) {
		m_graph_points = points;
		m_graph_duration = duration;
		m_graph_duration_samples = int(samplesFromMicroseconds(duration));
	}

	void Analyser::generateGraph(std::vector<float>& points) {


		
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