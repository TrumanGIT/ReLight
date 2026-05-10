
#include "config.hpp"
#include "global.h"
#include "LightManager.h"
#include "utility.h"
#include <algorithm>

using json = nlohmann::json;

#define BOOL2JSON_READ(C, I) \
if (data.contains(#C)) { \
    config.C = data[#C].get<bool>(); \
} \
else { \
    config.C = I; \
} \

#define FLOAT2JSON_READ(C, I) \
if (data.contains(#C)) { \
    config.C = data[#C].get<float>(); \
} \
else { \
    config.C = I; \
} \

bool loadConfiguration(LightConfig& config, const json& data) {
	try {
		if (data.contains("menuName")) {
			config.menuName = data["menuName"].get<std::string>();
		}

		FOREACH_BOOL(BOOL2JSON_READ)
		FOREACH_FLOAT(FLOAT2JSON_READ)

		config.startingFade = config.brightness;

		// clamp radius
		config.radius = std::clamp(config.radius, 0.0f, 500.f);

		if (data.contains("color") && data["color"].is_array()) {
			auto& arr = data["color"];
			for (size_t i = 0; i < std::min(arr.size(), size_t(COL_SIZE)); ++i) {
				auto val = arr[i].get<int>();
				config.diffuseColor[i] = std::clamp(val, 0, 255);
			}
		}

		if (data.contains("position") && data["position"].is_array()) {
			auto& arr = data["position"];
			for (size_t i = 0; i < std::min(arr.size(), size_t(POS_SIZE)); ++i) {
				auto val = arr[i].get<float>();  // <- use float, not int
				config.position[i] = val;
			}
		}

		if (data.contains("flags")) {
			config.flags = ParseFlags(data["flags"]);
		}

		if (data.contains("attachPath") && data["attachPath"].is_array()) {
			config.attachPath.clear();
			for (const auto& v : data["attachPath"]) {
				if (!v.is_number_integer()) continue;
				int idx = v.get<int>();
				if (idx >= 0)
					config.attachPath.push_back(idx);
			}
		}

		return true;
	}
	catch (const json::exception& e) {
		logger::error("cannot read JSON object due to {}", e.what());
		return false;
	}
}

bool saveConfiguration(const LightConfig& config) {
	try {
		std::ifstream inFile(config.configPath);
		if (!inFile.is_open()) {
			logger::error("Failed to open config file for reading: {}", config.configPath);
			return false;
		}

		json data;
		inFile >> data;
		inFile.close();

		// handle multiple json objects in 1 config
		if (!data.is_array()) {
			data = json::array({ data });
		}

		json newEntry;

		json& originalEntry = data[config.jsonIndex];

		if (originalEntry.contains("meshPath")) {
			newEntry["meshPath"] = originalEntry["meshPath"];
		}
		else if (!config.meshPaths.empty()) {
			newEntry["meshPath"] = config.meshPaths;
		}
		else {
			newEntry["meshPath"] = json::array();
		}

		if (originalEntry.contains("refID")) {
			newEntry["refID"] = originalEntry["refID"];
		}
		else if (!config.refFormIDAndModName.empty()) {
			newEntry["refID"] = config.refFormIDAndModName;
		}

		newEntry["menuName"] = config.menuName;

#define JSON_WRITE(C, I) newEntry[#C] = config.C;

		FOREACH_BOOL(JSON_WRITE)
			// brighness useses in game slider copy value for a more stable value
		newEntry["brightness"] = truncateDecimals(config.startingFade, 2);
		// clamp radius as community shaders can send brighntess / radius values to infinity
		newEntry["radius"] = truncateDecimals(std::clamp(config.radius, 0.1f, 500.0f), 2);
		newEntry["fov"] = truncateDecimals(config.fov, 2);
		newEntry["falloff"] = truncateDecimals(config.falloff, 2);
		newEntry["nearDistance"] = truncateDecimals(config.nearDistance, 2);
		newEntry["depthBias"] = truncateDecimals(config.depthBias, 2);
		newEntry["ambientRatio"] = truncateDecimals(config.ambientRatio, 2);
		newEntry["flickerIntensity"] = truncateDecimals(config.flickerIntensity, 2);
		newEntry["flickersPerSecond"] = truncateDecimals(config.flickersPerSecond, 2);
		newEntry["flickerAmplitude"] = truncateDecimals(config.flickerAmplitude, 2);
		//clamp to 0.1f
		newEntry["cutoffOverride"] =
			std::max(0.01f, static_cast<float>(truncateDecimals(config.cutoffOverride, 2)));

		newEntry["size"] =
			std::max(0.01f, static_cast<float>(truncateDecimals(config.size, 2)));

		newEntry["color"] = { config.diffuseColor[0], config.diffuseColor[1], config.diffuseColor[2] };
		newEntry["position"] = { 
			truncateDecimals(config.position[0], 2),
			truncateDecimals(config.position[1], 2),
			truncateDecimals(config.position[2], 2)
		};

		newEntry["flags"] = FlagsToJson(config.flags);

		newEntry["attachPath"] = config.attachPath;

		// json index is the pos of json objects in a single json file.
		if (config.jsonIndex >= data.size()) {
			logger::warn("JSON index {} out of bounds, appending to end", config.jsonIndex);
			data.push_back(newEntry);
		}
		else {
			data[config.jsonIndex] = newEntry;
		}

		std::ofstream outFile(config.configPath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("Failed to open config file for writing: {}", config.configPath);
			return false;
		}

		outFile << data.dump(4);
		logger::info("Successfully saved light data to template at {} (index {})", config.configPath, config.jsonIndex);
		return true;
	}
	catch (const std::exception& e) {
		logger::error("Failed to write config {}: {}", config.configPath, e.what());
		return false;
	}
}

bool saveNewConfiguration(LightConfig& config)
{
	try {
		json newEntry;

		newEntry["meshPath"] = config.meshPaths;

		if (!config.refFormIDAndModName.empty()) {
			newEntry["refID"] = config.refFormIDAndModName;
		}

		newEntry["menuName"] = config.menuName;

#define JSON_WRITE(C, I) newEntry[#C] = config.C;
		FOREACH_BOOL(JSON_WRITE)

			newEntry["brightness"] = truncateDecimals(config.startingFade, 2);
		newEntry["radius"] = truncateDecimals(std::clamp(config.radius, 0.1f, 500.0f), 2);
		newEntry["fov"] = truncateDecimals(config.fov, 2);
		newEntry["falloff"] = truncateDecimals(config.falloff, 2);
		newEntry["nearDistance"] = truncateDecimals(config.nearDistance, 2);
		newEntry["depthBias"] = truncateDecimals(config.depthBias, 2);
		newEntry["ambientRatio"] = truncateDecimals(config.ambientRatio, 2);
		newEntry["flickerIntensity"] = truncateDecimals(config.flickerIntensity, 2);
		newEntry["flickersPerSecond"] = truncateDecimals(config.flickersPerSecond, 2);
		newEntry["flickerAmplitude"] = truncateDecimals(config.flickerAmplitude, 2);
		newEntry["size"] = truncateDecimals(std::max(0.01f, config.size), 2);
		newEntry["cutoffOverride"] = truncateDecimals(std::max(0.01f, config.cutoffOverride), 2);

		newEntry["color"] = {
			config.diffuseColor[0],
			config.diffuseColor[1],
			config.diffuseColor[2]
		};

		newEntry["position"] = {
			truncateDecimals(config.position[0], 2),
			truncateDecimals(config.position[1], 2),
			truncateDecimals(config.position[2], 2)
		};

		newEntry["flags"] = FlagsToJson(config.flags);
		newEntry["attachPath"] = config.attachPath;

		std::filesystem::path outPath(config.configPath);

		if (outPath.empty()) {
			logger::error("saveNewConfiguration: configPath was empty");
			return false;
		}

		std::filesystem::create_directories(outPath.parent_path());

		if (std::filesystem::exists(outPath)) {
			logger::warn("saveNewConfiguration: file already exists at {}", outPath.string());
			return false;
		}

		std::ofstream outFile(outPath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("Failed to open new config file for writing: {}", outPath.string());
			return false;
		}

		outFile << json::array({ newEntry }).dump(4);
		outFile.close();

		logger::info("Successfully created new light config at {}", outPath.string());
		return true;
	}
	catch (const std::exception& e) {
		logger::error("Failed to create new config {}: {}", config.configPath, e.what());
		return false;
	}
}

 bool AddMeshPathToAllEntries(const std::string& filePath, const std::string& meshPath)
{
	try {
		std::ifstream inFile(filePath);
		if (!inFile.is_open()) {
			logger::error("Failed to open config file for reading: {}", filePath);
			return false;
		}

		json data;
		inFile >> data;
		inFile.close();

		if (!data.is_array()) {
			data = json::array({ data });
		}

		bool changed = false;

		for (auto& entry : data) {
			if (!entry.is_object()) {
				continue;
			}

			if (!entry.contains("meshPath")) {
				entry["meshPath"] = meshPath;
				changed = true;
				continue;
			}

			auto& meshField = entry["meshPath"];

			if (meshField.is_string()) {
				std::string existing = meshField.get<std::string>();
				if (existing != meshPath) {
					meshField = json::array({ existing, meshPath });
					changed = true;
				}
			}
			else if (meshField.is_array()) {
				bool found = false;

				for (const auto& item : meshField) {
					if (item.is_string() && item.get<std::string>() == meshPath) {
						found = true;
						break;
					}
				}

				if (!found) {
					meshField.push_back(meshPath);
					changed = true;
				}
			}
			else {
				logger::warn("meshPath in {} was neither string nor array", filePath);
			}
		}

		if (!changed) {
			logger::info("meshPath '{}' already present in all entries of {}", meshPath, filePath);
			return true;
		}

		std::ofstream outFile(filePath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("Failed to open config file for writing: {}", filePath);
			return false;
		}

		outFile << data.dump(4);
		logger::info("Added meshPath '{}' to config file {}", meshPath, filePath);
		return true;
	}
	catch (const std::exception& e) {
		logger::error("Failed updating meshPath in {}: {}", filePath, e.what());
		return false;
	}
 }

inline uint32_t ParseFlags(const nlohmann::json& j)
{
	uint32_t mask = 0;

	auto setFlag = [&](const std::string& f)
		{
			for (const auto& [flag, name] : LightFlagNames) {
				if (f == name) {
					mask |= static_cast<uint32_t>(flag);
					break;
				}
			}
		};

	if (j.is_string()) {
		setFlag(j.get<std::string>());
	}
	else if (j.is_array()) {
		for (const auto& v : j) {
			if (!v.is_string()) continue;
			setFlag(v.get<std::string>());
		}
	}

	return mask;
}

inline nlohmann::json FlagsToJson(uint32_t mask) {
	nlohmann::json arr = nlohmann::json::array();

	for (const auto& [flag, name] : LightFlagNames) {
		if (mask & static_cast<uint32_t>(flag)) {
			arr.push_back(name);
		}
	}

	return arr;
}

// ini parser already filled the users desired, priority nodes, so these ones have no priority
// doesnet actually sort file path or node name atm.
void sortInPriorityList(const LightConfig& cfg)
{
	for (const auto& meshPath : cfg.meshPaths) {
		if (meshPath.empty()) {
			continue;
		}

		if (std::find(globals::priorityList.begin(),
			globals::priorityList.end(),
			meshPath) == globals::priorityList.end())
		{
			globals::priorityList.push_back(meshPath);
		}
	}
}

void parseTemplates() {
	logger::info("Parsing light templates..");
	std::vector<std::string> paths = GetConfigPaths();
	

	for (const auto& p : paths) {
		logger::info(" reading.. {}", p);

		std::ifstream configFile(p);
		if (!configFile.is_open()) {
			logger::error("Failed to open config file: {}", p);
			continue;
		}

		std::string raw((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
		json data = json::parse(raw, nullptr, true, true);

		json entries;

		if (data.is_array()) {
			entries = data;
		}
		else if (data.is_object()) {
			entries = json::array({ data });
		}
		else {
			logger::error("Invalid JSON root in {}", p);
			continue;
		}

		uint16_t jsonIndex = 0;

		for (json json : entries) {

			std::vector<std::string> refFormIDs;
			if (json.contains("refID")) {
				if (json["refID"].is_string()) {
					refFormIDs.push_back(json["refID"]);
				}
				else if (json["refID"].is_array()) {
					refFormIDs = json["refID"].get<std::vector<std::string>>();
				}
			}

			// create a config for each refID listed
			for (const auto& refID : refFormIDs) {
				if (refID.empty()) {
					continue;
				}

				LightConfig cfg;
				loadConfiguration(cfg, json);
				cfg.configPath = p;
				cfg.configID = globals::nextID++;
				cfg.jsonIndex = jsonIndex;
				cfg.refFormIDAndModName = refID;
				toLower(cfg.refFormIDAndModName);

				auto tildePos = cfg.refFormIDAndModName.find('~');
				if (tildePos == std::string::npos) {
					logger::warn("Invalid refID format '{}', expected FormID~ModName", refID);
					continue;
				}

				std::string formIDStr = trim(cfg.refFormIDAndModName.substr(0, tildePos));
				std::string modName = trim(cfg.refFormIDAndModName.substr(tildePos + 1));
				toLower(modName);

				try {
					if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
						formIDStr = formIDStr.substr(2);
					}

					std::uint32_t parsedID = std::stoul(formIDStr, nullptr, 16);

					auto* dataHandler = RE::TESDataHandler::GetSingleton();
					if (!dataHandler) {
						logger::warn("TESDataHandler was null while parsing refID '{}'", refID);
						continue;
					}

					const RE::TESFile* file = dataHandler->LookupLoadedModByName(modName);
					bool isLightMod = false;

					if (!file) {
						file = dataHandler->LookupLoadedLightModByName(modName);
						if (file) {
							isLightMod = true;
						}
					}

					if (!file) {
						logger::warn("Invalid mod name '{}' in refID '{}'", modName, refID);
						continue;
					}

					RE::FormID runtimeID = 0;

					if (isLightMod) {
						auto* ref = dataHandler->LookupForm<RE::TESObjectREFR>(parsedID, modName);
						if (!ref) {
							logger::warn("Failed to resolve light plugin refID '{}' in {}", refID, p);
							continue;
						}

						runtimeID = ref->GetFormID();
					}
					else {
						runtimeID = parsedID;
					}

					if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
						LightData::refFormIDToJsonCfgExteriors[runtimeID].push_back(cfg);
						logger::info("adding ref ID outdoor config 0x{:08X}", static_cast<std::uint32_t>(runtimeID));
						cfg.print(true);
					}
					else {
						LightData::refFormIDToJsonCfg[runtimeID].push_back(cfg);
						logger::info("adding ref ID config 0x{:08X}", static_cast<std::uint32_t>(runtimeID));
						cfg.print(false);
					}

					LightData::configIDToJsonCfg[cfg.configID] = cfg;
					LightData::defaultConfigs[cfg.configID] = cfg;
				}
				catch (...) {
					logger::warn("Failed to parse refID '{}' in {}", refID, p);
					continue;
				}
			}

			// create a config for each mesh file path
			std::vector<std::string> meshFilePaths;

			if (json.contains("meshPath")) {
				if (json["meshPath"].is_string()) {
					meshFilePaths.push_back(json["meshPath"].get<std::string>());
				}
				else if (json["meshPath"].is_array()) {
					meshFilePaths = json["meshPath"].get<std::vector<std::string>>();
				}
			}

			if (!meshFilePaths.empty()) {
				LightConfig cfg;
				loadConfiguration(cfg, json);

				cfg.configPath = p;
				cfg.configID = globals::nextID++;
				cfg.jsonIndex = jsonIndex;
				cfg.meshPaths = meshFilePaths;

				for (auto& meshPath : cfg.meshPaths) {
					toLower(meshPath);
				}

				sortInPriorityList(cfg);

				LightData::configIDToJsonCfg[cfg.configID] = cfg;
				LightData::defaultConfigs[cfg.configID] = cfg;

				for (const auto& meshPath : cfg.meshPaths) {
					if (meshPath.empty()) {
						continue;
					}

					if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
						LightData::meshPathToJsonCfgExteriors[meshPath].push_back(cfg);
					}
					else {
						LightData::meshPathToJsonCfg[meshPath].push_back(cfg);
					}
				}

				cfg.print(cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor));
			}

			//used to get the right index to save back too
			jsonIndex++;
		}
	}
}


std::vector<LightConfig>& findConfigsForMeshPath(std::string& meshPath, bool interior)
{
	static std::vector<LightConfig> empty;

	if (meshPath.empty()) {
		logger::error("meshPath was empty in find configs for mesh path");
		return empty;
	}

	toLower(meshPath);

	if (!interior) {
		auto it = LightData::meshPathToJsonCfgExteriors.find(meshPath);
		if (it != LightData::meshPathToJsonCfgExteriors.end()) {
			return it->second;
		}
	}

	auto fallbackIt = LightData::meshPathToJsonCfg.find(meshPath);
	if (fallbackIt != LightData::meshPathToJsonCfg.end()) {
		return fallbackIt->second;
	}

	logger::warn("found meshPath '{}' but no config exists", meshPath);
	return empty;
}

 std::size_t CountJsonEntriesInFile(const std::string& configPath)
{
	try {
		if (configPath.empty()) {
			logger::warn("CountJsonEntriesInFile: configPath was empty");
			return 0;
		}

		std::ifstream inFile(configPath);
		if (!inFile.is_open()) {
			logger::warn("CountJsonEntriesInFile: failed to open {}", configPath);
			return 0;
		}

		json data;
		inFile >> data;

		if (data.is_array()) {
			return data.size();
		}

		if (data.is_object()) {
			return 1;
		}

		logger::warn("CountJsonEntriesInFile: invalid JSON root in {}", configPath);
		return 0;
	}
	catch (const std::exception& e) {
		logger::error("CountJsonEntriesInFile failed for {}: {}", configPath, e.what());
		return 0;
	}
}

 bool AppendNewConfigEntryFromLight(
	 const std::string& configPath,
	 std::uint16_t jsonIndex,
	 const std::string& menuName,
	 RE::NiLight* niLight,
	 const std::string& refIDAndModName,
	 const std::string& matched,
	 const LightConfig& baseCfg,
	 bool refLight,
	 RE::FormID refFormID)
 {
	 try {
		 if (configPath.empty()) {
			 logger::error("AppendNewConfigEntryFromLight: configPath was empty");
			 return false;
		 }

		 if (!niLight) {
			 logger::error("AppendNewConfigEntryFromLight: niLight was null");
			 return false;
		 }

		 LightConfig cfg;
		 LightData::updateConfigFromLight(cfg, baseCfg, niLight);

		 cfg.configPath = configPath;
		 cfg.jsonIndex = jsonIndex;
		 cfg.configID = globals::nextID++;
		 cfg.menuName = menuName;
		 cfg.refFormIDAndModName = refIDAndModName;

		 cfg.meshPaths.clear();
		 if (!matched.empty()) {
			 cfg.meshPaths.push_back(matched);
		 }

		 std::ifstream inFile(configPath);
		 if (!inFile.is_open()) {
			 logger::error("AppendNewConfigEntryFromLight: failed to open {}", configPath);
			 return false;
		 }

		 json data;
		 inFile >> data;
		 inFile.close();

		 if (!data.is_array()) {
			 data = json::array({ data });
		 }

		 json newEntry;

		 if (!cfg.meshPaths.empty()) {
			 newEntry["meshPath"] = cfg.meshPaths;
		 }

		 if (!cfg.refFormIDAndModName.empty()) {
			 newEntry["refID"] = cfg.refFormIDAndModName;
			 cfg.flags |= static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging);
		 }

		 newEntry["menuName"] = cfg.menuName;

#define JSON_WRITE(C, I) newEntry[#C] = cfg.C;
		 FOREACH_BOOL(JSON_WRITE)

			 newEntry["brightness"] = truncateDecimals(cfg.startingFade, 2);
		 newEntry["radius"] = truncateDecimals(cfg.radius, 2);
		 newEntry["fov"] = truncateDecimals(cfg.fov, 2);
		 newEntry["falloff"] = truncateDecimals(cfg.falloff, 2);
		 newEntry["nearDistance"] = truncateDecimals(cfg.nearDistance, 2);
		 newEntry["depthBias"] = truncateDecimals(cfg.depthBias, 2);
		 newEntry["ambientRatio"] = truncateDecimals(cfg.ambientRatio, 2);
		 newEntry["flickerIntensity"] = truncateDecimals(cfg.flickerIntensity, 2);
		 newEntry["flickersPerSecond"] = truncateDecimals(cfg.flickersPerSecond, 2);
		 newEntry["flickerAmplitude"] = truncateDecimals(cfg.flickerAmplitude, 2);
		 newEntry["size"] = truncateDecimals(std::max(0.1f, cfg.size), 2);
		 newEntry["cutoffOverride"] = truncateDecimals(cfg.cutoffOverride, 2);

		 newEntry["color"] = {
			 cfg.diffuseColor[0],
			 cfg.diffuseColor[1],
			 cfg.diffuseColor[2]
		 };

		 newEntry["position"] = {
			 truncateDecimals(cfg.position[0], 2),
			 truncateDecimals(cfg.position[1], 2),
			 truncateDecimals(cfg.position[2], 2)
		 };

		 newEntry["flags"] = FlagsToJson(cfg.flags);
		 newEntry["attachPath"] = cfg.attachPath;

		 if (jsonIndex > data.size()) {
			 logger::warn(
				 "AppendNewConfigEntryFromLight: jsonIndex {} > data.size() {}, appending instead",
				 jsonIndex,
				 data.size());

			 data.push_back(newEntry);
			 cfg.jsonIndex = static_cast<std::uint16_t>(data.size() - 1);
		 }
		 else {
			 data.insert(data.begin() + static_cast<std::ptrdiff_t>(jsonIndex), newEntry);
		 }

		 std::ofstream outFile(configPath, std::ios::trunc);
		 if (!outFile.is_open()) {
			 logger::error("AppendNewConfigEntryFromLight: failed to write {}", configPath);
			 return false;
		 }

		 outFile << data.dump(4);

		 LightData::configIDToJsonCfg[cfg.configID] = cfg;
		 LightData::defaultConfigs[cfg.configID] = cfg;

		 if (refLight) {
			 if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
				 LightData::refFormIDToJsonCfgExteriors[refFormID].push_back(cfg);
			 }
			 else {
				 LightData::refFormIDToJsonCfg[refFormID].push_back(cfg);
			 }

			 logger::info(
				 "AppendNewConfigEntryFromLight: added ref config '{}' at {} index {}",
				 cfg.menuName,
				 cfg.configPath,
				 cfg.jsonIndex);
		 }
		 else {
			 for (const auto& meshPath : cfg.meshPaths) {
				 if (meshPath.empty()) {
					 continue;
				 }

				 if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
					 LightData::meshPathToJsonCfgExteriors[meshPath].push_back(cfg);
				 }
				 else {
					 LightData::meshPathToJsonCfg[meshPath].push_back(cfg);
				 }
			 }

			 logger::info(
				 "AppendNewConfigEntryFromLight: added mesh config '{}' at {} index {}",
				 cfg.menuName,
				 cfg.configPath,
				 cfg.jsonIndex);
		 }

		 return true;
	 }
	 catch (const std::exception& e) {
		 logger::error("AppendNewConfigEntryFromLight failed: {}", e.what());
		 return false;
	 }
 }