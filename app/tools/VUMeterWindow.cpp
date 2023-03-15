#include "VUMeterWindow.hpp"

#include "../App.hpp"
#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"

namespace sns {

	VUMeterWindow::VUMeterWindow()
	{
		TAG = "VU";
		m_window_name = "VU Meter";
		m_levels = 20;
	}

	VUMeterWindow::~VUMeterWindow() {

	}

	void VUMeterWindow::render(){
		beforeRender();

		Analyser& analyser = app()->engine().analyser();
	
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

        if (m_showing && !analyser.isAccepting())
			analyser.start(TAG);

		float db = 30.0f;
		ImGui::Text("%d db", int(db));

		float h = ImGui::GetTextLineHeight() / 2.0f;
		float w = h * 8.0f;


		float step0 = 18.0f / 30.0f;
		float step1 = 24.0f / 30.0f;

		for (int i = 0; i != m_levels; ++i) {
			float factor = float(m_levels - i) / float(m_levels);

			float alpha = 0.2f;
			ImVec4 color;
			if (factor > step1) {
				color = ImVec4(1.0f, 0.0f, 0.0f, alpha);
			} else if (factor > step0)  {
				color = ImVec4(1.0f, 1.0f, 0.0f, alpha);
			} else {
				color = ImVec4(0.0f, 1.0f, 0.0f, alpha);
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + w, p.y + h), ImGui::ColorConvertFloat4ToU32(color));
            ImGui::Dummy(ImVec2(w, h));

		}

		if (!m_showing && analyser.isAccepting())
			analyser.stop(TAG);
		
		aboutToFinishRender();
		ImGui::End();
	}
}
