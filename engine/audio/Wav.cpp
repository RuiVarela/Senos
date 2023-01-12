#include "Wav.hpp"
#include <stdio.h>

// https://github.com/mhroth/tinywav/blob/master/tinywav.c

struct WavHeader {
	uint8_t ChunkID[4] = { 'R', 'I', 'F', 'F' };		// RIFF Header Magic header
	uint32_t ChunkSize = 0;
	uint8_t Format[4] = { 'W', 'A', 'V', 'E' };			// WAVE Header
	uint8_t Subchunk1ID[4] = { 'f', 'm', 't', ' ' };	// FMT header
	uint32_t Subchunk1Size = 0;
	uint16_t AudioFormat = 0;							//3 - float | 1 - pcm
	uint16_t NumChannels = 0;
	uint32_t SampleRate = 0;
	uint32_t ByteRate = 0;
	uint16_t BlockAlign = 0;
	uint16_t BitsPerSample = 0;
	uint8_t Subchunk2ID[4] = { 'd', 'a', 't', 'a' };	// "data"  string
	uint32_t Subchunk2Size = 0;
};
namespace sns {

	struct Wav::PrivateImplementation {
		FILE* file = nullptr;
		WavHeader header;
		uint32_t frames_on_file = 0;
	};

	Wav::Wav() 
		: m(std::make_shared<PrivateImplementation>()),
		m_is_ok(false)
	{
	}

	bool Wav::open(std::string const& filename)
	{
		close();

		m_filename = filename;
		m_is_ok = false;

		m->file = fopen(filename.c_str(), "wb");
		if (m->file)
		{

			m->frames_on_file = 0;
			const int channels = 1;

			const int sample_bytes = 4; // float
			WavHeader *header = &(m->header);

			// prepare WAV header
			header->ChunkSize = 0;							   // fill this in on file-close
			header->Subchunk1Size = 16;						   // PCM
			header->AudioFormat = (sample_bytes == 4) ? 3 : 1; // 1 PCM, 3 IEEE float
			header->NumChannels = channels;
			header->SampleRate = sns::SampleRate;
			header->ByteRate = sns::SampleRate * channels * sample_bytes;
			header->BlockAlign = channels * sample_bytes;
			header->BitsPerSample = 8 * sample_bytes;
			header->Subchunk2Size = 0; // fill this in on file-close

			int header_size = sizeof(WavHeader);
			assert(header_size == 44);

			// write WAV header
			fwrite(header, header_size, 1, m->file);

			m_is_ok = true;
		}

		return m_is_ok;
	}

	int Wav::write(float* data, int frames) {

		int samples = int(m->header.NumChannels) * frames;

		m->frames_on_file += frames;
		size_t written = fwrite(data, sizeof(float), samples, m->file);

		return (int)written / (int)m->header.NumChannels;
	}

	Wav::~Wav() {
		close();
	}

	void Wav::close(){

		if (m->file){

			uint32_t sample_size = m->header.BitsPerSample / 8;
			uint32_t channels = m->header.NumChannels;
			uint32_t data_len = m->frames_on_file * channels * sample_size;

			m->header.ChunkSize = 36 + data_len;
			m->header.Subchunk2Size = data_len;

			fseek(m->file, 0, SEEK_SET);
			fwrite(&m->header, sizeof(WavHeader), 1, m->file);

			fclose(m->file);
			m->file = nullptr;
		}

		m_filename = "";
		m_is_ok = false;
	}

	bool Wav::isOk() {
		return m_is_ok;
	}



	std::vector<float> Wav::load(std::string const& filename) {
		std::vector<float> data;

			//
			// Reading
			//
			FILE* file = fopen(filename.c_str(), "rb");
			if (file == nullptr) 
				return data;


			WavHeader header;

			size_t ret = fread(&header, sizeof(WavHeader), 1, file);

			bool ok = (ret > 0);

			ok &= header.ChunkID[0] == 'R';
			ok &= header.ChunkID[1] == 'I';
			ok &= header.ChunkID[2] == 'F';
			ok &= header.ChunkID[3] == 'F';

			ok &= header.Format[0] == 'W';
			ok &= header.Format[1] == 'A';
			ok &= header.Format[2] == 'V';
			ok &= header.Format[3] == 'E';

			ok &= header.Subchunk1ID[0] == 'f';
			ok &= header.Subchunk1ID[1] == 'm';
			ok &= header.Subchunk1ID[2] == 't';
			ok &= header.Subchunk1ID[3] == ' ';

			ok &= header.Subchunk2ID[0] == 'd';
			ok &= header.Subchunk2ID[1] == 'a';
			ok &= header.Subchunk2ID[2] == 't';
			ok &= header.Subchunk2ID[3] == 'a';

			uint32_t samples = 0;

			if (ok){
				uint32_t sample_bytes = header.BitsPerSample / 8;
				samples = header.Subchunk2Size / sample_bytes;

				ok = (samples > 0);


				// 32 bit float, mono
				ok &= (header.NumChannels == 1);
				ok &= (header.SampleRate == sns::SampleRate);
				ok &= (header.BitsPerSample == 32);
				ok &= (header.AudioFormat == 3); // float
			}

			if (ok) {
				data.resize(samples);
				fread(data.data(), sizeof(float), samples, file);
			}

			return data;
	}

}