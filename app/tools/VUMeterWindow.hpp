#pragma once

#include "../Window.hpp"

namespace sns
{
	class VUMeterWindow : public Window
	{
	public:
		VUMeterWindow();
		~VUMeterWindow() override;

		void render() override;

	private:
		int m_levels;

	};
}
