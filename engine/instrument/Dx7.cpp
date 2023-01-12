#include "Dx7.hpp"
#include "../core/Log.hpp"
#include "synthmachine/Value.hpp"

#include "dx7/synth.h"
#include "dx7/freqlut.h"
#include "dx7/sin.h"
#include "dx7/exp2.h"
#include "dx7/pitchenv.h"
#include "dx7/env.h"
#include "dx7/patch.h"
#include "dx7/tuning.h"
#include "dx7/controllers.h"
#include "dx7/dx7note.h"
#include "dx7/lfo.h"

#include "dx7/banks/Banks.hpp"

using namespace dx7;

namespace sns {
	constexpr int max_active_notes = 16;

	constexpr float VOLUME_RAMP_INCREMENT = 10.0f / float(SampleRate);

	struct Voice {
		int midi_note;
		bool keydown;
		bool sustained;
		bool live;
		Dx7Note* dx7_note;
	};

	struct Dx7::PrivateImplementation {
		Voice voices[max_active_notes];
		int current_note;

		uint8_t patch_data[4096];
		char unpacked_patch[156];

		// The original DX7 had one single LFO. Later units had an LFO per note.
		Lfo lfo;

		// in MIDI units (0x4000 is neutral)
		Controllers controllers;

		bool sustain;

		uint8_t midi_channel = 0;

		AlignedBuf<int32_t, N> audiobuffer;
		std::array<float, N> produced;
		size_t produced_index;

		FmCore core;

		int group_index;
		int bank_index;
		int program_index;
		bool mono;

		Value volume;

		bool do_log;
	};

	Dx7::Dx7()
		: m(std::make_unique<PrivateImplementation>())
	{
		TAG = "Dx7";

		m->do_log = false;

		// global initialization
		double sample_rate = SampleRate;
		Freqlut::init(sample_rate);
		Exp2::init();
		Tanh::init();
		Sin::init();
		Lfo::init(sample_rate);
		PitchEnv::init(sample_rate);
		Env::init_sr(sample_rate);

		// instance initialization
		for (int note = 0; note < max_active_notes; ++note) {
			m->voices[note].dx7_note = new Dx7Note(createStandardTuning());
			m->voices[note].keydown = false;
			m->voices[note].sustained = false;
			m->voices[note].live = false;
		}

		m->current_note = 0;

		m->controllers.core = &m->core;
		m->controllers.masterTune = 0;
		m->controllers.values_[kControllerPitchRangeUp] = 3;
		m->controllers.values_[kControllerPitchRangeDn] = 3;
		m->controllers.values_[kControllerPitchStep] = 0;
		m->controllers.values_[kControllerPitch] = 0x2000;
		m->controllers.modwheel_cc = 0;
		m->controllers.foot_cc = 0;
		m->controllers.breath_cc = 0;
		m->controllers.aftertouch_cc = 0;
		m->controllers.refresh();

		m->sustain = false;

		memset(m->audiobuffer.get(), 0, N * sizeof(int32_t));
		memset(m->produced.data(), 0, N * sizeof(float));
		m->produced_index = N;

		m->mono = false;
		m->group_index = -1;
		m->bank_index = -1;
		m->program_index = -1;

		m->volume = 1.0f;

		Dx7::setValues(Dx7::defaultParameters());
		m->do_log = true;
	}

	Dx7::~Dx7() {

	}

	ParametersValues Dx7::defaultParameters() {
		ParametersValues values;

		values[ParameterGroup] = 0.0f;
		values[ParameterBank] = 0.0f;
		values[ParameterPatch] = 0.0f;

		values[ParameterModulationWheelRange] = 0.0f;
		values[ParameterModulationWheelPitch] = 0.0f;
		values[ParameterModulationWheelAmp] = 0.0f;
		values[ParameterModulationWheelEnv] = 0.0f;

		values[ParameterAftertouchRange] = 0.0f;
		values[ParameterAftertouchPitch] = 0.0f;
		values[ParameterAftertouchAmp] = 0.0f;
		values[ParameterAftertouchEnv] = 0.0f;

		values[ParameterPitchBendStep] = 0.0f;
		values[ParameterPitchBendUp] = 3.0f;
		values[ParameterPitchBendDown] = 3.0f;

		values[ParameterMono] = 0.0f;
		values[ParameterVolume] = 1.0f;

		return values;
	}

	void Dx7::setValues(ParametersValues const& values) {
		int group = -1;
		int bank = -1;
		int program = -1;

		for (auto const& [p, v] : values) {
			int iv = int(v);
			bool ib = bool(iv);
			bool changed = false;
			bool controllers_changed = false;

			switch (p) {
				// banks
			case ParameterGroup: group = int(v); break;
			case ParameterBank: bank = int(v); break;
			case ParameterPatch: program = int(v); break;


				// Pitch bend
			case ParameterPitchBendStep:
				if (iv != m->controllers.values_[kControllerPitchStep]) {
					m->controllers.values_[kControllerPitchStep] = iv;
					changed = true;
				}
				break;
			case ParameterPitchBendUp:
				if (iv != m->controllers.values_[kControllerPitchRangeUp]) {
					m->controllers.values_[kControllerPitchRangeUp] = iv;
					changed = true;
				}
				break;
			case ParameterPitchBendDown:
				if (iv != m->controllers.values_[kControllerPitchRangeDn]) {
					m->controllers.values_[kControllerPitchRangeDn] = iv;
					changed = true;
				}
				break;

				// Mod Wheel
			case ParameterModulationWheelRange:
				if (iv != m->controllers.wheel.range) {
					m->controllers.wheel.range = iv;
					controllers_changed = true;
				}
				break;
			case ParameterModulationWheelPitch:
				if (ib != m->controllers.wheel.pitch) {
					m->controllers.wheel.pitch = ib;
					controllers_changed = true;
				}
				break;
			case ParameterModulationWheelAmp:
				if (ib != m->controllers.wheel.amp) {
					m->controllers.wheel.amp = ib;
					controllers_changed = true;
				}
				break;
			case ParameterModulationWheelEnv:
				if (ib != m->controllers.wheel.eg) {
					m->controllers.wheel.eg = ib;
					controllers_changed = true;
				}
				break;

				// after touch
			case ParameterAftertouchRange:
				if (iv != m->controllers.at.range) {
					m->controllers.at.range = iv;
					controllers_changed = true;
				}
				break;
			case ParameterAftertouchPitch:
				if (ib != m->controllers.at.pitch) {
					m->controllers.at.pitch = ib;
					controllers_changed = true;
				}
				break;
			case ParameterAftertouchAmp:
				if (ib != m->controllers.at.amp) {
					m->controllers.at.amp = ib;
					controllers_changed = true;
				}
				break;
			case ParameterAftertouchEnv:
				if (ib != m->controllers.at.eg) {
					m->controllers.at.eg = ib;
					controllers_changed = true;
				}
				break;

				// Mono
			case ParameterMono:
				if (ib != m->mono) {
					m->mono = ib;
					panic();
					changed = true;
				}
				break;

				// Volume
			case ParameterVolume:
			{
				iv = int(v * 100.0f);
				m->volume.changeWithIncrement(v, VOLUME_RAMP_INCREMENT);
				changed = true;
			}
			break;

			default: break;
			}

			if (controllers_changed) {
				m->controllers.refresh();
			}

			if ((changed || controllers_changed) && m->do_log) {
				Log::i(TAG, sfmt("Updating value [%s] -> [%.3f]", parameterToString(p), iv));
			}
		}

		if (group > -1 && bank > -1 && program > -1) {
			if (group != m->group_index || bank != m->bank_index) {
				Banks::BankInfo info = Banks::instance().bank(group, bank);
				if (info.data && setSysex(info.data, info.size)) {

					if (m->do_log)
						Log::d(TAG, sfmt("Loaded %s/%s", info.group, info.bank));

					m->group_index = group;
					m->bank_index = bank;
					m->program_index = -1;
				}
				else {

					Log::e(TAG, sfmt("Invalid group=%d bank=%d", group, bank));
					m->group_index = -1;
					m->bank_index = -1;
					m->program_index = -1;
				}
			}

			if (m->bank_index > -1 && (m->program_index != program)) {
				if (program < 32) {
					programChange(program);
					m->program_index = program;
				}
				else {
					Log::e(TAG, sfmt("Invalid program=%d", program));
				}
			}
		}

		BaseInstrument::setValues(values);
	}

	void Dx7::panic() {
		BaseInstrument::panic();

		for (int i = 0; i < max_active_notes; i++) {
			resetVoice(i);
		}
		internalTrackMidiNotesReset();
	}

	void Dx7::resetVoice(int v) {
		m->voices[v].sustained = false;
		m->voices[v].keydown = false;
		m->voices[v].live = false;
		m->voices[v].dx7_note->oscSync();
	}

	void Dx7::setController(int controller, int value) {
		m->controllers.values_[controller] = value;
	}

	void Dx7::programChange(int p) {
		if (m->do_log)
			Log::d(TAG, sfmt("Changing program [%d]", p));

		const uint8_t* patch = m->patch_data + 128 * p;
		setPatch(patch, 128);
		panic();
	}

	bool Dx7::setSysex(const uint8_t* data, uint32_t size) {
		if ((size >= 4104) && (data[1] == 0x43 && data[2] == 0x00 && data[3] == 0x09 && data[4] == 0x20 && data[5] == 0x00)) {
			memcpy(m->patch_data, data + 6, 4096);
			return true;
		}
		return false;
	}

	void Dx7::setPatch(const uint8_t* patch, uint32_t size) {
		char name[11];

		if (size == 128) {
			UnpackPatch((const char*)patch, m->unpacked_patch);
			memcpy(name, m->unpacked_patch + 145, 10);
			name[10] = 0;
		}
		else if (size == 145) {
			memcpy(m->unpacked_patch, patch, size);
			m->unpacked_patch[155] = 0x3f;  // operator on/off

			strcpy(name, "custom");
			name[6] = 0;
		}
		else {
			return;
		}

		//lfo_.reset((const char *)unpacked_patch_);
		m->lfo.reset(m->unpacked_patch + 137);

		if (m->do_log)
			Log::d(TAG, sfmt("Program Changed [%s]", name));
	}

	void Dx7::setParam(uint32_t id, char value) {
		m->unpacked_patch[id] = value;
	}

	void Dx7::setNote(int note, float velocity) {
		if (velocity > 0.0f) {
			const int ivelocity = int(velocity * 127.0f);
			midiNotePressed(note, ivelocity);
		}
		else {
			midiNoteReleased(note);
		}
	}

	void Dx7::onMidi(MidiMessage const& message) {
		if ((message.parameter != ParameterNone) && (message.parameter != ParameterPitchBend)) {
			BaseInstrument::onMidi(message);
		}
		else {
			onMidi(message.bytes.data(), int(message.bytes.size()));
		}

	}

	void Dx7::midiNotePressed(int midinote, int velocity) {

		if (m->mono) {
			for (int i = 0; i < max_active_notes; i++) {
				if (m->voices[i].live) {
					resetVoice(i);
				}
			}
		}

		int note = m->current_note;
		for (int i = 0; i < max_active_notes; i++) {
			if (!m->voices[note].keydown) {
				m->current_note = (note + 1) % max_active_notes;

				m->lfo.keydown(); // TODO: should only do this if # keys down was 0
				m->voices[note].midi_note = midinote;
				m->voices[note].keydown = true;
				m->voices[note].sustained = m->sustain;
				m->voices[note].live = true;
				m->voices[note].dx7_note->init((uint8_t*)m->unpacked_patch, midinote, velocity);
				break;
			}
			note = (note + 1) % max_active_notes;
		}

	}

	void Dx7::midiNoteReleased(int midinote) {
		int note;

		for (note = 0; note < max_active_notes; ++note)
			if (m->voices[note].midi_note == midinote && m->voices[note].keydown)
				break;

		// note not found ?
		if (note >= max_active_notes) {
			Log::i(TAG, sfmt("Note released not found value [%d]", midinote));
			return;
		}

		if (m->sustain) {
			m->voices[note].sustained = true;
		}
		else {
			m->voices[note].dx7_note->keyup();
		}
		m->voices[note].keydown = false;
	}

	void Dx7::onMidi(uint8_t const* data, int data_size) {
		uint8_t cmd = data[0];
		uint8_t cmd_type = cmd & 0xf0;

		if (cmd_type == 0x80 || (cmd_type == 0x90 && data[2] == 0)) {
			// note off
			if (data_size < 3) return;

			midiNoteReleased(data[1]);
			internalTrackMidiNotePressed(data[1], false);

		}
		else if (cmd_type == 0x90) {
			// note on
			if (data_size < 3) return;

			midiNotePressed(data[1], data[2]);
			internalTrackMidiNotePressed(data[1], true);

		}
		else if (cmd_type == 0xb0) {
			// controller
			if (data_size < 3) return;

			int controller = data[1];
			int value = data[2];

			if (controller == 1) {
				m->controllers.modwheel_cc = value;
				m->controllers.refresh();
			}
			else if (controller == 2) {
				m->controllers.breath_cc = value;
				m->controllers.refresh();
			}
			else if (controller == 3) {
				m->controllers.foot_cc = value;
				m->controllers.refresh();
			}
			else if (controller == 64) {
				m->sustain = value != 0;
				if (!m->sustain) {
					for (int note = 0; note < max_active_notes; note++) {
						if (m->voices[note].sustained && !m->voices[note].keydown) {
							m->voices[note].dx7_note->keyup();
							m->voices[note].sustained = false;
						}
					}
				}
			}

		}
		else if (cmd_type == 0xd0) {
			m->controllers.aftertouch_cc = data[1];
			m->controllers.refresh();
		}
		else if (cmd_type == 0xc0) {
			// program change
			if (data_size < 2) return;
			programChange(std::min(int(data[1]), 31));

		}
		else if (cmd == 0xe0) {
			// pitch bend
			if (data_size < 3) return;

			setController(kControllerPitch, data[1] | (data[2] << 7));

		}
		else if (cmd == 0xf0) {
			//sysex

			if (setSysex(data, data_size))
				programChange(0);
		}
	}

	float Dx7::next() {

		if (m->produced_index == N) {
			int32_t lfovalue = m->lfo.getsample();
			int32_t lfodelay = m->lfo.getdelay();
			for (int note = 0; note < max_active_notes; ++note) {
				if (m->voices[note].live) {
					m->voices[note].dx7_note->compute(m->audiobuffer.get(), lfovalue, lfodelay, &m->controllers);

					for (int j = 0; j < N; ++j) {
						int32_t val = m->audiobuffer.get()[j];
						val = val >> 4;
						int clip_val = val < -(1 << 24) ? 0x8000 : val >= (1 << 24) ? 0x7fff : val >> 9;
						float f = ((float)clip_val) / (float)0x8000;
						m->produced[j] += HardClip(f);
						m->audiobuffer.get()[j] = 0;
					}
				}
			}
			m->produced_index = 0;
		}

		if (m->produced_index != N) {
			float value = m->produced[m->produced_index] * m->volume.next();
			m->produced[m->produced_index] = 0.0f;
			m->produced_index++;
			return value;
		}

		return 0.0f;
	}
}