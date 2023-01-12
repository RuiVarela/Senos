#pragma once

#include "Window.hpp"

namespace sns {

	class SettingsWindow : public Window {
	public:
		SettingsWindow();
		~SettingsWindow() override;


		void show(bool value) override;
		void render() override;

	private:
		void renderAudio();
		void renderVideo();
		void rendeMidi();

		std::vector<std::string> m_midi_presets;
	};
}
