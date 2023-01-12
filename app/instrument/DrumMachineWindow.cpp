#include "DrumMachineWindow.hpp"

#include "../App.hpp"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui-knobs/imgui-knobs.h"
#include "../../engine/core/Text.hpp"
#include "../../engine/core/Log.hpp"



namespace sns {


	DrumMachineWindow::DrumMachineWindow() {
		TAG = "DrumMachineWindow";
		m_window_name = "Drum Machine";
		m_subtype = InstrumentIdDrumMachine;
		m_is_instrument = true;
		m_has_presets = true;
	}

	DrumMachineWindow::~DrumMachineWindow() {

	}

	void DrumMachineWindow::render() {
		beforeRender();
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);


		float height = ImGui::GetFontSize() * 25.0f;


		{
			float width = ImGui::GetFontSize() * 16.0f;
			ImGui::BeginChild("drum_machine_", ImVec2(width, height), true);
			ImGui::TextDisabled("KITS");

			renderTable("TR-808", DrumMachine::tr808());
			ImGui::SameLine();
			renderTable("TR-909", DrumMachine::tr909());
			ImGui::EndChild();
		}

		ImGui::SameLine();

		//
		// options
		//

		{
			float width = ImGui::GetFontSize() * 6.4f;

			ImGui::BeginChild("options_block", ImVec2(width, height), true);
			renderOptions();
			ImGui::EndChild();
		}

		//ImGui::SameLine();
		//renderPresets(presets_height);


		aboutToFinishRender();
		ImGui::End();
	}



	void DrumMachineWindow::renderTable(std::string const& name, std::vector<DrumMachine::DMKey> const& data) {
		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;

		if (ImGui::BeginTable(name.c_str(), 2, flags)) {
			float c_width = ImGui::GetFontSize() * 5.0f;

			ImGui::TableSetupColumn("");
			ImGui::TableSetupColumn(name.c_str(), ImGuiTableColumnFlags_WidthFixed, c_width);
			ImGui::TableHeadersRow();

			for (auto const& [key, name, alias] : data) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(noteName(key, true).c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", name.c_str());
			}

			ImGui::EndTable();
		}
	}

	void DrumMachineWindow::renderOptions() {
		ImGui::TextDisabled("OPTIONS");
		pKnob("Volume", ParameterVolume);
	}

}