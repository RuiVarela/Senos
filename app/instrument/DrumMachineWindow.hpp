#pragma once

#include "../Window.hpp"

#include "../../engine/instrument/DrumMachine.hpp"

namespace sns {

	class DrumMachineWindow : public Window {
	public:
		DrumMachineWindow();
		~DrumMachineWindow() override;

		void render() override;
	protected:


		void renderTable(std::string const& name, std::vector<DrumMachine::DMKey> const& data);
		void renderOptions();
	};
}