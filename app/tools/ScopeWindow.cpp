#include "ScopeWindow.hpp"

#include "../App.hpp"
#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"


namespace sns {

	ScopeWindow::ScopeWindow()
	{
		TAG = "Scope";
		m_window_name = "Scope";

		m_active = true;
		m_offset_factor = 0.0f;

		m_time_values = {1, 2, 5, 10, 20, 25, 50, 100, 150, 200};
		for (auto current : m_time_values)
			m_time_names.push_back(sfmt("%d ms", current));
		m_time = 3;


		m_points_values = { 20, 50, 100, 150, 200 };
		for (auto current : m_points_values)
			m_points_names.push_back(sfmt("%d", current));
		m_points = 2;


		m_sync_values = {AnalyserSync::None, AnalyserSync::RiseZero, AnalyserSync::FallZero};
		for (auto current : m_sync_values)
			m_sync_names.push_back(toString(current));
		m_sync = 0;
	}

	ScopeWindow::~ScopeWindow() {

	}

	void ScopeWindow::render(){
		beforeRender();

		Analyser& analyser = app()->engine().analyser();
	

		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		if (m_showing && !analyser.isAccepting() && m_active)
			updateCapture();

		float base_size = ImGui::GetTextLineHeightWithSpacing() * 2.0f;

		app()->engine().analyser().generateGraph(m_samples);

		ImGui::PlotLines("##Lines", m_samples.data(), int(m_samples.size()), 0, 
			"", -1.0f, 1.0f, 
			ImVec2(base_size * 10.0f, base_size * 5.0f));

		ImGui::SameLine();

		//
		// Pickers
		//
		ImGui::BeginChild("options_block", ImVec2(base_size * 4.0f, 0), true);
		{
			ImGui::Checkbox("Running", &m_active);

			if (pCombo("Time", m_time_names, m_time))
				updateCapture();
			
			float f = 1.0f - m_offset_factor;
			if (ImGui::SliderFloat("Offset", &f, 0.0f, 1.0f, "")) {
				m_offset_factor = 1.0f - f;
				updateCapture();
			}

			if (pCombo("Sync", m_sync_names, m_sync))
				updateCapture();

			ImGui::Separator();

			if (pCombo("Points", m_points_names, m_points))
				updateCapture();


		}
		ImGui::EndChild();



		if (analyser.isAccepting() && (!m_showing || !m_active)) {
			analyser.stop(TAG);
		}

		aboutToFinishRender();
		ImGui::End();
	}

	void ScopeWindow::updateCapture() {
		Analyser& analyser = app()->engine().analyser();

		analyser.configureGraph(
			m_points_values[m_points], 
			m_time_values[m_time], 
			m_offset_factor,
			m_sync_values[m_sync]);

		if (m_active)
			analyser.start(TAG);
	}
}
