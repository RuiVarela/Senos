#include "TB303Window.hpp"

#include "../App.hpp"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui-knobs/imgui-knobs.h"
#include "../../engine/core/Text.hpp"
#include "../../engine/core/Log.hpp"
#include "../../engine/instrument/TB303.hpp"


namespace sns {

	TB303Window::TB303Window() {
		TAG = "TB303Window";
		m_window_name = "TB-303";
		m_subtype = InstrumentIdTB303;
		m_is_instrument = true;
		m_has_presets = true;
	}

	TB303Window::~TB303Window() {

	}

	void TB303Window::render() {
		beforeRender();
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		float presets_height = ImGui::GetFontSize() * 9.5f;

		{
			float width = ImGui::GetFontSize() * 24.3f;

			ImGui::BeginChild("bass_line_block", ImVec2(width, presets_height), true);
			renderBassLine();
			ImGui::EndChild();
		}



		ImGui::SameLine();
		renderPresets(presets_height);


		aboutToFinishRender();
		ImGui::End();
	}

	void TB303Window::renderBassLine() {
		ImGui::TextDisabled("Bass Line");

		pKnob("Tuning", ParameterTuning);
		ImGui::SameLine();
		pKnob("Cutoff", ParameterFilterBase + ParameterFilterCutoff);
		ImGui::SameLine();
		pKnob("Res.", ParameterFilterBase + ParameterFilterResonance);
		ImGui::SameLine();
		pKnob("Env M.", ParameterEnvBase + ParameterEnvMod);
		ImGui::SameLine();
		pKnob("Decay", ParameterEnvBase + ParameterEnvDecay);
		ImGui::SameLine();
		pKnob("Accent", ParameterAccent);
		ImGui::SameLine();
		pKnob("Volume", ParameterVolume);


		int osc_kind = int(m_values[ParameterOscBase + ParameterOscKind]);
		bool changed = false;
		changed |= ImGui::RadioButton("Saw", &osc_kind, 0);
		ImGui::SameLine();
		changed |= ImGui::RadioButton("Square", &osc_kind, 1);
		if (changed) {
			m_values[ParameterOscBase + ParameterOscKind] = float(osc_kind);
			applyInstrumentValues();
		}
	}

}