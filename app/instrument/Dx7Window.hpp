#pragma once

#include "../Window.hpp"

namespace sns {

	class Dx7Window : public Window {
	public:
		Dx7Window();
		~Dx7Window() override;

		void render() override;
	protected:
		void onLoadPreset(std::string const& name) override;

	private:
		void renderPatches();
		void renderDynamics();
		void renderOptions();

		bool m_update_scrolls;
	};
}