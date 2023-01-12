#pragma once

#include "../Window.hpp"
#include "../Configuration.hpp"

namespace sns {

	class ChainerWindow : public Window {
	public:
		ChainerWindow();
		~ChainerWindow() override;

		void render() override;
		void stateChanged(Chainer::State const& state);

		void reloadSequences();
		bool playing();
		void stop();

		static void inflate(sns::Configuration const& cfg, Chainer::Configuration& chainer_cfg);
		static void deflate(Chainer::Configuration& chainer_cfg);
	private:

		void renderTopBar();
		void renderTable(float height);

		void onSavePreset(std::string const& name) override;
		void onLoadPreset(std::string const& name) override;
		void onDeletePreset(std::string const& name) override;
		void onRefreshPresets() override;

		int m_selected;
		Chainer::Configuration m_cfg;
		bool m_cfg_loaded;
		Chainer::State m_state;

		void sequencerStop();

		std::vector<std::string> m_sequence_names;
	};

}