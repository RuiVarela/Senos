#pragma once

#include "../Window.hpp"

namespace sns {

	class DebugWindow : public Window {
	public:
		DebugWindow();
		~DebugWindow() override;


		void initialize() override;
		void shutdown() override;

		void dumpTests();

		void render() override;

		void clearLog();
		void addLog(std::string const& message);
	private:
		struct PrivateImplementation;
		std::shared_ptr<PrivateImplementation> m;

		void renderLog();
	};
}
