#include "ChainerWindow.hpp"

#include "../Configuration.hpp"
#include "../App.hpp"

#include "../engine/core/Log.hpp"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui-fonts/material_design_icons.h" 

#include "../tools/SequencerWindow.hpp"

namespace sns {

	ChainerWindow::ChainerWindow() {
		TAG = "App";
		m_window_name = "Chainer";

		m_selected = 0;
		m_cfg_loaded = false;

		m_has_presets = true;
	}

	ChainerWindow::~ChainerWindow() {

	}

	void ChainerWindow::render() {
		beforeRender();

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 24.5f, 0.0f));
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		float table_height = ImGui::GetTextLineHeightWithSpacing() * 13.0f;

		m_cfg.action = Sequencer::Action::None;

		if (m_cfg_loaded) {
			m_cfg.action = Sequencer::Action::Stop;
			m_cfg_loaded = false;
		}

		renderTopBar();
		renderTable(table_height);
		ImGui::SameLine();
		renderPresets(table_height, "CHAINS", "chain");

		if ((m_cfg.action == Sequencer::Action::Stop) && !m_state.playing) {
			m_cfg.action = Sequencer::Action::None;
		}


		if (m_cfg.action != Sequencer::Action::None) {
			if (m_cfg.action == Sequencer::Action::Play) {
				m_cfg.action_link = m_selected;
				inflate(app()->configuration(), m_cfg);
			}
			else {
				deflate(m_cfg);
			}

			sequencerStop();
			app()->engine().setChainerConfiguration(m_cfg);
		}

		aboutToFinishRender();
		ImGui::End();

	}

	void ChainerWindow::stateChanged(Chainer::State const& state) {
		//Log::d(TAG, "ChainerWindow::stateChanged");
		m_state = state;
	}

	void ChainerWindow::renderTopBar() {

		// leading icons
		{
			if (ImGui::Button((const char*)ICON_MDI_STOP))
				m_cfg.action = Sequencer::Action::Stop;

			ImGui::SameLine();

			bool blink = m_state.playing && ((getCurrentMilliseconds() / 500) % 2 == 0);

			if (blink)
				ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0.25f, 0.95f, 0.65f));

			if (ImGui::Button((const char*)ICON_MDI_PLAY))
				m_cfg.action = Sequencer::Action::Play;

			if (blink)
				ImGui::PopStyleColor();
		}

		float const spacing = ImGui::GetTextLineHeight() * 5.25f;
		ImGui::SameLine(0.0f, spacing);
		int index = m_state.playing ? m_state.link_index + 1 : 0;
		int run = m_state.playing ? m_state.link_run + 1 : 0;
		ImGui::Text("%03d/%03d", index, run);
		ImGui::SameLine(0.0f, spacing);

		// line action
		{

			if (ImGui::Button((const char*)ICON_MDI_CHEVRON_DOUBLE_UP)) {
				if (m_cfg.chain.size() >= 2 && m_selected > 0) {
					std::swap(m_cfg.chain[m_selected - 1], m_cfg.chain[m_selected]);
					m_selected -= 1;

					m_cfg.action = Sequencer::Action::Stop;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button((const char*)ICON_MDI_CHEVRON_DOUBLE_DOWN)) {
				if (m_cfg.chain.size() >= 2 && m_selected < int(m_cfg.chain.size() - 1)) {
					std::swap(m_cfg.chain[m_selected], m_cfg.chain[m_selected + 1]);
					m_selected += 1;

					m_cfg.action = Sequencer::Action::Stop;
				}
			}


			ImGui::SameLine();

			if (ImGui::Button((const char*)ICON_MDI_PLUS_CIRCLE_OUTLINE)) {
				if (m_cfg.chain.empty()) {
					m_cfg.chain.push_back(Chainer::Link());
					m_selected = 0;
				}
				else {
					auto it = m_cfg.chain.begin();
					std::advance(it, m_selected + 1);
					m_cfg.chain.insert(it, Chainer::Link());
				}

				m_cfg.action = Sequencer::Action::Stop;
			}

			ImGui::SameLine();

			if (ImGui::Button((const char*)ICON_MDI_DELETE_FOREVER)) {
				if (!m_cfg.chain.empty()) {
					auto it = m_cfg.chain.begin();
					std::advance(it, m_selected);
					m_cfg.chain.erase(it);

					m_cfg.action = Sequencer::Action::Stop;
				}

				if (!m_cfg.chain.empty()) {
					m_selected = clampTo(m_selected, 0, int(m_cfg.chain.size()) - 1);
				}
			}
		}
	}

	void ChainerWindow::renderTable(float height) {

		float const base_width = ImGui::GetTextLineHeight();
		float const row_min_height = base_width * 0.0f;

		int const freeze_cols = 1;
		int const freeze_rows = 1;

		ImGuiTableFlags const flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX;
		float const spacing = base_width * 0.1f;
		ImVec2 const button_size(base_width, base_width);

		ImVec2 const outer_size(base_width * 15.0f, height);

		if (ImGui::BeginTable("sequence_table", 3, flags, outer_size)) {
			ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);

			ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, base_width * 1.5f);
			ImGui::TableSetupColumn((const char*)ICON_MDI_REPEAT, ImGuiTableColumnFlags_WidthFixed, base_width * 2.0f);
			ImGui::TableSetupColumn("Sequence");

			ImGui::TableHeadersRow();

			for (int i = 0; i != int(m_cfg.chain.size()); ++i) {
				Chainer::Link& link = m_cfg.chain[i];

				ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);

				ImGui::PushID(i);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(base_width * 0.25f, 0.0f));


				const bool item_is_selected = m_selected == i;

				ImGui::TableSetColumnIndex(0);

				ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
				if (ImGui::Selectable(sfmt("%03d", i + 1).c_str(), item_is_selected, selectable_flags, ImVec2(0, row_min_height)))
					m_selected = i;


				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(base_width * 2.0f);
				if (ImGui::DragInt("##runs", &link.runs, 0.25f, 0, 99)) {
					m_cfg.action = Sequencer::Action::Stop;
				}



				ImGui::TableSetColumnIndex(2);

				int combo_selected = findIndex(m_sequence_names, link.name, 0);
				int flags = ImGuiComboFlags_NoPreview;
				if (ImGui::BeginCombo("##sequence", "sequence_no_names", flags)) {
					for (int n = 0; n < int(m_sequence_names.size()); n++) {
						bool is_selected = (combo_selected == n);
						if (ImGui::Selectable(m_sequence_names[n].c_str(), is_selected)) {
							link.name = m_sequence_names[n];
							trim(link.name);

							m_cfg.action = Sequencer::Action::Stop;
						}
					}
					ImGui::EndCombo();
				}


				ImGui::SameLine();
				ImGui::Text("%s", m_sequence_names[combo_selected].c_str());


				ImGui::PopStyleVar();
				ImGui::PopID();
			}


			ImGui::EndTable();
		}
	}

	bool ChainerWindow::playing() {
		return m_state.playing;
	}

	void ChainerWindow::stop() {
		Chainer::Configuration cfg;
		cfg.action = Sequencer::Action::Stop;
		app()->engine().setChainerConfiguration(cfg);
	}

	void ChainerWindow::sequencerStop() {
		auto sequencer = app()->getWindow<SequencerWindow>();
		if (sequencer->playing())
			sequencer->stop();
	}

	void ChainerWindow::reloadSequences() {
		if (app() == nullptr)
			return;

		m_sequence_names.clear();
		m_sequence_names.push_back(" ");

		for (auto const& current : sequenceNames(app()->configuration())) {
			if (current.empty()) continue;
			if (current == DefaultSessionName) continue;
			if (current == LastSessionName) continue;

			m_sequence_names.push_back(current);
		}

		for (auto const& current : m_cfg.chain) {
			if (current.name.empty()) continue;
			if (contains(m_sequence_names, current.name)) continue;

			m_sequence_names.push_back(current.name);
		}
	}

	void ChainerWindow::onRefreshPresets() { m_preset_names = chainNames(app()->configuration()); }
	void ChainerWindow::onSavePreset(std::string const& name) { sns::saveChain(app()->configuration(), name, m_cfg); }
	void ChainerWindow::onDeletePreset(std::string const& name) { sns::deleteChain(app()->configuration(), name); }
	void ChainerWindow::onLoadPreset(std::string const& name) {
		m_cfg = sns::loadChain(app()->configuration(), name);
		reloadSequences();
		m_cfg_loaded = true;
	}

	void ChainerWindow::deflate(Chainer::Configuration& chainer_cfg) {
		for (auto& current : chainer_cfg.chain)
			current.sequence = Sequencer::Configuration();
	}

	void ChainerWindow::inflate(sns::Configuration const& cfg, Chainer::Configuration& chainer_cfg) {
		std::vector<std::string> names;
		std::vector<sns::Sequencer::Configuration> sequences;

		for (auto& current : chainer_cfg.chain) {
			if (current.name.empty())
				continue;

			if (!contains(names, current.name))
				names.push_back(current.name);
		}

		for (auto& name : names) {
			sns::Sequencer::Configuration sequence = loadSequence(cfg, name);
			sequences.push_back(sequence);
		}

		for (auto& current : chainer_cfg.chain) {
			int index = findIndex(names, current.name);
			if (index >= 0)
				current.sequence = sequences[index];
		}
	}
}
