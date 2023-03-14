#pragma once

#include "../Window.hpp"

namespace sns
{
	class ScopeWindow : public Window
	{
	public:
		ScopeWindow();
		~ScopeWindow() override;

		void render() override;

	private:
		std::vector<float> m_samples;

		void updateCapture();

		// pickers
		bool m_active;

		std::vector<std::string> m_time_names;
		std::vector<int> m_time_values;
		int m_time;

		std::vector<std::string> m_points_names;
		std::vector<int> m_points_values;
		int m_points;

		std::vector<std::string> m_sync_names;
		std::vector<AnalyserSync> m_sync_values;
		int m_sync;

		float m_offset_factor;
	};
}
