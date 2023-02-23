#include "ScopeWindow.hpp"


#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"


namespace sns {

	ScopeWindow::ScopeWindow()
	{
		TAG = "Scope";
		m_window_name = "Scope";

	}

	ScopeWindow::~ScopeWindow() {

	}

	void ScopeWindow::render(){
		beforeRender();

		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

		float width = ImGui::GetTextLineHeightWithSpacing() * 2.0 * 10.0f;


		static float values[100] = {};
        static int values_offset = 0;

		static float phase = 0.0f;
		values[values_offset] = cosf(phase);
		values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
		phase += 0.10f * values_offset;

		ImGui::PlotLines("##Lines", values, IM_ARRAYSIZE(values), 
			0, "", -1.0f, 1.0f, 
			ImVec2(width, width / 2.0f));
        
		aboutToFinishRender();
		ImGui::End();
	}
}
