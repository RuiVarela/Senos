#include "Banks.hpp"
#include "../../../core/Lang.hpp"
#include "../../../core/Text.hpp"
#include "../../../core/Log.hpp"
#include "../patch.h"

#include <set>

using namespace sns;

namespace dx7 {

    static void saveFile(std::string const& base, std::string const& source, std::string const& output) {
        std::string name = source.substr(base.size() + 1);
        replace(name, ".syx", "");
        replace(name, " - ", " ");

        std::vector<std::string> splitted;
        if (containsText(name, "\\")) 
            splitted = split(name, "\\");
        else if (containsText(name, "/")) 
            splitted = split(name, "/");
        
        if (splitted.size() != 2) 
            return;
        
        std::string group = splitted[0];
        std::string bank = splitted[1];
        replace(bank, ".", " ");
        replace(bank, "_", " ");

        replace(name, "#", "");
        replace(name, "!", "");
        replace(name, "(", " ");
        replace(name, ")", " ");
        replace(name, ",", " ");
        replace(name, ".", " ");
        replace(name, "&", "");
        replace(name, "\\", " ");
        replace(name, ".", " ");
        replace(name, "  ", " ");
        replace(name, "  ", " ");
        replace(name, "  ", " ");

        camelcase(name, true);

		std::vector<uint8_t> bytes;
		readRawBinary(source, bytes);

        std::stringstream header;
		{
            header <<  "   static const uint8_t " << name << "Data[];" << std::endl;
		}

        std::stringstream order;
        {
            
            order <<
                "   {" << std::endl <<
                "       BankInfo record;" << std::endl <<
                "       record.group = \"" << group << "\";" << std::endl <<
                "       record.bank = \"" << bank << "\";" << std::endl <<
                "       record.size = " << bytes.size() << ";" << std::endl <<
                "       record.data = " << name << "Data;" << std::endl <<
                "       output.push_back(record);" << std::endl <<
                "   }" << std::endl << std::endl;
        }

		std::stringstream data;
        {
            data << 
                "   //const std::string Banks::" << name << "Group = \"" << group << "\";" << std::endl <<
                "   //const std::string Banks::" << name << "Bank = \"" << bank << "\";" << std::endl <<
                "   //const int Banks::" << name << "Size = " << bytes.size() << ";" << std::endl <<
                "   const uint8_t Banks::" << name << "Data[] = { ";

            int counter = 0;
            for (uint8_t current : bytes) {
                if (counter % 25 == 0)
                    data << std::endl << "        ";

                //char hex_string[10];
                //sprintf_s(hex_string, "0x%02X", current);

                data << sfmt("0x%02X", current);

                if (counter != bytes.size() - 1)
                    data << ", ";
                counter++;
            }

            data << "   };" << std::endl << std::endl;
        }

        // data
        {
            std::string output_filename = mergePaths(output, group) + ".cpp";
            std::string generated;
            {
                std::string file_text;
                readRawText(output_filename, file_text);
                generated += file_text;
            }

            generated += data.str();
            writeRawText(output_filename, generated);
        }

        // header
        {
            std::string output_filename = mergePaths(output, group) + "Header.cpp";
            std::string generated;
            {
                std::string file_text;
                readRawText(output_filename, file_text);
                generated += file_text;
            }

            generated += header.str();
            writeRawText(output_filename, generated);
        }

        // header
        {
            std::string output_filename = mergePaths(output, group) + "Order.cpp";
            std::string generated;
            {
                std::string file_text;
                readRawText(output_filename, file_text);
                generated += file_text;
            }

            generated += order.str();
            writeRawText(output_filename, generated);
        }
	}

    static void crawlFolder(std::string const& base, std::string const& source, std::string const& output) {
        auto contents = getDirectoryContents(source);
        for (auto& current : contents) {
            auto path = mergePaths(source, current);
            if (fileType(path) == FileType::FileDirectory) {
                crawlFolder(base, path, output);
            } else if (getFileExtension(path) == "syx") {
                saveFile(base, path, output);
            } 
        }
    }

    void Banks::generateFilesFromFolder(std::string folder) {
        crawlFolder(folder, folder, folder);
    }

    Banks::Banks() {
        buildRecords();
        buildDatabase();
    }
	
    Banks::~Banks() {

    }

    Banks& Banks::instance() {
        static Banks banks;
        return banks;
    }

    std::vector<std::string> const& Banks::groups() const {
        return m_groups;
    }

	std::vector<std::string> Banks::banks(int group_index) const {
        if (group_index >= 0 && group_index < m_groups.size()) {
            auto found = m_group_bank_names.find(m_groups[group_index]);
            if (found != m_group_bank_names.end()) 
                return found->second;
        }
       return std::vector<std::string>();
    }

    std::vector<std::string> Banks::patches(int group_index, int bank_index) const {
        return bank(group_index, bank_index).patches;
    }

	Banks::BankInfo Banks::bank(int group_index, int bank_index) const {
        std::string key = sfmt("%d.%d", group_index, bank_index);

        auto found = m_bank_map.find(key);
        if (found != m_bank_map.end()) 
            return m_banks[found->second];

        return Banks::BankInfo();
    }

    void Banks::buildDatabase() {
        int total_count = 0;

        // get patches names
        char unpacked_patch[156];
        char name[11];

        for (auto& bank : m_banks) {
            std::set<std::string> used;

            for (int p = 0; p != 32; ++p) {
                const uint8_t *patch = bank.data + 6 + 128 * p;

                UnpackPatch((const char *)patch, unpacked_patch);
			    memcpy(name, unpacked_patch + 145, 10);
			    name[10] = 0;
                
                std::string as_string(name);
                replace(as_string, "\\", "");
                replace(as_string, "%", "");
                replace(as_string, "...", "");
                trim(as_string);

                if (used.find(as_string) != used.end()) {
                    as_string += "!";
                }

                used.insert(as_string);

                bank.patches.push_back(as_string);

                total_count++;
            }
        }


        // generate maps
        for (int b = 0; b != int(m_banks.size()); ++b) {
            auto& current = m_banks[b];

            if (!contains(m_groups, current.group)) {
                m_groups.push_back(current.group);
                m_group_bank_names[current.group] = std::vector<std::string>();
            }
            
            int group_index = -1;
            for (int i = 0; i != int(m_groups.size()); ++i) 
                if (m_groups[i] == current.group) 
                    group_index = i;
            
            
            int bank_index = int(m_group_bank_names[current.group].size());
            m_group_bank_names[current.group].push_back(current.bank);

            m_bank_map[sfmt("%d.%d", group_index, bank_index)] = b;
        }

        Log::d("Dx7", sfmt("Loaded %d patches on %d banks", total_count, m_banks.size()));
    }

    void Banks::buildRecords() {
        std::vector<Banks::BankInfo> output;

        //
        // Factory
        //
        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Master (EU JP) rom1a";
            record.size = 4104;
            record.data = FactoryMasterEuJpRom1aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Keyboard & Plucked rom1b";
            record.size = 4104;
            record.data = FactoryKeyboardPluckedRom1bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Orchestral & Percussive rom2a";
            record.size = 4104;
            record.data = FactoryOrchestralPercussiveRom2aData;
            output.push_back(record);
        }
        
        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Synth, Complex & Effects rom2b";
            record.size = 4104;
            record.data = FactorySynthComplexEffectsRom2bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Master (US) rom3a";
            record.size = 4104;
            record.data = FactoryMasterUsRom3aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Keyboard & Plucked rom3b";
            record.size = 4104;
            record.data = FactoryKeyboardPluckedRom3bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Orchestral & Percussive rom4a";
            record.size = 4104;
            record.data = FactoryOrchestralPercussiveRom4aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Factory";
            record.bank = "Synth, Complex & Effects rom4b";
            record.size = 4104;
            record.data = FactorySynthComplexEffectsRom4bData;
            output.push_back(record);
        }

        //
        // VRC
        //

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Keyboard, Plucked & Tuned Perc vrc101a";
            record.size = 4104;
            record.data = VrcKeyboardPluckedTunedPercVrc101aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Keyboard, Plucked & Tuned Perc vrc101b";
            record.size = 4104;
            record.data = VrcKeyboardPluckedTunedPercVrc101bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Wind Instrument vrc102a";
            record.size = 4104;
            record.data = VrcWindInstrumentVrc102aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Wind Instrument vrc102b";
            record.size = 4104;
            record.data = VrcWindInstrumentVrc102bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Sustain vrc103a";
            record.size = 4104;
            record.data = VrcSustainVrc103aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Sustain vrc103b";
            record.size = 4104;
            record.data = VrcSustainVrc103bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Percussion vrc104a";
            record.size = 4104;
            record.data = VrcPercussionVrc104aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Percussion vrc104b";
            record.size = 4104;
            record.data = VrcPercussionVrc104bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Sound Effect vrc105a";
            record.size = 4104;
            record.data = VrcSoundEffectVrc105aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Sound Effect vrc105b";
            record.size = 4104;
            record.data = VrcSoundEffectVrc105bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Synthesizer vrc106a";
            record.size = 4104;
            record.data = VrcSynthesizerVrc106aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Synthesizer vrc106b";
            record.size = 4104;
            record.data = VrcSynthesizerVrc106bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "David Bristow Selection vrc107a";
            record.size = 4104;
            record.data = VrcDavidBristowSelectionVrc107aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "David Bristow Selection vrc107b";
            record.size = 4104;
            record.data = VrcDavidBristowSelectionVrc107bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Gary Leuenberger Selection vrc108a";
            record.size = 4104;
            record.data = VrcGaryLeuenbergerSelectionVrc108aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Gary Leuenberger Selection vrc108b";
            record.size = 4104;
            record.data = VrcGaryLeuenbergerSelectionVrc108bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Studio 64 vrc109a";
            record.size = 4104;
            record.data = VrcStudio64Vrc109aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Studio 64 vrc109b";
            record.size = 4104;
            record.data = VrcStudio64Vrc109bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Bo Tomlyn Selection vrc110a";
            record.size = 4104;
            record.data = VrcBoTomlynSelectionVrc110aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Bo Tomlyn Selection vrc110b";
            record.size = 4104;
            record.data = VrcBoTomlynSelectionVrc110bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Bo Tomlyn Selection II vrc111a";
            record.size = 4104;
            record.data = VrcBoTomlynSelectionIiVrc111aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Bo Tomlyn Selection II vrc111b";
            record.size = 4104;
            record.data = VrcBoTomlynSelectionIiVrc111bData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Live 64 Akira Inoue vrc112a";
            record.size = 4104;
            record.data = VrcLive64AkiraInoueVrc112aData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "VRC";
            record.bank = "Live 64 Akira Inoue vrc112b";
            record.size = 4104;
            record.data = VrcLive64AkiraInoueVrc112bData;
            output.push_back(record);
        }

        //
        // Grey
        //
        {
            BankInfo record;
            record.group = "E! Grey Matter";
            record.bank = "Disk #2";
            record.size = 4104;
            record.data = EGreyMatterDisk2Data;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "E! Grey Matter";
            record.bank = "Disk #5";
            record.size = 4104;
            record.data = EGreyMatterDisk5Data;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "E! Grey Matter";
            record.bank = "Disk #7";
            record.size = 4104;
            record.data = EGreyMatterDisk7Data;
            output.push_back(record);
        }

        //
        // Finetales
        //
        {
            BankInfo record;
            record.group = "Finetales";
            record.bank = "EPs and Bells";
            record.size = 4104;
            record.data = FinetalesEpsAndBellsData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Finetales";
            record.bank = "Plucks Basses Brass";
            record.size = 4104;
            record.data = FinetalesPlucksBassesBrassData;
            output.push_back(record);
        }

        {
            BankInfo record;
            record.group = "Finetales";
            record.bank = "Strings Pads etc";
            record.size = 4104;
            record.data = FinetalesStringsPadsEtcData;
            output.push_back(record);
        }


        
        m_banks = output;
    }
}
