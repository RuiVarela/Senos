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
	};
}
