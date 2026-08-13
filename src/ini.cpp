#include "ini.hpp"
#include "utility.h"

namespace ini {

 void IniParser()
{
	std::string path = "Data\\SKSE\\Plugins\\ReLight.ini";

	if (!std::filesystem::exists(path)) {
		logger::warn("INI file not found: {}", path);
		return;
	}

	std::ifstream iniFile(path);
	if (!iniFile.is_open()) {
		logger::warn("Failed to open INI file: {}", path);
		return;
	}

	std::string line;

	enum Section
	{
		NONE,
		lightEdid,
		lightEdidDisable,
		meshPathExact,
		meshPathPartial,
		priority,
		refid,
		baseid
	} section = NONE;

	while (std::getline(iniFile, line))
	{
		line = trim(line);
		if (line.empty())
			continue;

		if (line.starts_with(";"))
		{

			if (line.find("exclude lights from being disabled") != std::string::npos)
				section = lightEdid;
			else if (line.find("disable lights by editor id") != std::string::npos)
				section = lightEdidDisable;
			else if (line.find("exclude specific mesh paths") != std::string::npos)
				section = meshPathExact;
			else if (line.find("exclude partial mesh paths") != std::string::npos)
				section = meshPathPartial;
			else if (line.find("priority") != std::string::npos)
				section = priority;
			else if (line.find("exclude refs") != std::string::npos)
				section = refid;
			else if (line.find("exclude base") != std::string::npos)
				section = baseid;
			//else
			//	section = NONE;

			continue;
		}

		switch (section)
		{
		case lightEdid:
			toLower(line);
			globals::enableByEditorID.push_back(line);
			logger::info("Added light editorID Exclusion: {}", line);
			continue;

		case lightEdidDisable:
			toLower(line);
			globals::disableByEditorID.push_back(line);
			logger::info("Added light editorID To Disable: {}", line);
			continue;

		case meshPathExact:
			toLower(line);
			globals::meshPathExclusionList.push_back(line);
			logger::info("Added exact mesh path exclude: {}", line);
			continue;

		case meshPathPartial:
			toLower(line);
			globals::meshPathExclusionListPartialMatch.push_back(line);
			logger::info("Added partial mesh path exclude: {}", line);
			continue;


		case priority:

			if (!line.starts_with("0") && !line.starts_with("[")) {
				toLower(line);
				globals::priorityList.push_back(line);
				logger::info("Added priority node: {}", line);
				continue;
			}


		case refid:
		{
			auto tildePos = line.find('~');
			if (tildePos == std::string::npos) {
				logger::warn("Excluded ref entry missing ~modname, skipping: {}", line);
				continue;
			}

			std::string formIDStr = trim(line.substr(0, tildePos));
			std::string modName = trim(line.substr(tildePos + 1));

			try {
				if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
					formIDStr = formIDStr.substr(2);
				}

				RE::FormID parsedID = std::stoul(formIDStr, nullptr, 16);

				auto dataHandler = RE::TESDataHandler::GetSingleton();
				auto mod = dataHandler ? dataHandler->LookupModByName(modName) : nullptr;

				if (mod && mod->IsLight()) {
					auto ref = dataHandler->LookupForm<RE::TESObjectREFR>(parsedID, modName);
					if (!ref) {
						logger::warn(
							"Failed to resolve light plugin ref localID 0x{:X} from mod {}",
							static_cast<std::uint32_t>(parsedID), modName);
						continue;
					}
					globals::excludedRefFormIDs.insert(ref->GetFormID());
					logger::info(
						"Added excluded light plugin ref runtime formID: 0x{:08X} from {} (local: 0x{:X})",
						static_cast<std::uint32_t>(ref->GetFormID()), modName,
						static_cast<std::uint32_t>(parsedID));
				}
				else {
					parsedID &= 0x00FFFFFF;
					globals::excludedRefFormIDs.insert(parsedID);
					logger::info(
						"Added excluded non-light runtime ref formID: 0x{:08X} from {}",
						static_cast<std::uint32_t>(parsedID), modName);
				}
			}
			catch (...) {
				logger::warn("Failed to parse excluded ref entry: {}", line);
			}

			continue;
		}
		case baseid:
		{
			auto tildePos = line.find('~');
			if (tildePos == std::string::npos) {
				logger::warn("Excluded base entry missing ~modname, skipping: {}", line);
				continue;
			}

			std::string formIDStr = trim(line.substr(0, tildePos));
			std::string modName = trim(line.substr(tildePos + 1));

			try {
				if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
					formIDStr = formIDStr.substr(2);
				}

				RE::FormID parsedID = std::stoul(formIDStr, nullptr, 16);

				auto dataHandler = RE::TESDataHandler::GetSingleton();
				auto mod = dataHandler ? dataHandler->LookupModByName(modName) : nullptr;

				if (mod && mod->IsLight()) {
					auto baseObj = dataHandler->LookupForm<RE::TESBoundObject>(parsedID, modName);
					if (!baseObj) {
						logger::warn(
							"Failed to resolve light plugin base localID 0x{:X} from mod {}",
							static_cast<std::uint32_t>(parsedID), modName);
						continue;
					}
					globals::excludedBaseFormIDs.insert(baseObj->GetFormID());
					logger::info(
						"Added excluded light plugin base runtime formID: 0x{:08X} from {} (local: 0x{:X})",
						static_cast<std::uint32_t>(baseObj->GetFormID()), modName,
						static_cast<std::uint32_t>(parsedID));
				}
				else {
					parsedID &= 0x00FFFFFF;
					globals::excludedBaseFormIDs.insert(parsedID);
					logger::info(
						"Added excluded non-light base formID: 0x{:08X} from {}",
						static_cast<std::uint32_t>(parsedID), modName);
				}
			}
			catch (...) {
				logger::warn("Failed to parse excluded base entry: {}", line);
			}

			continue;
		}
		default:
			break;
		}

		auto eq = line.find('=');
		if (eq == std::string::npos)
			continue;

		std::string key = trim(line.substr(0, eq));
		toLower(key);

		std::string value = trim(line.substr(eq + 1));
		toLower(value);

		if (key == "disablegamelights") {
			globals::disableGameLights = value == "true";
			logger::info("disableGameLights = {}", globals::disableGameLights);
			continue;
		}

		if (key == "lightbrightnessmultiplier") {
			globals::brightnessModifier = std::stof(value);
			logger::info("light brightness multiplier = {}", globals::brightnessModifier);
			continue;
		}

		if (key == "nonskselightsbrightnessmultiplier") {
			globals::vanillaBrightnessModifier = std::stof(value);
			logger::info("nonskse Light brightness multiplier = {}", globals::brightnessModifier);
			continue;
		}

		if (key == "removefakegloworbs") {
			globals::removeFakeGlowOrbs = value == "true";
			logger::info("removeFakeGlowOrbs = {}", globals::removeFakeGlowOrbs);
			continue;
		}

		if (key == "enablelightflickerprevention") {
			globals::enableLightFlickerPreventionMeasures = value == "true";
			logger::info("enableLightFlickerPreventionMeasures = {}", globals::enableLightFlickerPreventionMeasures);
			continue;
		}

		if (key == "enabledebugbulbs") {
			globals::enableDebugLightBulbs = value == "true";
			logger::info("enableDebugLightBulbs = {}", globals::enableDebugLightBulbs);
			continue;
		}

		if (key == "maxdistancefordrawdebuglines") {
			globals::distanceForDrawDebugLines = std::stoi(value);
			logger::info("max distance for draw debug lines = {}", globals::distanceForDrawDebugLines);
			continue;
		}

		if (key == "enabledebuglines") {
			globals::enableDebugLines = value == "true";
			logger::info("enableDebugLines = {}", globals::enableDebugLines);
			continue;
		}

		if (key == "allrelightsasisl") {
			globals::allRelightsAsISL = value == "true";
			logger::info("allRelightsAsISL = {}", globals::allRelightsAsISL);
			continue;
		}

		if (key == "whitelist") {
			splitString(value, ',', globals::whitelist);
			continue;
		}

		if (key == "logginglevel") {
			globals::loggingLevel = std::clamp(std::stoi(value), 0, 3);
			logger::info("Logging level set to {}", globals::loggingLevel);

			spdlog::level::level_enum lvl = spdlog::level::info;
			if (globals::loggingLevel == 0) lvl = spdlog::level::critical;
			else if (globals::loggingLevel == 1) lvl = spdlog::level::warn;
			else if (globals::loggingLevel == 3) lvl = spdlog::level::debug;

			spdlog::set_level(lvl);
			spdlog::flush_on(lvl);
			continue;
		}

		// light merge

		if (key == "light merge distance") {
			globals::lightMergeDistance = std::stof(value);
			continue;
		}

		if (key == "shadow light merge distance") {
			globals::shadowLightMergeDistance = std::stof(value);
			continue;
		}

		if (key == "light merge distance increased") {
			globals::lightMergeSeekingDistance = std::stof(value);
			continue;
		}

		if (key == "max z diff to merge") {
			globals::fMaxZDiffToMerge = std::stof(value);
			continue;
		}

		if (key == "max z diff to merge increased") {
			globals::fMaxZDiffToMergeIncreased = std::stof(value);
			continue;
		}

		if (key == "light fade increase per merge") {
			globals::lightFadePerMerge = std::stof(value);
			continue;
		}

		if (key == "light radius increase per merge") {
			globals::lightRadiusPerMerge = std::stof(value);
			continue;
		}

		if (key == "light fade max") {
			globals::lightFadeMax = std::stof(value);
			continue;
		}

		if (key == "light radius max") {
			globals::lightRadiusMax = std::stof(value);
			continue;
		}

		if (key == "light merge max lights") {
			globals::lightMergeMaxLights = std::stoi(value);
			continue;
		}

		if (key == "enablelightmerging") {
			globals::enableLightMerging = value == "true";
			logger::info("enable Light Merging = {}", globals::enableLightMerging);
			continue;
		}

		if (key == "enableshadowlightmerging") {
			globals::enableShadowLightMerging = value == "true";
			logger::info("enable shadow Light Merging = {}", globals::enableShadowLightMerging);
			continue;
		}
		// light flicker prevention 

		if (key == "large surface size") {
			globals::largeSurfaceSize = std::stoi(value);
			continue;
		}

		if (key == "medium surface size") {
			globals::mediumSurfaceSize = std::stoi(value);
			continue;
		}

		if (key == "small surface size") {
			globals::smallSurfaceSize = std::stoi(value);
			continue;
		}

		if (key == "max candles per sm surface") {
			globals::maxCandlesPerSurfaceSM = std::clamp(std::stoi(value), 0, 7);
			continue;
		}

		if (key == "max chandeliers per sm surface") {
			globals::maxChandeliersPerSurfaceSM = std::clamp(std::stoi(value), 0, 7);
			continue;
		}

		if (key == "max fires per sm surface") {
			globals::maxFiresPerSurfaceSM = std::clamp(std::stoi(value), 0, 7);
			continue;
		}

		if (key == "max candles per m surface") {
			globals::maxCandlesPerSurfaceM = std::clamp(std::stoi(value), 0, 7);
			continue;
		}

		if (key == "max chandeliers per m surface") {
			globals::maxChandeliersPerSurfaceM = std::clamp(std::stoi(value), 0, 7);
			continue;
		}

		if (key == "max fires per m surface") {
			globals::maxFiresPerSurfaceM = std::clamp(std::stoi(value), 0, 7);
			continue;
		}

		if (key == "max candle distance") {
			globals::maxCandleDistance = std::stof(value);
			continue;
		}

		if (key == "max candle z distance") {
			globals::maxCandleZDistance = std::stof(value);
			continue;
		}

		if (key == "max chandelier distance") {
			globals::maxChandelierDistance = std::stof(value);
			continue;
		}

		if (key == "max chandelier z distance") {
			globals::maxChandelierZDistance = std::stof(value);
			continue;
		}

	}

	iniFile.close();
	logger::info("ReLight.ini parsed successfully!");
}

 bool saveSettingsToIni()
{
	logger::info("Saving ReLight.ini...");
	const std::string path = "Data\\SKSE\\Plugins\\ReLight.ini";

	// READ: grab everything from the exclude refs section downward as a raw block
	std::string preservedBlock;
	{
		std::ifstream inFile(path);
		if (inFile.is_open())
		{
			std::string line;
			bool inSection = false;
			while (std::getline(inFile, line))
			{
				if (!inSection && line.find("; add esps by name") != std::string::npos)
					inSection = true;
				if (inSection)
					preservedBlock += line + "\n";
			}
		}
	}

	std::ofstream outFile(path, std::ios::trunc);
	if (!outFile.is_open())
	{
		logger::error("Failed to open {} for writing!", path);
		return false;
	}
	outFile << ";allow relight to disable all non relight lights(except those whos editor is in exclude section below)\n";
	outFile << "disableGameLights=" << (globals::disableGameLights ? "true" : "false") << "\n\n";
	outFile << "; enable light flicker prevention (Not for CS users)\n";
	outFile << "enableLightFlickerPrevention=" << (globals::enableLightFlickerPreventionMeasures ? "true" : "false") << "\n\n";
	outFile << ";Change brightness of all Relight lights\n";
	outFile << "lightBrightnessMultiplier=" << std::clamp(globals::brightnessModifier, 0.1f, 2.0f) << "\n\n";
	outFile << ";Change brightness of all non SKSE lights\n";
	outFile << "nonSKSELightsBrightnessMultiplier=" << std::clamp(globals::vanillaBrightnessModifier, 0.1f, 2.0f) << "\n\n";
	outFile << "; remove fake glow orbs (default = true)\n";
	outFile << "removeFakeGlowOrbs=" << (globals::removeFakeGlowOrbs ? "true" : "false") << "\n\n";
	outFile << "; enable debug bulbs (default = false)\n";
	outFile << "enableDebugBulbs=" << (globals::enableDebugLightBulbs ? "true" : "false") << "\n\n";
	outFile << "; enable debug lines (default = true)\n";
	outFile << "enableDebugLines=" << (globals::enableDebugLines ? "true" : "false") << "\n\n";
	outFile << "; max distance for debug lines (default = false)\n";
	outFile << "maxDistanceForDrawDebuglines=" << globals::distanceForDrawDebugLines << "\n\n";
	outFile << "; Make all Relights have Inverse Squared Lighting regardless of ISL flag\n";
	outFile << "allRelightsAsISL=" << (globals::allRelightsAsISL ? "true" : "false") << "\n\n";
	outFile << "; Logging Level (0: critical, 1: warnings/errors, 2: info, 3: debug)\n";
	outFile << "loggingLevel=" << globals::loggingLevel << "\n";
	outFile << "\n; Light merge settings\n\n";
	outFile << "enableLightMerging=" << (globals::enableLightMerging ? "true" : "false") << "\n";
	outFile << "light merge distance=" << globals::lightMergeDistance << "\n";
	outFile << "enableShadowLightMerging=" << (globals::enableShadowLightMerging ? "true" : "false") << "\n";
	outFile << "shadow light merge distance=" << globals::shadowLightMergeDistance << "\n";
	outFile << "light merge distance increased=" << globals::lightMergeSeekingDistance << "\n";
	outFile << "max z diff to merge=" << globals::fMaxZDiffToMerge << "\n";
	outFile << "max z diff to merge increased=" << globals::fMaxZDiffToMergeIncreased << "\n";
	outFile << "light fade increase per merge=" << globals::lightFadePerMerge << "\n";
	outFile << "light radius increase per merge=" << globals::lightRadiusPerMerge << "\n";
	outFile << "light fade max=" << globals::lightFadeMax << "\n";
	outFile << "light radius max=" << globals::lightRadiusMax << "\n";
	outFile << "light merge max lights=" << globals::lightMergeMaxLights << "\n\n";
	outFile << "\n;Light FlickerPrevention Settings\n\n";
	outFile << ";trishapes with worldbound radius larger than this number will not participate in light flicker prevention\n";
	outFile << "large surface size=" << globals::largeSurfaceSize << "\n";
	outFile << ";trishapes with worldbound radius larger then this number will not participate in light type distance checks\n";
	outFile << "medium surface size=" << globals::mediumSurfaceSize << "\n";
	outFile << ";trishapes with worldbound radius larger then this number will not participate in max light types per surface checks\n";
	outFile << "small surface size=" << globals::smallSurfaceSize << "\n";
	outFile << "max candles per sm surface=" << globals::maxCandlesPerSurfaceSM << "\n";
	outFile << "max chandeliers per sm surface=" << globals::maxChandeliersPerSurfaceSM << "\n";
	outFile << "max fires per sm surface=" << globals::maxFiresPerSurfaceSM << "\n";
	outFile << "max candles per m surface=" << globals::maxCandlesPerSurfaceM << "\n";
	outFile << "max chandeliers per m surface=" << globals::maxChandeliersPerSurfaceM << "\n";
	outFile << "max fires per m surface=" << globals::maxFiresPerSurfaceM << "\n";
	outFile << "max candle distance=" << globals::maxCandleDistance << "\n";
	outFile << "max candle z distance=" << globals::maxCandleZDistance << "\n";
	outFile << "max chandelier distance=" << globals::maxChandelierDistance << "\n";
	outFile << "max chandelier z distance=" << globals::maxChandelierZDistance << "\n";

	// dump the entire preserved block back verbatim - comments, formids, everything
	if (!preservedBlock.empty())
		outFile << "\n" << preservedBlock;

	outFile.close();
	logger::info("ReLight.ini saved successfully!");
	return true;
}

 bool AppendMenuExcludedRefToINI(const std::string& iniPath, const std::string& refIDAndModName)
{
	if (iniPath.empty()) {
		logger::error("AppendMenuExcludedRefToINI: iniPath was empty");
		return false;
	}

	if (refIDAndModName.empty()) {
		logger::error("AppendMenuExcludedRefToINI: refIDAndModName was empty");
		return false;
	}

	try {
		std::ifstream inFile(iniPath);
		if (!inFile.is_open()) {
			logger::error("AppendMenuExcludedRefToINI: failed to open {}", iniPath);
			return false;
		}

		std::vector<std::string> lines;
		std::string line;
		while (std::getline(inFile, line)) {
			lines.push_back(line);
		}
		inFile.close();

		const std::string targetSection = "[Refs Excluded Using in Game Menu]";
		const std::string legacyComment = "; exclude refs from receiving relights";

		bool inTargetSection = false;
		bool sectionFound = false;
		bool alreadyExists = false;
		std::size_t insertPos = lines.size();
		std::size_t legacyInsertPos = lines.size();
		bool legacyBlockFound = false;

		for (std::size_t i = 0; i < lines.size(); ++i) {
			std::string trimmed = trim(lines[i]);

			if (trimmed == targetSection) {
				inTargetSection = true;
				sectionFound = true;
				insertPos = i + 1;
				continue;
			}

			if (!sectionFound && trimmed == legacyComment) {
				legacyBlockFound = true;
				legacyInsertPos = i + 1;
				continue;
			}

			if (inTargetSection) {
				if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
					insertPos = i;
					break;
				}

				if (trimmed.empty() || trimmed.starts_with(";")) {
					continue;
				}

				if (trimmed == refIDAndModName) {
					alreadyExists = true;
					break;
				}

				insertPos = i + 1;
			}
			else if (legacyBlockFound) {
				if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
					legacyInsertPos = i;
					legacyBlockFound = false;
					continue;
				}

				if (!trimmed.empty() && !trimmed.starts_with(";")) {
					legacyInsertPos = i + 1;
				}
			}
		}

		if (alreadyExists) {
			logger::info("AppendMenuExcludedRefToINI: entry already exists: {}", refIDAndModName);
			return true;
		}

		if (sectionFound) {
			lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(insertPos), refIDAndModName);
		}
		else {
			std::size_t sectionPos = lines.size();

			if (legacyInsertPos < lines.size()) {
				sectionPos = legacyInsertPos;
			}
			else if (!lines.empty() && !lines.back().empty()) {
				lines.push_back("");
			}

			if (sectionPos != lines.size()) {
				lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionPos), "");
				++sectionPos;
				lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionPos), targetSection);
				++sectionPos;
				lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionPos), refIDAndModName);
			}
			else {
				lines.push_back(targetSection);
				lines.push_back(refIDAndModName);
			}
		}

		std::ofstream outFile(iniPath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("AppendMenuExcludedRefToINI: failed to write {}", iniPath);
			return false;
		}

		for (const auto& outLine : lines) {
			outFile << outLine << '\n';
		}

		logger::info("AppendMenuExcludedRefToINI: added {}", refIDAndModName);
		return true;
	}
	catch (const std::exception& e) {
		logger::error("AppendMenuExcludedRefToINI failed: {}", e.what());
		return false;
	}
}


 bool RemoveMenuExcludedRefFromINI(const std::string& iniPath, const std::string& refIDAndModName)
{
	if (iniPath.empty()) {
		logger::error("RemoveMenuExcludedRefFromINI: iniPath was empty");
		return false;
	}

	if (refIDAndModName.empty()) {
		logger::error("RemoveMenuExcludedRefFromINI: refIDAndModName was empty");
		return false;
	}

	try {
		std::ifstream inFile(iniPath);
		if (!inFile.is_open()) {
			logger::error("RemoveMenuExcludedRefFromINI: failed to open {}", iniPath);
			return false;
		}

		std::vector<std::string> lines;
		std::string line;
		while (std::getline(inFile, line)) {
			lines.push_back(line);
		}
		inFile.close();

		const std::string targetSection = "[Refs Excluded Using in Game Menu]";

		bool inTargetSection = false;
		bool sectionFound = false;
		bool removed = false;

		std::vector<std::string> output;
		output.reserve(lines.size());

		for (std::size_t i = 0; i < lines.size(); ++i) {
			std::string trimmed = trim(lines[i]);

			if (trimmed == targetSection) {
				inTargetSection = true;
				sectionFound = true;
				output.push_back(lines[i]);
				continue;
			}

			if (inTargetSection) {
				if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
					inTargetSection = false;
					output.push_back(lines[i]);
					continue;
				}

				if (trimmed == refIDAndModName) {
					removed = true;
					logger::info("RemoveMenuExcludedRefFromINI: removed {}", refIDAndModName);
					continue;
				}
			}

			output.push_back(lines[i]);
		}

		if (!sectionFound) {
			logger::info("RemoveMenuExcludedRefFromINI: section not found, nothing to remove");
			return true;
		}

		if (!removed) {
			logger::info("RemoveMenuExcludedRefFromINI: entry not present: {}", refIDAndModName);
			return true;
		}

		std::ofstream outFile(iniPath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("RemoveMenuExcludedRefFromINI: failed to write {}", iniPath);
			return false;
		}

		for (const auto& outLine : output) {
			outFile << outLine << '\n';
		}

		return true;
	}
	catch (const std::exception& e) {
		logger::error("RemoveMenuExcludedRefFromINI failed: {}", e.what());
		return false;
	}
}

  void RemoveFromIniExcludeRefID(RE::TESObjectREFR* ref, std::string& refIDandModName)
 {

	 if (refIDandModName.empty()) {
		 logger::warn("RemoveFromIniExcludeRefID: Failed to build refID string.");
		 return;
	 }

	 if (!RemoveMenuExcludedRefFromINI("Data/SKSE/Plugins/ReLight.ini", refIDandModName)) {
		 logger::info("No Ref {} Found in Ini Excludes to Remove", refIDandModName);
	 }

	 globals::excludedRefFormIDs.erase(ref->GetFormID());
 }

}