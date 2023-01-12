#include "SettingsWindow.hpp"

#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"
#include "App.hpp"

namespace sns {

	SettingsWindow::SettingsWindow()
	{
		TAG = "SettingsWindow";
		m_window_name = "Settings";
	}

	SettingsWindow::~SettingsWindow() {
	}


	void SettingsWindow::render() {
		beforeRender();

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 17.0f, 0.0f));
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		renderAudio();
		ImGui::Separator();
		renderVideo();
		ImGui::Separator();
		rendeMidi();

		aboutToFinishRender();
		ImGui::End();
	}

	void SettingsWindow::show(bool value) {
		Window::show(value);

		if (value) {
			m_midi_presets = midiPresetNames(app()->configuration());
		}
	}

	void SettingsWindow::renderAudio() {
		ImGui::TextDisabled("AUDIO:");

		Configuration& configuration = app()->configuration();

		ImGui::Indent();
		static const std::vector<int> values{ 128, 256, 512, 1024, 2048, 4096, 8192 };
		static const std::vector<std::string> labels{ "128", "256", "512", "1024", "2048", "4096", "8192" };

		int item = findIndex(values, app()->configuration().audio_buffer_size, 2);
		//ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
		if (pCombo("Buffer", labels, item)) {
			configuration.audio_buffer_size = values[item];
			saveConfiguration(app(), configuration);
		}
		ImGui::SameLine();
		pHelpMarker("Amount of samples produced and buffered per step. Requires a restart.");

		ImGui::Unindent();
	}

	void SettingsWindow::renderVideo() {
		ImGui::TextDisabled("VIDEO:");

		Configuration& configuration = app()->configuration();

		ImGui::Indent();
		static const std::vector<int> values{ 30, 60, 120, 0 };
		static const std::vector<std::string> labels{ "30", "60", "120", "Max" };

		int item = findIndex(values, app()->configuration().video_fps, 1);
		if (pCombo("Fps", labels, item)) {
			configuration.video_fps = values[item];
			saveConfiguration(app(), configuration);
		}
		ImGui::SameLine();
		pHelpMarker("Target video rendering. May not be respected.");

		ImGui::Unindent();
	}

	void SettingsWindow::rendeMidi() {
		ImGui::TextDisabled("MIDI:");

		bool changed = false;
		bool preset_changed = false;
		Configuration& configuration = app()->configuration();
		Midi& midi = app()->engine().midi();

		ImGui::Indent();
		{
			//bool value = app()->engine
			if (ImGui::Checkbox("Filter Focus Windows", &configuration.midi.filter_active_instrument)) {
				changed = true;
			}

			ImGui::SameLine();
			pHelpMarker("Only send midi messages to the instrument which window is in focus");
		}


		for (int i = InstrumentStart; i != InstrumentCount; ++i) {
			std::string name = instrumentToString(i);

			ImGui::PushID(name.c_str());
			ImGui::Text("%s", name.c_str());

			std::vector<std::string> ports;
			ports.push_back("None");
			addAll(ports, midi.ports());

			int item = findIndex(ports, configuration.midi.instruments[i].port, 0);

			if (pCombo("Port", ports, item)) {
				configuration.midi.instruments[i].port = (item == 0) ? "" : ports[item];
				changed = true;
			}

			//ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
			constexpr int min = 0;
			constexpr int max = 16;
			int value = configuration.midi.instruments[i].channel + 1;
			if (ImGui::DragInt("Channel", &value, 1, min, max)) {
				configuration.midi.instruments[i].channel = clampTo(value, min, max) - 1;
				changed = true;
			}

			ImGui::PopID();
		}


		{
			ImGui::Text("Mappings");

			std::vector<std::string> presets;
			presets.push_back("None");
			addAll(presets, m_midi_presets);

			int item = findIndex(presets, configuration.midi_preset, 0);
			if (pCombo("Preset", presets, item)) {
				configuration.midi_preset = (item == 0) ? "" : presets[item];
				preset_changed = true;
			}
		}


		ImGui::Unindent();

		if (changed || preset_changed) {

			if (preset_changed)
				loadMidiPreset(configuration, configuration.midi_preset);

			midi.setMapping(configuration.midi);
			saveConfiguration(app(), configuration);
		}
	}
}