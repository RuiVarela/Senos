#pragma once

#include "../Window.hpp"

namespace sns {

	class SynthMachineWindow : public Window {
	public:
		SynthMachineWindow();
		~SynthMachineWindow() override;

		void render() override;

	private:
		void renderOscillator(int index);
		void renderEnvelope();
		void renderFilter();
		void renderOptions();
	};
}