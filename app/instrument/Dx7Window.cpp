#include "Dx7Window.hpp"

#include "../App.hpp"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui-knobs/imgui-knobs.h"
#include "../../engine/core/Text.hpp"
#include "../../engine/core/Log.hpp"
#include "../../engine/instrument/Dx7.hpp"
#include "../../engine/instrument/dx7/banks/Banks.hpp"

using namespace dx7;

namespace sns {

	Dx7Window::Dx7Window() {
		TAG = "Dx7Window";
		m_window_name = "Dx7";
		m_subtype = InstrumentIdDx7;
		m_is_instrument = true;
		m_has_presets = true;

		m_update_scrolls = false;
		//dx7::Banks::generateFilesFromFolder("/Users/ruivarela/Desktop/dx7");
		//dx7::Banks::generateFilesFromFolder("C:\\Users\\ruiva\\Desktop\\dx7");
	}

	Dx7Window::~Dx7Window() {

	}

	void Dx7Window::onLoadPreset(std::string const& name) {
		Window::onLoadPreset(name);
		m_update_scrolls = true;
	}

	void Dx7Window::render() {
		beforeRender();
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		{
			float width = ImGui::GetFontSize() * 30.4f;
			float height = ImGui::GetFontSize() * 10.8f;

			ImGui::BeginChild("patches_block", ImVec2(width, height), true);
			renderPatches();
			ImGui::EndChild();
		}


		float presets_height = ImGui::GetFontSize() * 9.5f;
		{
			float width = ImGui::GetFontSize() * 15.0f;

			ImGui::BeginChild("dynamics_block", ImVec2(width, presets_height), true);
			renderDynamics();
			ImGui::EndChild();
		}

		ImGui::SameLine();

		{
			float width = ImGui::GetFontSize() * 6.3f;

			ImGui::BeginChild("options_block", ImVec2(width, presets_height), true);
			renderOptions();
			ImGui::EndChild();
		}

		ImGui::SameLine();

		renderPresets(presets_height);

		aboutToFinishRender();
		ImGui::End();
	}

	void Dx7Window::renderPatches() {
		ImGui::TextDisabled("PATCHES");

		int list_drawn = 0;
		bool changed = false;
		bool scrool_groups = m_update_scrolls;
		bool scrool_banks = m_update_scrolls;
		bool scrool_patch = m_update_scrolls;

		float base_size = 5.6f * ImGui::GetTextLineHeightWithSpacing();
		float height_size = 7.0f * ImGui::GetTextLineHeightWithSpacing();

		{
			ImVec2 size = ImVec2(base_size, height_size);
			int index = int(m_values[ParameterGroup]);
			std::vector<std::string> records = Banks::instance().groups();
			if (ImGui::BeginListBox("##groups", size)) {
				for (size_t i = 0; i != records.size(); i++) {
					bool is_selected = (i == index);

					if (ImGui::Selectable(records[i].c_str(), &is_selected) && is_selected) {
						m_values[ParameterGroup] = float(i);
						m_values[ParameterBank] = 0.0f;
						m_values[ParameterPatch] = 0.0f;

						scrool_banks = true;
						scrool_patch = true;
						changed = true;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if ((i == index) && scrool_groups)
						ImGui::SetScrollHereY();
				}
				++list_drawn;
				ImGui::EndListBox();
			}
		}

		ImGui::SameLine();

		{
			ImVec2 size = ImVec2(base_size * 2.3f, height_size);
			int index = int(m_values[ParameterBank]);
			std::vector<std::string> records = Banks::instance().banks(int(m_values[ParameterGroup]));
			if (ImGui::BeginListBox("##banks", size)) {
				for (size_t i = 0; i != records.size(); i++) {
					bool is_selected = (i == index);

					if (ImGui::Selectable(records[i].c_str(), &is_selected) && is_selected) {
						m_values[ParameterBank] = float(i);
						m_values[ParameterPatch] = 0.0f;
						scrool_patch = true;
						changed = true;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if ((i == index) && scrool_banks)
						ImGui::SetScrollHereY();
				}
				++list_drawn;
				ImGui::EndListBox();
			}
		}

		ImGui::SameLine();

		{
			ImVec2 size = ImVec2(base_size, height_size);
			int index = int(m_values[ParameterPatch]);
			std::vector<std::string> records = Banks::instance().patches(int(m_values[ParameterGroup]), int(m_values[ParameterBank]));
			if (ImGui::BeginListBox("##patch", size)) {
				for (size_t i = 0; i != records.size(); i++) {
					bool is_selected = (i == index);

					if (ImGui::Selectable(records[i].c_str(), &is_selected) && is_selected) {
						m_values[ParameterPatch] = float(i);
						changed = true;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if ((i == index) && scrool_patch)
						ImGui::SetScrollHereY();
				}
				++list_drawn;
				ImGui::EndListBox();
			}
		}

		if (list_drawn == 3) {
			m_update_scrolls = false;
		}

		if (changed)
			applyInstrumentValues();
	}


	void Dx7Window::renderDynamics() {
		ImGui::TextDisabled("DYNAMICS");

		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;

		if (ImGui::BeginTable("table_controller", 5, flags)) {

			float bc_width = ImGui::GetFontSize() * 1.8f;
			float bc_spacing = ImGui::GetFontSize() * 0.25f;

			ImGui::TableSetupColumn("Controller");
			ImGui::TableSetupColumn("Range");
			ImGui::TableSetupColumn("Pitch", ImGuiTableColumnFlags_WidthFixed, bc_width);
			ImGui::TableSetupColumn("Amp", ImGuiTableColumnFlags_WidthFixed, bc_width);
			ImGui::TableSetupColumn("Env", ImGuiTableColumnFlags_WidthFixed, bc_width);
			ImGui::TableHeadersRow();

			// Modulation Wheel
			int c = 0;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(c++);
			ImGui::TextUnformatted("Mod Wheel");
			ImGui::TableSetColumnIndex(c++);
			pTableDragInt(ParameterModulationWheelRange);
			ImGui::TableSetColumnIndex(c++);
			pTableCheck(ParameterModulationWheelPitch, bc_spacing);
			ImGui::TableSetColumnIndex(c++);
			pTableCheck(ParameterModulationWheelAmp, bc_spacing);
			ImGui::TableSetColumnIndex(c++);
			pTableCheck(ParameterModulationWheelEnv, bc_spacing);


			//aftertouch
			c = 0;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(c++);
			ImGui::TextUnformatted("Aftertouch");
			ImGui::TableSetColumnIndex(c++);
			pTableDragInt(ParameterAftertouchRange);
			ImGui::TableSetColumnIndex(c++);
			pTableCheck(ParameterAftertouchPitch, bc_spacing);
			ImGui::TableSetColumnIndex(c++);
			pTableCheck(ParameterAftertouchAmp, bc_spacing);
			ImGui::TableSetColumnIndex(c++);
			pTableCheck(ParameterAftertouchEnv, bc_spacing);

			ImGui::EndTable();
		}


		if (ImGui::BeginTable("table_pitch_bend", 4, flags)) {
			float bc_width = ImGui::GetFontSize() * 2.8f;

			ImGui::TableSetupColumn("");
			ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_WidthFixed, bc_width);
			ImGui::TableSetupColumn("Up", ImGuiTableColumnFlags_WidthFixed, bc_width);
			ImGui::TableSetupColumn("Down", ImGuiTableColumnFlags_WidthFixed, bc_width);
			ImGui::TableHeadersRow();

			int c = 0;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(c++);
			ImGui::TextUnformatted("Pitch Bend");
			ImGui::TableSetColumnIndex(c++);
			pTableDragInt(ParameterPitchBendStep, 0, 12);
			ImGui::TableSetColumnIndex(c++);
			pTableDragInt(ParameterPitchBendUp, 0, 48);
			ImGui::TableSetColumnIndex(c++);
			pTableDragInt(ParameterPitchBendDown, 0, 48);

			ImGui::EndTable();
		}
	}

	void Dx7Window::renderOptions() {
		ImGui::TextDisabled("OPTIONS");
		pBool(ParameterMono, "Mono");
		pKnob("Volume", ParameterVolume);
	}
}