#include "VUMeterWindow.hpp"

#include "../App.hpp"
#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"

namespace sns {

	VUMeterWindow::VUMeterWindow()
	{
		TAG = "VU";
		m_window_name = "VU Meter";
		m_levels = 24;
	}

	VUMeterWindow::~VUMeterWindow() {

	}

	void VUMeterWindow::render(){
		beforeRender();

		Analyser& analyser = app()->engine().analyser();
	
		ImGui::Begin("VU", &m_showing, ImGuiWindowFlags_NoResize);

        if (m_showing && !analyser.isAccepting())
			analyser.start(TAG);

		const float current_linear = analyser.peak();
		const float current_db = toDb(current_linear);
		const std::string text = sfmt("%05.1f db", current_db);

		const float base_size = ImGui::GetTextLineHeightWithSpacing();
		const float h = ImGui::GetTextLineHeight();
		const float w = h * 2.3f;

		const float step0 = 21.0f / 30.0f;
		const float step1 = 27.0f / 30.0f;
		const float step_delta = 1.0f / float(m_levels);

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text.c_str()).x) / 2.f);
		ImGui::Text("%s", text.c_str());

		ImGui::BeginChild("bars_block", ImVec2(w, m_levels * base_size + base_size), false);
		{
			ImGui::Dummy(ImVec2(base_size, h/3.0f));

			for (int i = 0; i != m_levels; ++i) {
				float f_start = float(m_levels - i - 1) * step_delta;
				float alpha = (current_linear > f_start) ? 1.0 : 0.2f;
				ImVec4 color;
				if (f_start >= step1) {
					color = ImVec4(1.0f, 0.0f, 0.0f, alpha);
				} else if (f_start >= step0)  {
					color = ImVec4(1.0f, 1.0f, 0.0f, alpha);
				} else {
					color = ImVec4(0.0f, 1.0f, 0.0f, alpha);
				}

				ImVec2 p = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), ImGui::ColorConvertFloat4ToU32(color));
				ImGui::Dummy(ImVec2(w, h));

			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("labels_block", ImVec2(base_size * 1.8f, (m_levels + 1) * base_size), false);
		{
			for (int i = 0; i != (m_levels + 1); ++i) {
				float factor = float(m_levels - i) * step_delta;
				float db = toDb(factor);
				if (equivalent(db, 0.0f)) 
					ImGui::Text(" 0");
				else
					ImGui::Text("%05.1f", db);
			}
		}
		ImGui::EndChild();



		if (!m_showing && analyser.isAccepting())
			analyser.stop(TAG);
		
		aboutToFinishRender();
		ImGui::End();
	}
}
