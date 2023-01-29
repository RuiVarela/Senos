#include "DebugWindow.hpp"
#include "../App.hpp"
#include "../Platform.hpp"

#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"

#include <mutex>
#include <fstream>
#include <iostream>



//static void fakeMidi(sns::Midi& midi, int k) {
//    sns::MidiMessage m;
//
//    m.port = "";
//    m.parameter = sns::ParameterNone;
//    m.timestamp = 0.0;
//    m.bytes.resize(3);
//
//    if (k == 0) { // send note on
//        m.bytes[0] = 0x90;
//        m.bytes[1] = sns::noteIndex(4, 4); // note
//        m.bytes[2] = 50;                   // velocity
//    }
//    else if (k == 1) {  // send note off
//        m.bytes[0] = 0x80;
//        m.bytes[1] = sns::noteIndex(4, 4); // note
//        m.bytes[2] = 50;                   // velocity
//    }
//    else if (k == 2) {  // send cc
//        m.bytes[0] = 0xb0;
//        m.bytes[1] = 74;
//        m.bytes[2] = 127;
//    }
//
//    midi.midiReceived(m);
//}

namespace sns {

	struct DebugWindow::PrivateImplementation {
		std::recursive_mutex logger_mutex;
		ImGuiTextBuffer logger_buf;
		ImGuiTextFilter logger_filter;
		ImVector<int> logger_line_offsets; // Index to lines offset. We maintain this with AddLog() calls.
		bool logger_auto_scroll = true;  // Keep scrolling if already at the bottom.

		bool show_imgui_demo_window = false;
	};

	DebugWindow::DebugWindow()
		:m(std::make_shared<PrivateImplementation>())
	{
		TAG = "App";
		m_window_name = "Debug";

		m->logger_auto_scroll = true;
		clearLog();
	}

	DebugWindow::~DebugWindow() {
		m.reset();
	}

	void DebugWindow::initialize() {
		Log::Handler handler = std::bind(&DebugWindow::addLog, this, std::placeholders::_1);
		Log::logger().setWindowHandler(handler);
	}

	void DebugWindow::shutdown() {
		Log::logger().setWindowHandler([](std::string const&) {});
	}

	void DebugWindow::render()
	{
		beforeRender();

		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		ImGui::Text("Video [fps: %d]", int(m_app->filteredFps()));

		ImGui::SameLine();

		{
			uint64_t produced_ms;
			float syntesis_average_ms;
			int filled_samples;
			app()->engine().stats(syntesis_average_ms, produced_ms, filled_samples);

			ImGui::Text("Audio [Synthesis: %.1fms, Packet: %dms, Generated: %s]",
				syntesis_average_ms, int(audioMilliseconds(filled_samples)), timecode(produced_ms).c_str());
		}

		auto keyboard = app()->getWindow<KeyboardWindow>();
		if (true) {
			int start = (12 * 3);
			int end = TotalNotes - (12 * 2);
			std::stringstream keys;
			keys << "[";
			bool started = false;

			for (int key = start; key != end; ++key) {
				if (key >= TotalNotes) break;

				if ((key % 12) == 0 && started)
					keys << " ";

				keys << int(keyboard->state(key));
				started = true;
			}
			keys << "]";

			ImGui::Text("Keys %s", keys.str().c_str());
		}

		{
			if (ImGui::Button("Tasks"))
				ImGui::OpenPopup("select_tasks");

			if (ImGui::BeginPopup("select_tasks")){

				if (ImGui::Button("Open Data Folder"))
					platformShellOpen(rootFolder());

				if (ImGui::Button("Geometry"))
					Log::d(TAG, sfmt("Width: %d Height: %d dpi_scale: %.1f font_size: %.1f", app()->width(), app()->height(), app()->dpiScale(), ImGui::GetFontSize()));
#ifndef NDEBUG
				if (ImGui::Button("ImGui"))
					m->show_imgui_demo_window = !m->show_imgui_demo_window;
#endif
				if (ImGui::Button("Show Notes Names"))
					showNotesNames();

				if (ImGui::Button("Show Notes Frequencies"))
					showNotesFrequencies();

				ImGui::EndPopup();
			}
		}

		// logs
		{
			ImGui::SameLine();
			int level = int(Log::logger().level());
			const char *items[] = {"Debug", "Info", "Warning", "Error"};
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
			if (ImGui::BeginCombo("Log", items[level], 0)) {
				for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
					const bool is_selected = (level == n);
					if (ImGui::Selectable(items[n], is_selected)) 
						Log::logger().setLeveL(LogLevel(n));
				}
				ImGui::EndCombo();
			}
		}


		renderLog();

		aboutToFinishRender();
		ImGui::End();



		// Show the ImGui test window. Most of the sample code is in ImGui::ShowDemoWindow()
		if (m->show_imgui_demo_window) {
			// ImGui::SetNextWindowPos(ImVec2(100, 20), ImGuiCond_FirstUseEver);
			ImGui::ShowDemoWindow();
		}
	}

	//
	// Log
	//
	void DebugWindow::clearLog() {
		std::unique_lock<std::recursive_mutex> lock(m->logger_mutex);

		m->logger_buf.clear();
		m->logger_line_offsets.clear();
		m->logger_line_offsets.push_back(0);
	}

	void DebugWindow::addLog(std::string const& message) {
		if (message.empty()) return;

		std::unique_lock<std::recursive_mutex> lock(m->logger_mutex);

		int old_size = m->logger_buf.size();
		m->logger_buf.append(message.c_str());
		m->logger_buf.append("\n");

		for (int new_size = m->logger_buf.size(); old_size < new_size; old_size++)
			if (m->logger_buf[old_size] == '\n')
				m->logger_line_offsets.push_back(old_size + 1);
	}

	void DebugWindow::renderLog()
	{
		std::unique_lock<std::recursive_mutex> lock(m->logger_mutex);

		ImGui::Separator();

		std::string title = "-- LOG --";
		auto windowWidth = ImGui::GetWindowSize().x;
		auto textWidth = ImGui::CalcTextSize(title.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::TextUnformatted(title.c_str());



		// Options menu
		if (ImGui::BeginPopup("Options")) {
			ImGui::Checkbox("Auto-scroll", &m->logger_auto_scroll);
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");

		ImGui::SameLine();
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		m->logger_filter.Draw("Filter", -100.0f);

		ImGui::BeginChild("scrolling", ImVec2(0, 250), false, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

		if (clear)
			clearLog();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = m->logger_buf.begin();
		const char* buf_end = m->logger_buf.end();
		if (m->logger_filter.IsActive())
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of
			// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
			for (int line_no = 0; line_no < m->logger_line_offsets.Size; line_no++)
			{
				const char* line_start = buf + m->logger_line_offsets[line_no];
				const char* line_end = (line_no + 1 < m->logger_line_offsets.Size) ? (buf + m->logger_line_offsets[line_no + 1] - 1) : buf_end;
				if (m->logger_filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}
		}
		else
		{
			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
			// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
			// within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
			// on your side is recommended. Using ImGuiListClipper requires
			// - A) random access into your data
			// - B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display
			// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
			// it possible (and would be recommended if you want to search through tens of thousands of entries).
			ImGuiListClipper clipper;
			clipper.Begin(m->logger_line_offsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + m->logger_line_offsets[line_no];
					const char* line_end = (line_no + 1 < m->logger_line_offsets.Size) ? (buf + m->logger_line_offsets[line_no + 1] - 1) : buf_end;
					ImGui::TextUnformatted(line_start, line_end);
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		if (m->logger_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();

	}

	void DebugWindow::showNotesNames() {
		std::stringstream names0;
		std::stringstream names1;
		for (int key = 0; key != 12; ++key) {
			names0 << noteName(key, false, false) << " ";
			names1 << noteName(key, false, true) << " ";
		}

		Log::i(TAG, sfmt("Notes: %s", names0.str()));
		Log::i(TAG, sfmt("Notes: %s", names1.str()));
	}
	
	void DebugWindow::showNotesFrequencies() {
		for (int i = 0; i != TotalNotes; ++i)
			Log::i(TAG, sfmt("midi=%d name=%s frequency=%.2f", i, noteName(i, true), noteFrequency(i)));
	}
}
