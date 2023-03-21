#pragma once

#include "../core/Lang.hpp"

namespace sns
{
	class RunningAverage {
	public:
		explicit RunningAverage(int window, float initial_value = 0.0)
			:m_window(window)
		{
			reset(initial_value);
		}

		void reset(float initial_value) {
			m_average = 0.0f;
			m_sum = 0.0f;
			m_samples.clear();
			add(initial_value);
		}

		float add(float value) {
			m_samples.push_back(value);
			m_sum += value;

			if (m_samples.size() > m_window) {
				m_sum -= m_samples.front();
				m_samples.pop_front();
			}

			m_average = m_sum / float(m_samples.size());
			return m_average;
		}

		float average() const { return m_average; }
	private:
		int m_window;
		std::list<float> m_samples;
		float m_sum;
		float m_average;
	};


	class RunningRms {
	public:
		explicit RunningRms(int window, float initial_value = 0.0)
			:m_window(window)
		{
			reset(initial_value);
		}

		void reset(float initial_value) {
			m_rms = 0.0f;
			m_sum_of_squares = 0.0f;

			m_samples.clear();
			add(initial_value);
		}

		float add(float value) {
			m_samples.push_back(value);
			m_sum_of_squares += value * value;

			if (m_samples.size() > m_window) {
				float front = m_samples.front();
				m_samples.pop_front();
				m_sum_of_squares -= front * front;
			}

			m_rms = std::sqrt(m_sum_of_squares / float(m_samples.size()));
			return m_rms;
		}

		float rms() const { return m_rms; }
	private:
		int m_window;
		std::list<float> m_samples;
		float m_sum_of_squares;
		float m_rms;
	};

}