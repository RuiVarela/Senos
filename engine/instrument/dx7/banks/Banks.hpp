#pragma once

#include <string>
#include <vector>
#include <map>

namespace dx7 {

	class Banks {
	public:

		struct BankInfo {
			std::string group;
			std::string bank;
			int size = 0; 
			const uint8_t* data = 0;

			std::vector<std::string> patches;
		};

		static void generateFilesFromFolder(std::string folder);

		static Banks& instance();

		std::vector<std::string> const& groups() const;
		std::vector<std::string> banks(int group_index) const;
		std::vector<std::string> patches(int group_index, int bank_index) const;
		BankInfo bank(int group_index, int bank_index) const;
	private:
		Banks();
		~Banks();

 		std::vector<Banks::BankInfo> m_banks;
		std::vector<std::string> m_groups;
		std::map<std::string, std::vector<std::string>> m_group_bank_names;
		std::map<std::string, int> m_bank_map;
		
		void buildRecords();
		void buildDatabase();

		//
		// Factory
		//
		static const uint8_t FactoryKeyboardPluckedRom1bData[];
		static const uint8_t FactoryKeyboardPluckedRom3bData[];
		static const uint8_t FactoryMasterEuJpRom1aData[];
		static const uint8_t FactoryMasterUsRom3aData[];
		static const uint8_t FactoryOrchestralPercussiveRom2aData[];
		static const uint8_t FactoryOrchestralPercussiveRom4aData[];
		static const uint8_t FactorySynthComplexEffectsRom2bData[];
		static const uint8_t FactorySynthComplexEffectsRom4bData[];

		//
		// VRC
		//
		static const uint8_t VrcBoTomlynSelectionIiVrc111aData[];
		static const uint8_t VrcBoTomlynSelectionIiVrc111bData[];
		static const uint8_t VrcBoTomlynSelectionVrc110aData[];
		static const uint8_t VrcBoTomlynSelectionVrc110bData[];
		static const uint8_t VrcDavidBristowSelectionVrc107aData[];
		static const uint8_t VrcDavidBristowSelectionVrc107bData[];
		static const uint8_t VrcGaryLeuenbergerSelectionVrc108aData[];
		static const uint8_t VrcGaryLeuenbergerSelectionVrc108bData[];
		static const uint8_t VrcKeyboardPluckedTunedPercVrc101aData[];
		static const uint8_t VrcKeyboardPluckedTunedPercVrc101bData[];
		static const uint8_t VrcLive64AkiraInoueVrc112aData[];
		static const uint8_t VrcLive64AkiraInoueVrc112bData[];
		static const uint8_t VrcPercussionVrc104aData[];
		static const uint8_t VrcPercussionVrc104bData[];
		static const uint8_t VrcSoundEffectVrc105aData[];
		static const uint8_t VrcSoundEffectVrc105bData[];
		static const uint8_t VrcStudio64Vrc109aData[];
		static const uint8_t VrcStudio64Vrc109bData[];
		static const uint8_t VrcSustainVrc103aData[];
		static const uint8_t VrcSustainVrc103bData[];
		static const uint8_t VrcSynthesizerVrc106aData[];
		static const uint8_t VrcSynthesizerVrc106bData[];
		static const uint8_t VrcWindInstrumentVrc102aData[];
		static const uint8_t VrcWindInstrumentVrc102bData[];

		//
		// E Grey
		//
		static const uint8_t EGreyMatterDisk2Data[];
		static const uint8_t EGreyMatterDisk5Data[];
		static const uint8_t EGreyMatterDisk7Data[];

		//
		// Finetales
		//
		static const uint8_t FinetalesEpsAndBellsData[];
		static const uint8_t FinetalesPlucksBassesBrassData[];
		static const uint8_t FinetalesStringsPadsEtcData[];

	};

}