#include "Engine.hpp"
#include "core/Log.hpp"
#include "instrument/SynthMachine.hpp"
#include "instrument/DrumMachine.hpp"
#include "instrument/Dx7.hpp"
#include "instrument/TB303.hpp"

namespace sns {

    Engine::Engine()
        :TAG("Engine"),
        m_last_fill_samples(0),
        m_produced_samples_counter(0),
        m_synthesis_duration_ms(sns::SampleRate / 2048),
        m_feedback_callback(nullptr)
    {
        m_instruments[InstrumentIdSynthMachine] = std::make_unique<SynthMachine>();
        m_instruments[InstrumentIdDrumMachine] = std::make_unique<DrumMachine>();
        m_instruments[InstrumentIdDx7] = std::make_unique<Dx7>();
        m_instruments[InstrumentIdTB303] = std::make_unique<TB303>();

        setFeedbackCallback(nullptr);

        m_sequencer.setEngine(this);
        m_chainer.setEngine(this);

        m_midi.startWorking();
    }

    Engine::~Engine() {

    }

    void Engine::setFeedbackCallback(FeedbackCallback* callback) {
        m_feedback_callback = callback;

        if (callback)
            callback = new FeedbackCallback();
    }

    Recorder& Engine::recorder() {
        return m_recorder;
    }

    Midi& Engine::midi() {
        return m_midi;
    }

    Sequencer& Engine::sequencer() {
        return m_sequencer;
    }

    Chainer& Engine::chainer(){
        return m_chainer;
    }

    void Engine::stats(float& synthesis_average_ms, uint64_t& produced_ms, int& last_fill_samples) {
        std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);
        synthesis_average_ms = m_synthesis_duration_ms.average();
        produced_ms = audioMilliseconds(m_produced_samples_counter);
        last_fill_samples = m_last_fill_samples;
    }

    void Engine::produceSamples(uint64_t count, float *buffer) {
        std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);

        int64_t ts = getCurrentMilliseconds();

        // dispatch midi messages
        {
            auto messages = midi().take();
            for (auto current : messages) 
                if (m_instruments[current.instrument]) 
                    m_instruments[current.instrument]->onMidi(current);
            

            if (!messages.empty()) {
                for (int i = InstrumentStart; i != InstrumentCount; ++i) {
                    bool values;
                    bool notes;

                    m_instruments[i]->takeMidiControllerUpdates(values, notes);

                    if (values)
                        m_feedback_callback->onInstrumentParamsFeedback(i, m_instruments[i]->getValues());

                     if (notes)
                        m_feedback_callback->onMidiNotesFeedback(i, m_instruments[i]->getNotesPressedOnMidiController());
                }
            }
        }

        // dispatch actions
        while (!m_actions.empty()) {
            auto action = m_actions.front();
            m_actions.pop_front();
            action();
        }


        for (uint64_t s = 0; s != count; ++s) {
            //
            // update chainer
            //
            if (m_chainer.next()) 
                m_feedback_callback->onChainerState(m_chainer.state());

            //
            // update sequencer
            //
            if (m_sequencer.next()) 
                m_feedback_callback->onSequencerState(m_sequencer.state());
            
            //
            // render instruments
            //
            float sample = 0.0f;
            for (int i = InstrumentStart; i != InstrumentCount; ++i)
                sample += m_instruments[i]->next();
                
            // make sure we don't go overboard
            sample = HardClip(sample);

            //
            // send to output
            //
            buffer[s] = sample;
            
            //
            // send to recording
            //
            if (m_recorder.isAccepting()) {
                m_recorder.push(sample);
            }

            m_produced_samples_counter++;
        }

        int64_t work_duration = getCurrentMilliseconds() - ts;
        m_synthesis_duration_ms.add(float(work_duration));
        m_last_fill_samples = int(count);
    }

    void Engine::fill(float* buffer, int num_frames, int num_channels) {
        int needed_samples = num_frames * num_channels;
        produceSamples(needed_samples, buffer);
    }

	void Engine::setInstrumentParams(InstrumentId instrument_id, ParametersValues const& values) {
        assert(instrument_id > 0);
        assert(instrument_id < InstrumentCount);

		std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);
        auto action = [this, instrument_id, values] { m_instruments[instrument_id]->setValues(values); };
        m_actions.push_back(action);
	}

	void Engine::setInstrumentNote(InstrumentId instrument_id, int note, float velocity) {
        assert(instrument_id > 0);
        assert(instrument_id < InstrumentCount);

		std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);
        auto action = [this, instrument_id, note, velocity] { m_instruments[instrument_id]->setNote(note, velocity); };
        m_actions.push_back(action);
	}

    void Engine::setSequencerConfiguration(Sequencer::Configuration const& configuration) {
        std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);
        auto action = [this, configuration] { m_sequencer.apply(configuration); };
        m_actions.push_back(action);
    }

    void Engine::setChainerConfiguration(Chainer::Configuration const& configuration) {
        std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);
        auto action = [this, configuration] { m_chainer.apply(configuration); };
        m_actions.push_back(action);
    }

    void Engine::panic() {
        std::unique_lock<std::recursive_mutex> lock(m_sync_mutex);
        auto action = [this] { 

            m_chainer.panic();
            m_sequencer.panic();

            for (auto& instrument: m_instruments)
                if (instrument)
                    instrument->panic();

        };
        m_actions.push_back(action);
    }
}