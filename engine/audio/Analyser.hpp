#pragma once

#include "../core/Lang.hpp"
#include <vector>

namespace sns {

	class Analyser {
	public:
		Analyser();

		bool isAccepting();
		void push(float sample);

		void setDecimation(int decimation);
	private:
		int m_decimation;
		int m_max_samples;
		std::vector<float> m_samples;
	};

}