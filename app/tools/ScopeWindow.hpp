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

		void updateTime();

		// pickers
		std::vector<std::string> m_time_names;
		std::vector<int> m_time_values;
		int m_time;

		std::vector<std::string> m_points_names;
		std::vector<int> m_points_values;
		int m_points;
	};
}
