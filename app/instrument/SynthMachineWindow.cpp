#include "SynthMachineWindow.hpp"

#include "../App.hpp"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"
#include "../../engine/core/Text.hpp"
#include "../../engine/core/Log.hpp"
#include "../../engine/instrument/SynthMachine.hpp"

namespace sns {

	SynthMachineWindow::SynthMachineWindow() {
		TAG = "SynthMachineWindow";
		m_window_name = "Synth Machine";
		m_subtype = InstrumentIdSynthMachine;
		m_is_instrument = true;
		m_has_presets = true;
	}

	SynthMachineWindow::~SynthMachineWindow() {

	}


	void SynthMachineWindow::render() {

		beforeRender();
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);


		//
		// oscillators
		//
		{
			for (int i = 0; i != SynthMachineOscCount; ++i) {
				renderOscillator(i);

				if (i != SynthMachineOscCount - 1)
					ImGui::SameLine();
			}
		}

		ImGui::SameLine();

		//
		// options
		//

		{
			float width = ImGui::GetFontSize() * 6.4f;
			float height = ImGui::GetFontSize() * 16.7f;

			ImGui::BeginChild("options_block", ImVec2(width, height), true);
			renderOptions();
			ImGui::EndChild();
		}




		float presets_height = ImGui::GetFontSize() * 9.0f;

		//
		// Envelope
		//
		{
			float width = ImGui::GetFontSize() * 14.2f;

			ImGui::BeginChild("envelope", ImVec2(width, presets_height), true);
			renderEnvelope();
			ImGui::EndChild();
		}

		ImGui::SameLine();

		//
		// Filter
		//
		{
			float width = ImGui::GetFontSize() * 17.0f;

			ImGui::BeginChild("filter", ImVec2(width, presets_height), true);
			renderFilter();
			ImGui::EndChild();
		}

		ImGui::SameLine();

		//
		// Presets
		//
		renderPresets(presets_height);

		aboutToFinishRender();
		ImGui::End();
	}

	void SynthMachineWindow::renderOscillator(int index)
	{
		float width = ImGui::GetFontSize() * 10.9f;
		float height = ImGui::GetFontSize() * 16.7f;
		ImGui::BeginChild(sfmt("osc_%d", index + 1).c_str(), ImVec2(width, height), true);

		//ImGui::PushID(sfmt("osc_%d", index + 1).c_str());
		ImGui::TextDisabled("OSCILLATOR %d", index + 1);

		int osc_base = getOscBase(index);
		int lfo_base = getLfoBase(index);


		pCombo("OSC", osc_base + ParameterOscKind, Oscillator::kindNames());
		pKnob("Detune", osc_base + ParameterOscDetune, -100.0f, 100.0f);
		ImGui::SameLine();
		pKnob("Volume", osc_base + ParameterOscVolume);
		pCombo("LFO", lfo_base + ParameterLfoKind, Oscillator::kindNames());
		pKnob("Freq", lfo_base + ParameterLfoFrequency);
		ImGui::SameLine();
		pKnob("Pitch", lfo_base + ParameterLfoPitch);
		ImGui::SameLine();
		pKnob("Amp", lfo_base + ParameterLfoAmplitude);

		//ImGui::PopID();
		ImGui::EndChild();
	}

	void SynthMachineWindow::renderEnvelope() {
		ImGui::TextDisabled("AMPLITUDE ENVELOPE");

		int parameter_base = getEnvBase(0);

		pKnob("Attack", parameter_base + ParameterEnvAttack);
		ImGui::SameLine();
		pKnob("Decay", parameter_base + ParameterEnvDecay);
		ImGui::SameLine();
		pKnob("Sustain", parameter_base + ParameterEnvSustain);
		ImGui::SameLine();
		pKnob("Release", parameter_base + ParameterEnvRelease);
	}

	void SynthMachineWindow::renderFilter() {
		ImGui::TextDisabled("FILTER");

		int parameter_base = getFilterBase(0);

		pKnob("Cutoff", parameter_base + ParameterFilterCutoff);
		ImGui::SameLine();
		pKnob("Res.", parameter_base + ParameterFilterResonance);
		ImGui::SameLine();
		pKnob("Drive", parameter_base + ParameterFilterDrive);

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = 0;
		ImGui::Text("Kind");
		ImGui::PushItemWidth(ImGui::GetFontSize() * 5.5f);
		pCombo("##Kind", parameter_base + ParameterFilterKind, Filter::kindNames());
		ImGui::PopItemWidth();
		ImGui::EndGroup();
	}


	void SynthMachineWindow::renderOptions() {
		ImGui::TextDisabled("OPTIONS");
		pBool(ParameterMono, "Mono");

		pKnob("Portamento", ParameterPortamento);
		pKnob("Volume", ParameterVolume);
	}
}