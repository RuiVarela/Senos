#include "VUMeterWindow.hpp"

#include "../App.hpp"
#include "../engine/core/Log.hpp"
#include "../vendor/imgui/imgui.h"

namespace sns {

	VUMeterWindow::VUMeterWindow()
	{
		TAG = "VU";
		m_window_name = "VU";
	}

	VUMeterWindow::~VUMeterWindow() {

	}

	void VUMeterWindow::render(){
		beforeRender();

		Analyser& analyser = app()->engine().analyser();
	
		ImGui::Begin(m_window_name.c_str(), &m_showing, ImGuiWindowFlags_NoResize);

        if (m_showing && !analyser.isAccepting())
			analyser.start(TAG);

        ImGui::Text("Super mario");

		if (!m_showing && analyser.isAccepting())
			analyser.stop(TAG);
		
		aboutToFinishRender();
		ImGui::End();
	}
}
