#pragma once

#include "core/Worker.hpp"
#include "audio/CircularBuffer.hpp"
#include "audio/RunningAverage.hpp"
#include "audio/Recorder.hpp"
#include "audio/Midi.hpp"
#include "instrument/Instrument.hpp"
#include "Sequencer.hpp"
#include "Chainer.hpp"

namespace sns {

	class FeedbackCallback {
		public:
		FeedbackCallback() {}
		virtual ~FeedbackCallback() {}
		virtual void onInstrumentParamsFeedback(InstrumentId instrument_id, ParametersValues const& values) {}
		virtual void onMidiNotesFeedback(InstrumentId instrument_id, std::set<int> const& notes_pressed) {}

		virtual void onSequencerState(Sequencer::State state) {}
		virtual void onChainerState(Chainer::State state) {}
	};

	class Engine {
	public:
		Engine();
		~Engine();

		void setFeedbackCallback(FeedbackCallback* callback);

		Recorder& recorder();
		Midi& midi();
		Sequencer& sequencer();
		Chainer& chainer();

		void fill(float* buffer, int num_frames, int num_channels);
		void stats(float& synthesis_average_ms, uint64_t& produced_ms, int& last_fill_samples);

		void setInstrumentParams(InstrumentId instrument_id, ParametersValues const& values);
		void setInstrumentNote(InstrumentId instrument_id, int note, float velocity);
		void setSequencerConfiguration(Sequencer::Configuration const& configuration);
		void setChainerConfiguration(Chainer::Configuration const& configuration);

		void produceSamples(uint64_t count, float* buffer);

		void panic();
	private:
		std::string TAG;
		std::recursive_mutex m_sync_mutex;
		int m_last_fill_samples; // last filled samples
		uint64_t m_produced_samples_counter; // total produced samples
		RunningAverage m_synthesis_duration_ms; // average time we take to produce a sample

		std::array<std::unique_ptr<BaseInstrument>, InstrumentCount> m_instruments;
		Recorder m_recorder;
		Midi m_midi;
		Sequencer m_sequencer;
		Chainer m_chainer;

		std::list<std::function<void()>> m_actions;

		FeedbackCallback* m_feedback_callback;
	};

}