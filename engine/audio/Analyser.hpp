#pragma once

#include "../core/Lang.hpp"
#include "Audio.hpp"
#include "CircularBuffer.hpp"
#include <vector>

namespace sns {

	class Analyser {
	public:
		Analyser();

		bool isAccepting();
		void push(float sample);

		void configureGraph(int points, int duration);
		void generateGraph(std::vector<float>& points);

	private:

		int m_graph_points; // the number of graphed samples
		int m_graph_duration;
		int m_graph_duration_samples;

		CircularBuffer<float, SampleRate> m_samples;
	};

}