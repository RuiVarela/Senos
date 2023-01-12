#include "Midi.hpp"
#include "../core/Log.hpp"

#define LIBREMIDI_HEADER_ONLY 1

#if defined(__APPLE__)
	#define LIBREMIDI_COREAUDIO 1
#elif defined(_WIN32)
	#define  LIBREMIDI_WINMM 1
#endif


#include "libremidi/libremidi.hpp"

namespace sns {

	template<class A>
	std::vector<std::string> uniquePorts(A const& names) {
		std::vector<std::string> filtered;

		for (auto const& current : names) {
			std::string name = current.port;

			if (!name.empty() && !contains(filtered, name))
				filtered.push_back(name);
		}
		std::sort(filtered.begin(), filtered.end());

		return filtered;
	};


	Midi::Mapping::Mapping()
		:filter_active_instrument(true)
	{
		for (auto& current : instruments)
			current.channel = -1;
	}

	struct Midi::PrivateImplementation
	{
		Midi::Mapping active_mapping;
		Midi::Mapping next_mapping;

		bool needs_mapping_update = false;
		ElapsedTimer needs_enumerate;
		bool needs_dump_info = true;

		InstrumentId active_instrument = 0;

		std::vector<std::string> available_ports;
		MidiMessages messages;

		std::mutex mutex;

		std::vector<std::shared_ptr<libremidi::midi_in>> in;
		std::vector<std::string> in_names;

		std::shared_ptr<libremidi::midi_in> midi_in_helper;
	};

	Midi::Midi()
		:m(std::make_unique<PrivateImplementation>()),
		TAG("Midi")
	{
		m->needs_enumerate.invalidate();
		setSleepMs(3000);
	}

	Midi::~Midi() {
		Worker::stopWorking();
	}

	void Midi::setMapping(Mapping const& mapping) {
		std::unique_lock<std::mutex> lock(m->mutex);

		m->next_mapping = mapping;
		m->needs_mapping_update = true;

		signalWorkArrived();
	}

	void Midi::setActiveInstrument(InstrumentId instrument) {
		std::unique_lock<std::mutex> lock(m->mutex);

		m->active_instrument = instrument;
	}

	MidiMessages Midi::take() {
		MidiMessages messages;

		{
			std::unique_lock<std::mutex> lock(m->mutex);
			std::swap(messages, m->messages);
		}

		return messages;
	}

	std::vector<std::string> Midi::ports() {
		std::unique_lock<std::mutex> lock(m->mutex);
		return m->available_ports;
	}

	void Midi::preWork() {
		midiTeardown();
		m->midi_in_helper = std::make_shared<libremidi::midi_in>();
	}

	void Midi::postWork() {
		midiTeardown();
		m->midi_in_helper.reset();
	}

	void Midi::workStep() {
		//
		// dump info
		//
		if (m->needs_dump_info) {
			midiDumpInfo();
			m->needs_dump_info = false;
		}

		//
		// enumerate
		//
		if (!m->needs_enumerate.isValid() || m->needs_enumerate.hasExpired(5000)) {
			midiEnumerate();
			m->needs_enumerate.start();
		}

		//
		// update configuration
		//
		if (m->needs_mapping_update) {

			bool update = false;
			Mapping mapping;

			{
				std::unique_lock<std::mutex> lock(m->mutex);

				update = m->needs_mapping_update;
				mapping = m->next_mapping;

				m->needs_mapping_update = false;

				//Log::d(TAG, "Updating mapping...");
			}

			if (update) {
				std::vector<std::string> instruments_ports = uniquePorts(mapping.instruments);
				std::vector<std::string> names;
				std::set_intersection(instruments_ports.begin(), instruments_ports.end(),
					m->available_ports.begin(), m->available_ports.end(),
					std::back_inserter(names));

				update = names != m->in_names;
				m->active_mapping = mapping;

				if (update) {
					Log::d(TAG, "Setting up midi...");
					midiSetup(names);
					Log::d(TAG, "Setup done!");
				}
			}
		}
	}


	void Midi::midiSetup(std::vector<std::string> const& ports) {
		midiTeardown();

		for (auto const& port_name : ports) {
			auto in = std::make_shared<libremidi::midi_in>();
			int port = -1;

			{
				int count = in->get_port_count();
				for (int i = 0; i != count; i++) {
					std::string name = in->get_port_name(i);
					if (port_name == name)
						port = i;
				}
			}

			if (port == -1) {
				Log::e(TAG, sfmt("Unable to find port [%s]", port_name));
				continue;
			}

			in->open_port(port);

			in->set_error_callback([this, port](libremidi::midi_error type, std::string_view errorText) {
				Log::e(TAG, sfmt("Port [%d] error [%d/%s]", port, int(type), errorText));
				});

			in->set_callback([this, port_name](const libremidi::message& message) {
				MidiMessage data;
				data.port = port_name;
				data.timestamp = message.timestamp;
				data.bytes = message.bytes;
				data.parameter = ParameterNone;
				midiReceived(data);
				});

			m->in.push_back(in);
			m->in_names.push_back(port_name);
		}
	}

	void Midi::midiReceived(MidiMessage& message) {

		if (message.bytes.empty())
			return;

		uint8_t cmd = message.bytes[0];
		uint8_t type = cmd & 0xf0;
		int channel = int(cmd & 0x0f);

		int cc = -1000;
		float value = 0.0f;

		if (message.bytes.size() >= 3) {
			if (type == 0xb0) {
				// controller
				cc = message.bytes[1];
				value = float(clampTo(int(message.bytes[2]), 0, 127) / 127.0f);
			}
			else if (cmd == 0xe0) {
				// pitch bend
				cc = -1;
				int v = message.bytes[1] | (message.bytes[2] << 7);
				value = clampTo(float(v - 8192) / float(8192), -1.0f, 1.0f);
			}
		}

		//Log::d(TAG, sfmt("[%s] cmd=0x%02X channel=0x%X", message.port, cmd, channel));
		for (int i = InstrumentStart; i != int(m->active_mapping.instruments.size()); ++i) {
			if (m->active_mapping.filter_active_instrument && i != m->active_instrument)
				continue;

			bool same_port = (m->active_mapping.instruments[i].port == message.port);
			bool same_channel = (m->active_mapping.instruments[i].channel == -1) ||
				(m->active_mapping.instruments[i].channel == channel);

			if (same_port && same_channel) {
				message.instrument = i;

				if (cc >= 0) {
					auto found = m->active_mapping.instruments[i].cc.find(cc);
					if (found != m->active_mapping.instruments[i].cc.end()) {
						message.parameter = found->second;
						message.parameter_value = value;
					}
				}
				else if (cc == -1) {
					// pitch bend
					message.parameter = ParameterPitchBend;
					message.parameter_value = value;
				}

				{
					std::unique_lock<std::mutex> lock(m->mutex);
					m->messages.push_back(message);
				}
			}
		}
	}

	void Midi::midiTeardown() {
		m->in.clear();
		m->in_names.clear();
	}

	void Midi::midiEnumerate() {
		std::vector<std::string> ports;
		std::vector<std::string> old_ports;

		{
			std::unique_lock<std::mutex> lock(m->mutex);
			old_ports = m->available_ports;
		}

		int count = m->midi_in_helper->get_port_count();

		for (int i = 0; i != count; i++) {
			std::string name = m->midi_in_helper->get_port_name(i);
			ports.push_back(name);
		}

		if (ports != old_ports) {
			Log::i(TAG, sfmt("Midi ports changed (%d):", ports.size()));
			for (size_t i = 0; i != ports.size(); ++i)
				Log::i(TAG, sfmt("%d [%s]", i, ports[i]));

			m->needs_mapping_update = true;
		}

		{
			std::unique_lock<std::mutex> lock(m->mutex);
			std::swap(m->available_ports, ports);
		}
	}

	void Midi::midiDumpInfo() {
		// Create an api map.
		std::map<libremidi::API, std::string> api_map{
			{libremidi::API::MACOSX_CORE, "OS-X CoreMidi"},
			{libremidi::API::LINUX_ALSA, "Linux ALSA"},
			{libremidi::API::LINUX_ALSA_SEQ, "Linux ALSA (sequencer)"},
			{libremidi::API::LINUX_ALSA_RAW, "Linux ALSA (raw)"},
			{libremidi::API::UNIX_JACK, "Jack Client"},
			{libremidi::API::WINDOWS_MM, "Windows MultiMedia"},
			{libremidi::API::WINDOWS_UWP, "Windows WinRT"},
			{libremidi::API::EMSCRIPTEN_WEBMIDI, "Web MIDI Emscripten"},
			{libremidi::API::DUMMY, "Dummy (no driver)"},
		};

		auto apis = libremidi::available_apis();
		Log::i(TAG, "Midi compiled APIs:");

		for (auto& api : libremidi::available_apis()) {
			Log::i(TAG, sfmt("> %s", api_map[api]));
		}
	}



}