#pragma once

#include "../Window.hpp"

namespace sns {

	class TB303Window : public Window {
	public:
		TB303Window();
		~TB303Window() override;

		void render() override;
	protected:
		void renderBassLine();

	};
}