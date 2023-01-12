#pragma once

#include "../core/Lang.hpp"

namespace sns
{
	class RunningAverage {
	public:
		explicit RunningAverage(int max_samples, float initial_value = 0.0)
			:m_max_samples(max_samples)
		{
			reset(initial_value);
		}

		void reset(float initial_value) {
			m_average = 0.0;
			m_sum = 0.0;
			m_samples.clear();

			add(initial_value);
		}

		float add(float value) {
			m_samples.push_back(value);
			m_sum += value;

			if (m_samples.size() > m_max_samples) {
				m_sum -= m_samples.front();
				m_samples.pop_front();
			}

			m_average = m_sum / float(m_samples.size());
			return m_average;
		}

		float average() const { return m_average; }
	private:
		int m_max_samples;
		std::list<float> m_samples;
		float m_sum;
		float m_average;
	};
}