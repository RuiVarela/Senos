#include "SequencerWindow.hpp"

#include "../Configuration.hpp"
#include "../App.hpp"

#include "../engine/core/Log.hpp"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"
#include "../vendor/imgui-fonts/material_design_icons.h" 

#include "../instrument/DrumMachineWindow.hpp"
#include "../tools/ChainerWindow.hpp"

namespace sns {

	SequencerWindow::SequencerWindow()
	{
		TAG = "SequencerWindow";
		m_window_name = "Sequencer";
		m_has_presets = true;

		m_cfg_updated = false;
		m_cfg_loaded = false;
		m_needs_scroll = false;
		m_rendered_table = false;

		//ICON_MDI_CLOSE_OCTAGON          ICON_MDI_CLOSE_OCTAGON_OUTLINE
		//ICON_MDI_TRIANGLE               ICON_MDI_TRIANGLE_OUTLINE
		//ICON_MDI_SQUARE                 ICON_MDI_SQUARE_OUTLINE
		//ICON_MDI_HEXAGON_MULTIPLE       ICON_MDI_HEXAGON_OUTLINE
		//ICON_MDI_CHECKBOX_BLANK         ICON_MDI_CHECKBOX_BLANK_OUTLINE
		//ICON_MDI_CHECKBOX_BLANK_CIRCLE  ICON_MDI_CHECKBOX_BLANK_CIRCLE_OUTLINE


		m_note_icons[int(Sequencer::NoteMode::Off)] = " ";
		m_note_icons[int(Sequencer::NoteMode::Press)] = (const char*)ICON_MDI_TRIANGLE_OUTLINE;
		m_note_icons[int(Sequencer::NoteMode::Accent)] = (const char*)ICON_MDI_SQUARE_OUTLINE;
		m_note_icons[int(Sequencer::NoteMode::Hold)] = (const char*)ICON_MDI_TRIANGLE;


		m_note_keys[int(Sequencer::NoteMode::Off)] = ImGuiKey_O;
		m_note_keys[int(Sequencer::NoteMode::Press)] = ImGuiKey_P;
		m_note_keys[int(Sequencer::NoteMode::Accent)] = ImGuiKey_A;
		m_note_keys[int(Sequencer::NoteMode::Hold)] = ImGuiKey_H;


		m_move_names[int(MoveKind::Up)] = (const char*)ICON_MDI_ARROW_UP "Up";
		m_move_names[int(MoveKind::Down)] = (const char*)ICON_MDI_ARROW_DOWN "Down";
		m_move_names[int(MoveKind::Left)] = (const char*)ICON_MDI_ARROW_LEFT "Left";
		m_move_names[int(MoveKind::Right)] = (const char*)ICON_MDI_ARROW_RIGHT "Right";
	}

	SequencerWindow::~SequencerWindow() {
	}

	bool SequencerWindow::playing() {
		return m_state.playing;
	}

	void SequencerWindow::stop() {
		Sequencer::Configuration cfg;
		cfg.action = Sequencer::Action::Stop;
		app()->engine().setSequencerConfiguration(cfg);
	}

	void SequencerWindow::chainerStop() {
		auto chainer = app()->getWindow<ChainerWindow>();
		if (chainer->playing())
			chainer->stop();
	}

	bool SequencerWindow::chainerPlaying() {
		auto chainer = app()->getWindow<ChainerWindow>();
		return chainer->playing();
	}

	void SequencerWindow::stateChanged(Sequencer::State const& state) {

		// ignore data from engine if the chainer is running
		if (state.chaining) {
			m_state.playing = false;
			return;
		}

		//Log::d(TAG, "SequencerWindow::stateChanged");
		m_state = state;
	}

	void SequencerWindow::render() {
		beforeRender();

		ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 38.0f, 0.0f));
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		float table_height = ImGui::GetTextLineHeightWithSpacing() * 12.0f * 2.0f;

		m_cfg.action = Sequencer::Action::None;
		m_cfg.action_step = -1;

		if (m_cfg_loaded) {
			m_cfg.action = Sequencer::Action::Stop;
			m_needs_scroll = true;
			m_rendered_table = false;
			m_cfg_loaded = false;
		}

		renderTopBar();
		renderTable(table_height);
		ImGui::SameLine();
		renderPresets(table_height, "SEQUENCES", "sequence");

		m_cfg_updated |= (m_cfg.action != Sequencer::Action::None);


		if (m_cfg_updated && chainerPlaying() && (m_cfg.action != Sequencer::Action::TogglePlay)) {
			m_cfg_updated = false;
		}

		if (m_cfg_updated) {
			chainerStop();
			app()->engine().setSequencerConfiguration(m_cfg);
		}

		m_cfg_updated = false;

		aboutToFinishRender();
		ImGui::End();
	}

	static std::vector<std::string> instrumentNames() {
		std::vector<std::string> output;

		for (InstrumentId i = InstrumentStart; i != InstrumentCount; i++)
			output.push_back(instrumentToString(i));

		return output;
	}

	void SequencerWindow::renderTopBar() {
		// leading icons
		{
			if (ImGui::Button((const char*)ICON_MDI_STOP))
				m_cfg.action = Sequencer::Action::Stop;
			ImGui::SameLine();

			bool blink = m_state.playing && ((getCurrentMilliseconds() / 500) % 2 == 0);

			if (blink)
				ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0.25f, 0.95f, 0.65f));

			if (ImGui::Button((const char*)ICON_MDI_PLAY_PAUSE)) {
				m_cfg.action = Sequencer::Action::TogglePlay;
				if (!m_state.playing)
					m_cfg.action_step = m_state.active_step;
			}

			if (blink)
				ImGui::PopStyleColor();
		}


		//// random
		//ImGui::SameLine(0.0f, block_margin);
		//ImGui::Text("Random");
		//ImGui::SameLine();
		//ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(1.0f, 0.0f, 0.0f));
		//ImGui::Button((const char*)ICON_MDI_RECORD);
		//ImGui::PopStyleColor();

		float item_width = ImGui::GetTextLineHeightWithSpacing() * 2.2f;

		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(item_width * 3.0f);

			static std::vector<std::string> instrument_names = instrumentNames();
			int index = int(m_cfg.ui_selected_instrument - InstrumentStart);
			if (index < 0 || index >= instrument_names.size())
				index = 0;
			pCombo("##instrument", instrument_names, index);
			InstrumentId instrument_id = InstrumentId(InstrumentStart + index);
			if (instrument_id != m_cfg.ui_selected_instrument) {
				Log::d(TAG, "Instrument changed");
				m_cfg.ui_selected_instrument = instrument_id;
				m_needs_scroll = true;
			}
		}

		// Clear instrument
		{
			ImGui::SameLine();
			if (ImGui::Button((const char*)ICON_MDI_DELETE)) {
				m_cfg.instruments[m_cfg.ui_selected_instrument].steps.clear();
				m_cfg_updated = true;
			}
		}

		// duty
		{
			ImGui::SameLine(0.0f, ImGui::GetTextLineHeight() * 1.3f);
			ImGui::Text("Muted");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(item_width);
			if (ImGui::Checkbox("##muted", &m_cfg.instruments[m_cfg.ui_selected_instrument].muted))
				m_cfg_updated = true;
		}

		// duty
		{
			ImGui::SameLine();
			ImGui::Text("Duty");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(item_width);
			int v = int(m_cfg.duty * 100.0f);
			if (ImGui::DragInt("##duty", &v, 1, 1, 100, "%d%%")) {
				v = clampTo(v, 1, 100);
				m_cfg.duty = float(v) / 100.0f;
				m_cfg_updated = true;
			}
		}

		// tempo
		{
			ImGui::SameLine();
			ImGui::Text("Tempo");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(item_width);
			if (ImGui::DragInt("##tempo", &m_cfg.tempo, 1, 40, 700)) {
				m_cfg.tempo = clampTo(m_cfg.tempo, 40, 700);
				m_cfg_updated = true;
			}
		}

		// steps
		{
			ImGui::SameLine();
			ImGui::Text("%02d Steps", m_cfg.step_count);
			ImGui::SameLine();
			ImGui::PushButtonRepeat(true);
			if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
				m_cfg.step_count--;
				m_cfg_updated = true;
			}
			ImGui::SameLine(0.0f);
			if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
				m_cfg.step_count++;
				m_cfg_updated = true;
			}
			ImGui::PopButtonRepeat();
			m_cfg.step_count = clampTo(m_cfg.step_count, 2, 64);
		}

	}

	void SequencerWindow::renderTable(float height) {
		ImU32 const cell_bg_color = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
		ImGuiTableFlags const flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Borders;

		int const freeze_cols = 1;
		int const freeze_rows = 1;

		ImVec2 const button_size(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());
		ImGuiStyle const& style = ImGui::GetCurrentContext()->Style;
		float const border_size = 1.0f;  //hardcoded border size
		float const expected_cell_width = button_size.x + style.CellPadding.x * 2.0f + border_size;
		float const expected_cell_height = button_size.y + style.CellPadding.y * 2.0f;
		ImVec2 const outer_size(expected_cell_width * (16.0f + 3.6f), height);


		//scroll only works after the first render

		if (m_needs_scroll && m_rendered_table) {
			//
			// load scroll
			//
			float x = expected_cell_width * m_cfg.instruments[m_cfg.ui_selected_instrument].ui_first_col;
			float y = expected_cell_height * m_cfg.instruments[m_cfg.ui_selected_instrument].ui_first_row;
			//ImGui::SetScrollX(x);
			//ImGui::SetScrollY(y);

			ImGui::SetNextWindowScroll(ImVec2(x, y));
			m_needs_scroll = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_rendered_table ? 1.0f : 0.001f);
		if (ImGui::BeginTable("sequence_table", 1 + m_cfg.step_count, flags, outer_size)) {
			ImGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);

			ImGui::TableSetupColumn("Note", ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed, button_size.x * 4.0f);
			for (int step_index = 0; step_index != m_cfg.step_count; step_index++) {
				ImGui::TableSetupColumn(sfmt("%02d", step_index).c_str(), ImGuiTableColumnFlags_WidthFixed, button_size.x);
			}

			const float header_y = ImGui::GetCursorScreenPos().y + ImGui::GetScrollY();
			const float header_height = ImGui::TableGetHeaderRowHeight();
			ImGui::TableHeadersRow();

			// detect header click
			{
				int column = ImGui::TableGetHoveredColumn();
				if (ImGui::IsMouseClicked(0) && column != -1) {
					// Allow opening popup from the right-most section after the last column.
					ImVec2 mouse_pos = ImGui::GetMousePos();
					//Log::d(TAG, sfmt("Header (%.2f, %.2f) Y=%.2f row_height=%.2f", mouse_pos.x, mouse_pos.y, header_y, header_height));
					if (mouse_pos.y >= header_y && mouse_pos.y <= (header_y + header_height)) {
						//Log::d(TAG, sfmt("Header click %d", column));
						if (column > 0) {
							m_cfg.action = m_state.playing ? Sequencer::Action::Play : Sequencer::Action::Stop;
							m_cfg.action_step = column - 1;
						}
					}
				}
			}

			for (int note = (TotalNotes - 1); note > 12; note--) {

				ImGui::TableNextRow();

				{
					ImGui::TableSetColumnIndex(0);
					bool use_default = true;

					if (m_cfg.ui_selected_instrument == InstrumentIdDrumMachine) {
						std::string note_name = noteName(note, true);
						std::string dm_alias = DrumMachine::alias(note);
						if (!dm_alias.empty()) {
							ImGui::Text("%s> %s", note_name.c_str(), dm_alias.c_str());
							use_default = false;
						}
					}

					if (use_default)
						ImGui::Text("%s", noteName(note, true).c_str());
				}



				for (int step_index = 0; step_index != m_cfg.step_count; step_index++) {
					ImGui::TableSetColumnIndex(step_index + 1);

					if (m_state.active_step == step_index)
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);

					//m_cfg.steps[m_instrument].size();

					ImGui::PushID(note * 1000 + step_index);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					Sequencer::NoteMode value = m_cfg.stepState(m_cfg.ui_selected_instrument, step_index, note);
					if (ImGui::Button(m_note_icons[int(value)], button_size)) {
						m_cfg.toggleStepState(m_cfg.ui_selected_instrument, step_index, note);
						m_cfg_updated = true;
					}

					if (ImGui::IsItemHovered()) {
						for (int mode_i = int(Sequencer::NoteMode::Off); mode_i != int(Sequencer::NoteMode::Count); ++mode_i) {
							Sequencer::NoteMode mode = Sequencer::NoteMode(mode_i);
							ImGuiKey key = ImGuiKey(m_note_keys[mode_i]);
							if (ImGui::IsKeyDown(key) && value != mode) {
								m_cfg.setStepState(m_cfg.ui_selected_instrument, step_index, note, mode);
								m_cfg_updated = true;
							}
						}
					}

					renderPopup(step_index, note);

					ImGui::PopStyleVar(1);
					ImGui::PopStyleColor(1);
					ImGui::PopID();
					//ImGui::Text("%s", m_note_icons[int(value)]);
				}
			}

			if (!m_needs_scroll) {
				//
				// Save Scroll
				//
				int row = int(ImGui::GetScrollY() / expected_cell_height);
				int col = int(ImGui::GetScrollX() / expected_cell_width);
				if (row != m_cfg.instruments[m_cfg.ui_selected_instrument].ui_first_row) {
					m_cfg.instruments[m_cfg.ui_selected_instrument].ui_first_row = row;
					//Log::d(TAG, sfmt("Scroll %d(%s) | %d", row, noteName(12 + row, true).c_str(), col));
				}

				if (col != m_cfg.instruments[m_cfg.ui_selected_instrument].ui_first_col) {
					m_cfg.instruments[m_cfg.ui_selected_instrument].ui_first_col = col;
					//Log::d(TAG, sfmt("Scroll %d(%s) | %d", row, noteName(12 + row, true).c_str(), col));
				}
			}


			ImGui::EndTable();
			m_rendered_table = true;
		}
		ImGui::PopStyleVar(1);
	}

	void SequencerWindow::renderPopup(int step_index, int note) {
		if (ImGui::BeginPopupContextItem("context_popup")) {
			ImGui::SeparatorText("Step");
			for (int mode_i = int(Sequencer::NoteMode::Off); mode_i != int(Sequencer::NoteMode::Count); ++mode_i) {
				Sequencer::NoteMode mode = Sequencer::NoteMode(mode_i);
				if (ImGui::Selectable(sfmt("%s %s", m_note_icons[mode_i], toString(mode)).c_str())) {
					m_cfg.setStepState(m_cfg.ui_selected_instrument, step_index, note, mode);
					m_cfg_updated = true;
				}
			}

			ImGui::SeparatorText((const char*)ICON_MDI_BORDER_STYLE " " ICON_MDI_ARROW_BOTTOM_RIGHT);

			for (int kind_i = int(MoveKind::Up); kind_i != int(MoveKind::Count); ++kind_i) {
				MoveKind kind = MoveKind(kind_i);
				if (ImGui::Selectable(m_move_names[kind_i])) {
					handleMove(MoveKind(kind_i), step_index, note);
					m_cfg_updated = true;
				}
			}

			ImGui::EndPopup();
		}
	}

	void SequencerWindow::handleMove(MoveKind kind, int step_index, int note) {
		Log::d(TAG, sfmt("handleMove %d - %d | %d", int(kind), step_index, note));
	}

	void SequencerWindow::onRefreshPresets() {
		m_preset_names = sequenceNames(app()->configuration());
	}

	void SequencerWindow::onSavePreset(std::string const& name) {
		sns::saveSequence(app()->configuration(), name, m_cfg);

		auto chainer = app()->getWindow<ChainerWindow>();
		chainer->reloadSequences();
	}

	void SequencerWindow::onDeletePreset(std::string const& name) {
		sns::deleteSequence(app()->configuration(), name);
	}

	void SequencerWindow::onLoadPreset(std::string const& name) {
		m_cfg = sns::loadSequence(app()->configuration(), name);
		m_cfg_loaded = true;
	}
	


}
