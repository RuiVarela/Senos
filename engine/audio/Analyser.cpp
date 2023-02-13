#include "Analyser.hpp"
#include "Audio.hpp"

namespace sns {

	Analyser::Analyser() 
		:m_decimation(0)
	{

	}

	void Analyser::setDecimation(int decimation) {
		m_decimation = decimation;
		m_max_samples = SampleRate / (decimation + 1);
	}

	bool Analyser::isAccepting() {
		return false;
	}
		
	void Analyser::push(float sample) {

		while (!m_samples.empty() && (m_samples.size() + 1) > m_max_samples) 
			m_samples.erase(m_samples.begin());
		
		m_samples.push_back(sample);
	}
	

}