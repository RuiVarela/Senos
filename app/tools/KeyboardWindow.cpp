#include "KeyboardWindow.hpp"
#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"

#include "../App.hpp"

namespace sns {

	std::string KeyboardWindow::KeyStateName(KeyState state) {
		switch (state)
		{
		case KeyState::Off: return "Off";
		case KeyState::OnByMouse: return "OnByMouse";
		case KeyState::OnByKeyboard: return "OnByKeyboard";
		default: return "[WrongValue]";
		}
	}

	struct KeyboardWindow::PrivateImplementation
	{
		const int octaves = 2;
		const int min_octave = 0;
		const int max_octave = 8;

		const bool note_is_dark[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
		static const int pc_keys = 24;
		const ImGuiKey note_pc_key[pc_keys] = {
			ImGuiKey_Z, ImGuiKey_S, ImGuiKey_X, ImGuiKey_D, ImGuiKey_C, ImGuiKey_V,
			ImGuiKey_G, ImGuiKey_B, ImGuiKey_H, ImGuiKey_N, ImGuiKey_J, ImGuiKey_M,
			ImGuiKey_Q, ImGuiKey_2, ImGuiKey_W, ImGuiKey_3, ImGuiKey_E, ImGuiKey_R,
			ImGuiKey_5, ImGuiKey_T, ImGuiKey_6, ImGuiKey_Y, ImGuiKey_7, ImGuiKey_U
		};

		const ImU32 colors[5] = {
			  IM_COL32(255, 255, 255, 255),	    // light note
			  IM_COL32(0, 0, 0, 255),			// dark note
			  IM_COL32(255, 255, 0, 255),		// active light note
			  IM_COL32(200, 200, 0, 255),		// active dark note
			  IM_COL32(75, 75, 75, 255),		// background
		};

		KeyState keys[TotalNotes];
	};

	KeyboardWindow::KeyboardWindow()
		:m(std::make_shared<PrivateImplementation>()),
		m_octave(4)
	{
		TAG = "Keyboard";
		m_window_name = "Keyboard";

		for (int key = 0; key != TotalNotes; ++key)
			m->keys[key] = KeyState::Off;
	}

	KeyboardWindow::~KeyboardWindow() {
		m.reset();
	}


	void KeyboardWindow::render() {
		beforeRender();

		const float note_width = 3.0f * ImGui::GetFontSize();
		const float note_height = 5.5f * note_width;
		const float note_width_dark = (2.0f / 3.0f) * note_width;
		const float note_height_dark = (2.0f / 3.0f) * note_height;
		const float note_dark_offset = -note_width_dark / 2.0f;

		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);


		int last_octave = m_octave;
		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::Text("Octave: %d", m_octave);
		ImGui::SameLine();
		ImGui::PushButtonRepeat(true);
		if (ImGui::ArrowButton("##left", ImGuiDir_Left)) m_octave--;
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::ArrowButton("##right", ImGuiDir_Right)) m_octave++;
		ImGui::PopButtonRepeat();
		m_octave = clampTo(m_octave, m->min_octave, m->max_octave);

		if (m_octave != last_octave)
			changedOctave();

		ImGui::SameLine(0.0f, spacing * 5.0f);
		if (ImGui::Button("Panic"))
			panic();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 mouse_pos = ImGui::GetIO().MousePos;

		// item
		const ImVec2 size(m->octaves * 7.0f * note_width, note_height);
		ImGui::InvisibleButton("keys", size, 0);
		bool held = ImGui::IsItemActive();

		const ImRect bb(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
		int pressed_key = -1;
		float pressed_key_velocity = 0.0f;

		//
		// White keys
		//
		float x = bb.Min.x;
		float y = bb.Min.y;
		for (int i = 0; i != (m->octaves * 12); ++i) {
			if (m->note_is_dark[i % 12]) continue;
			const int octave = m_octave + (i / 12);
			const int key = noteIndex(i % 12, octave);

			ImRect region = ImRect(x, y, x + note_width, y + note_height);

			if (held && region.Contains(mouse_pos)) {
				pressed_key = key;
				pressed_key_velocity = (mouse_pos.y - region.Min.y) / region.GetHeight();
			}

			bool pressed = isPressed(key);

			draw_list->AddRectFilled(region.Min, region.Max, m->colors[pressed ? 2 : 0], 0.0f);
			draw_list->AddRect(region.Min, region.Max, m->colors[4], 0.0f);

			std::string text = noteName(key, true);
			auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
			ImVec2 text_pos(x + (region.GetWidth() - textWidth) / 2.0f, region.Max.y - ImGui::GetFontSize() * 1.5f);
			draw_list->AddText(text_pos, IM_COL32_BLACK, text.c_str());

			x += note_width;
		}



		//
		// Dark keys
		//
		x = bb.Min.x;
		y = bb.Min.y;
		for (int i = 0; i != (m->octaves * 12); ++i) {
			if (!m->note_is_dark[i % 12]) {
				x += note_width;
				continue;
			}

			const int octave = m_octave + (i / 12);
			const int key = noteIndex(i % 12, octave);

			ImRect region = ImRect(x + note_dark_offset, y,
				x + note_dark_offset + note_width_dark, y + note_height_dark);

			if (held && region.Contains(mouse_pos)) {
				pressed_key = key;
				pressed_key_velocity = (mouse_pos.y - region.Min.y) / region.GetHeight();
			}

			bool pressed = isPressed(key);

			draw_list->AddRectFilled(region.Min, region.Max, m->colors[pressed ? 3 : 1], 0.0f);
			draw_list->AddRect(region.Min, region.Max, m->colors[4], 0.0f);


			std::string text = noteName(key, true);
			auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
			ImVec2 text_pos(x + note_dark_offset + (region.GetWidth() - textWidth) / 2.0f, region.Max.y - ImGui::GetFontSize() * 1.5f);
			draw_list->AddText(text_pos, IM_COL32_WHITE, text.c_str());
		}


		// check mouse clicks
		for (int key = 0; key != TotalNotes; ++key) {
			if (state(key) == KeyState::OnByMouse && key != pressed_key) {
				changePressed(key, KeyState::Off);
			}
			else if (state(key) == KeyState::Off && (pressed_key == key)) {
				changePressed(key, KeyState::OnByMouse, pressed_key_velocity);
			}
		}

		// check pc keyboard press
		for (int i = 0; i != m->pc_keys; ++i) {
			ImGuiKey current_pc_key = m->note_pc_key[i];
			const int octave = m_octave + (i / 12);
			const int key = noteIndex(i % 12, octave);

			bool is_ke_pressed = ImGui::IsKeyPressed(current_pc_key) && (ImGui::GetIO().KeyMods == 0);

			if (is_ke_pressed && (state(key) == KeyState::Off)) {
				changePressed(key, KeyState::OnByKeyboard, DefaultPressVelocity);
			}
			else if (ImGui::IsKeyReleased(current_pc_key) && state(key) == KeyState::OnByKeyboard) {
				changePressed(key, KeyState::Off);
			}
		}


		aboutToFinishRender();
		ImGui::End();
	}

	bool KeyboardWindow::isPressed(int key)
	{
		assert(key >= 0);
		assert(key < TotalNotes);

		return (m->keys[key] == KeyState::OnByMouse) || (m->keys[key] == KeyState::OnByKeyboard) || (m->keys[key] == KeyState::OnByMidi);
	}

	KeyboardWindow::KeyState KeyboardWindow::state(int key)
	{
		assert(key >= 0);
		assert(key < TotalNotes);

		return m->keys[key];
	}

	void KeyboardWindow::changePressed(int key, KeyState state, float velocity)
	{
		assert(key >= 0);
		assert(key < TotalNotes);

		//Log::d(TAG, sfmt("Key %d changed from %d to %d v=%.3f", key, KeyStateName(m->keys[key]), KeyStateName(state), velocity));

		if (m->keys[key] == state)
			return;


		m->keys[key] = state;
		m_app->engine().setInstrumentNote(m_app->activeInstrument(), key, velocity);
	}

	void KeyboardWindow::updateMidiController(std::set<int> const& pressed) {
		for (int key = 0; key != TotalNotes; ++key) {
			bool current_pressed = pressed.find(key) != pressed.end();

			if (current_pressed && (state(key) == KeyState::Off)) {
				m->keys[key] = KeyState::OnByMidi;
			}
			else if (!current_pressed && state(key) == KeyState::OnByMidi) {
				m->keys[key] = KeyState::Off;
			}
		}
	}

	void KeyboardWindow::changedOctave() {
		for (int key = 0; key != TotalNotes; ++key) {
			if (state(key) == KeyState::OnByMouse || state(key) == KeyState::OnByKeyboard) {
				changePressed(key, KeyState::Off);
			}
		}
	}

	void KeyboardWindow::panic() {
		for (int key = 0; key != TotalNotes; ++key)
			m->keys[key] = KeyState::Off;

		m_app->engine().panic();
	}

}
