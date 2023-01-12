#pragma once

#include "Window.hpp"

namespace sns {

	class AboutWindow : public Window {
	public:
		AboutWindow();
		~AboutWindow() override;

		void render() override;

	};
}
