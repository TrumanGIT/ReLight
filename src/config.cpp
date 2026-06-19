
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

		if (data.contains("menuCategory")) {
			config.menuCategory = data["menuCategory"].get<std::string>();
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
				auto val = arr[i].get<float>(); 
				config.position[i] = val;
			}
		}

		if (data.contains("rotation") && data["rotation"].is_array()) {
			auto& arr = data["rotation"];
			for (size_t i = 0; i < std::min(arr.size(), size_t(POS_SIZE)); ++i) {
				auto val = arr[i].get<float>(); 
				config.rotation[i] = val;
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

		if (data.contains("externalEmittance") && data["externalEmittance"].is_string()) {
			std::string editorID = data["externalEmittance"].get<std::string>();
			if (!editorID.empty()) {
				auto* form = RE::TESForm::LookupByEditorID(editorID);
				config.emittanceRegion = form ? form->As<RE::TESRegion>() : nullptr;
				if (!config.emittanceRegion) {
					logger::warn("Failed to find TESRegion for externalEmittance '{}'", editorID);
				}
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
		else if (!config.refFormIDsAndModNames.empty()) {
			if (config.refFormIDsAndModNames.size() == 1) {
				newEntry["refID"] = config.refFormIDsAndModNames.front();
			}
			else {
				newEntry["refID"] = config.refFormIDsAndModNames;
			}
		}

		if (originalEntry.contains("baseID")) {
			newEntry["baseID"] = originalEntry["baseID"];
		}
		else if (!config.baseFormIDsAndModNames.empty()) {
			if (config.baseFormIDsAndModNames.size() == 1) {
				newEntry["baseID"] = config.baseFormIDsAndModNames.front();
			}
			else {
				newEntry["baseID"] = config.baseFormIDsAndModNames;
			}
		}

		newEntry["menuName"] = config.menuName;

		newEntry["externalEmittance"] = config.externalEmittance;

		// Checks if the template already had a group name to re-apply it, otherwise applies an empty menuCategory
		if (!config.menuCategory.empty()) {

			newEntry["menuCategory"] = config.menuCategory;

			logger::info("configuration saved with menu category: {}", newEntry["menuCategory"].get<std::string>());

		} else {

			newEntry["menuCategory"] = "";

			logger::info("configuration saved with an empty menu category.");

		}

#define JSON_WRITE(C, I) newEntry[#C] = config.C;

		FOREACH_BOOL(JSON_WRITE)

			const bool isSpotLight =
			(config.flags & static_cast<int>(LIGHT_FLAGS::kSpotLight)) != 0;

		const float maxRadius = isSpotLight ? 5000.0f : 500.0f;

			// brighness useses in game slider copy value for a more stable value
		newEntry["brightness"] = truncateDecimals(config.startingFade, 2);
		// clamp radius as community shaders can send brighntess / radius values to infinity
		newEntry["radius"] = truncateDecimals(std::clamp(config.radius, 0.1f, maxRadius), 2);
		newEntry["fov"] = truncateDecimals(config.fov, 2);
		newEntry["falloff"] = truncateDecimals(config.falloff, 2);
		newEntry["nearDistance"] = truncateDecimals(config.nearDistance, 2);
		newEntry["depthBias"] = truncateDecimals(config.depthBias, 2);
		newEntry["ambientRatio"] = truncateDecimals(config.ambientRatio, 2);
		newEntry["flickerIntensity"] = truncateDecimals(config.flickerIntensity, 2);
		newEntry["flickersPerSecond"] = truncateDecimals(config.flickersPerSecond, 2);
		newEntry["flickerAmplitude"] = truncateDecimals(config.flickerAmplitude, 2);
		newEntry["flickerRandomness"] = truncateDecimals(config.flickerRandomness, 2);
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

		newEntry["rotation"] = {
		truncateDecimals(config.rotation[0], 2),
		truncateDecimals(config.rotation[1], 2),
		truncateDecimals(config.rotation[2], 2)
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

		if (!config.refFormIDsAndModNames.empty()) {
			newEntry["refID"] = config.refFormIDsAndModNames;
		}

		newEntry["menuCategory"] = config.menuCategory;
		newEntry["menuName"] = config.menuName;
		newEntry["externalEmittance"] = config.externalEmittance;

		logger::info("New configuration saved with an empty menu category by default in the JSON for the user to be free to tweak if he wants to unclutter the menu.");

#define JSON_WRITE(C, I) newEntry[#C] = config.C;
		FOREACH_BOOL(JSON_WRITE)

		const bool isSpotLight =
		(config.flags & static_cast<int>(LIGHT_FLAGS::kSpotLight)) != 0;

		const float maxRadius = isSpotLight ? 5000.0f : 500.0f;

		newEntry["brightness"] = truncateDecimals(config.startingFade, 2);
		newEntry["radius"] = truncateDecimals(std::clamp(config.radius, 0.1f, maxRadius), 2);
		newEntry["fov"] = truncateDecimals(config.fov, 2);
		newEntry["falloff"] = truncateDecimals(config.falloff, 2);
		newEntry["nearDistance"] = truncateDecimals(config.nearDistance, 2);
		newEntry["depthBias"] = truncateDecimals(config.depthBias, 2);
		newEntry["ambientRatio"] = truncateDecimals(config.ambientRatio, 2);
		newEntry["flickerIntensity"] = truncateDecimals(config.flickerIntensity, 2);
		newEntry["flickersPerSecond"] = truncateDecimals(config.flickersPerSecond, 2);
		newEntry["flickerAmplitude"] = truncateDecimals(config.flickerAmplitude, 2);
		newEntry["flickerRandomness"] = truncateDecimals(config.flickerRandomness, 2);
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

		newEntry["rotation"] = {
			truncateDecimals(config.rotation[0], 2),
			truncateDecimals(config.rotation[1], 2),
			truncateDecimals(config.rotation[2], 2)
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

 inline bool AddRefIDToFirstJsonObject(
    const std::string& configPath,
    const std::string& refID)
{
    try {
        std::ifstream inFile(configPath);
        if (!inFile.is_open()) {
            logger::error("Failed to open config: {}", configPath);
            return false;
        }

        json data;
        inFile >> data;
        inFile.close();

        if (!data.is_array() || data.empty()) {
            logger::error("Config was not a valid JSON array");
            return false;
        }

        json& firstObj = data[0];

        if (!firstObj.contains("refFormIDsAndModNames")) {
            firstObj["refFormIDsAndModNames"] = json::array();
        }

        auto& refArray = firstObj["refFormIDsAndModNames"];

        for (const auto& existing : refArray) {
            if (existing.is_string() && existing.get<std::string>() == refID) {
                logger::info("Ref ID already exists in config");
                return true;
            }
        }

        refArray.push_back(refID);

        std::ofstream outFile(configPath);
        if (!outFile.is_open()) {
            logger::error("Failed to save config: {}", configPath);
            return false;
        }

        outFile << data.dump(4);

        logger::info("Added ref ID '{}' to {}", refID, configPath);

        return true;
    }
    catch (const std::exception& e) {
        logger::error("AddRefIDToFirstJsonObject failed: {}", e.what());
        return false;
    }
 }

  bool AddRefIDToAllEntries(
	 const std::string& configPath,
	 const std::string& refID)
 {
	 try {
		 std::ifstream inFile(configPath);
		 if (!inFile.is_open()) {
			 logger::error("Failed to open config file for reading: {}", configPath);
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

			 if (!entry.contains("refID")) {
				 entry["refID"] = json::array();
				 entry["refID"].push_back(refID);
				 changed = true;
				 continue;
			 }

			 auto& refField = entry["refID"];

			 if (refField.is_string()) {
				 std::string existing = refField.get<std::string>();

				 if (existing != refID) {
					 refField = json::array({ existing, refID });
					 changed = true;
				 }
			 }
			 else if (refField.is_array()) {
				 bool found = false;

				 for (const auto& item : refField) {
					 if (item.is_string() && item.get<std::string>() == refID) {
						 found = true;
						 break;
					 }
				 }

				 if (!found) {
					 refField.push_back(refID);
					 changed = true;
				 }
			 }
			 else {
				 logger::warn("refID in {} was neither string nor array", configPath);
			 }
		 }

		 if (!changed) {
			 logger::info("refID '{}' already present in all entries of {}", refID, configPath);
			 return true;
		 }

		 std::ofstream outFile(configPath, std::ios::trunc);
		 if (!outFile.is_open()) {
			 logger::error("Failed to open config file for writing: {}", configPath);
			 return false;
		 }

		 outFile << data.dump(4);
		 logger::info("Added refID '{}' to config file {}", refID, configPath);

		 return true;
	 }
	 catch (const std::exception& e) {
		 logger::error("Failed updating refID in {}: {}", configPath, e.what());
		 return false;
	 }
 }

  inline void parseFormIDs(json& json, const std::string& p, uint16_t jsonIndex, const std::string& key, bool isBaseID)
  {
	  std::vector<std::string> formIDs;
	  if (json.contains(key)) {
		  if (json[key].is_string()) {
			  formIDs.push_back(json[key]);
		  }
		  else if (json[key].is_array()) {
			  formIDs = json[key].get<std::vector<std::string>>();
		  }
	  }

	  if (formIDs.empty()) {
		  return;
	  }

	  LightConfig cfg;
	  loadConfiguration(cfg, json);

	  cfg.configPath = p;
	  cfg.configID = globals::nextID++;
	  cfg.jsonIndex = jsonIndex;

	  if (isBaseID) {
		  cfg.baseFormIDsAndModNames = formIDs;
	  }
	  else {
		  cfg.refFormIDsAndModNames = formIDs;
	  }

	  auto& idsAndModNames = isBaseID ? cfg.baseFormIDsAndModNames : cfg.refFormIDsAndModNames;

	  for (auto& id : idsAndModNames) {
		  toLower(id);
	  }

	  for (const auto& id : idsAndModNames) {
		  if (id.empty()) {
			  continue;
		  }

		  auto tildePos = id.find('~');
		  if (tildePos == std::string::npos) {
			  logger::warn("Invalid {} format '{}', expected FormID~ModName", key, id);
			  continue;
		  }

		  std::string formIDStr = trim(id.substr(0, tildePos));
		  std::string modName = trim(id.substr(tildePos + 1));
		  toLower(modName);

		  try {
			  if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
				  formIDStr = formIDStr.substr(2);
			  }

			  std::uint32_t parsedID = std::stoul(formIDStr, nullptr, 16);

			  auto* dataHandler = RE::TESDataHandler::GetSingleton();
			  if (!dataHandler) {
				  logger::warn("TESDataHandler was null while parsing {} '{}'", key, id);
				  continue;
			  }

			  // unified resolver (handles normal mods + light mods + vanilla fallback)
			  const RE::TESFile* file = ResolveTESFileWithFallback(dataHandler, modName);

			  if (!file) {
				  logger::warn("Invalid mod name '{}' in {} '{}'", modName, key, id);
				  continue;
			  }

			  // remove load order index of non light plugins incase users load order changes
			  if (!file->IsLight()) {
				  parsedID &= 0x00FFFFFF;
			  }

			  // parsedID is local FormID key
			  RE::FormID runtimeID = parsedID;

			  if (isBaseID) {
				  if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
					  LightData::baseFormIDToJsonCfgExteriors[runtimeID].push_back(cfg);
					  logger::info("adding base ID outdoor config 0x{:08X}", static_cast<std::uint32_t>(runtimeID));
				  }
				  else {
					  LightData::baseFormIDToJsonCfg[runtimeID].push_back(cfg);
					  logger::info("adding base ID config 0x{:08X}", static_cast<std::uint32_t>(runtimeID));
				  }
			  }
			  else {
				  if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
					  LightData::refFormIDToJsonCfgExteriors[runtimeID].push_back(cfg);
					  logger::info("adding ref ID outdoor config 0x{:08X}", static_cast<std::uint32_t>(runtimeID));
				  }
				  else {
					  LightData::refFormIDToJsonCfg[runtimeID].push_back(cfg);
					  logger::info("adding ref ID config 0x{:08X}", static_cast<std::uint32_t>(runtimeID));
				  }
			  }
		  }
		  catch (...) {
			  logger::warn("Failed to parse {} '{}' in {}", key, id, p);
			  continue;
		  }
	  }

	  // only create one lightconfig object for these maps
	  LightData::configIDToJsonCfg[cfg.configID] = cfg;
	  LightData::defaultConfigs[cfg.configID] = cfg;

	  cfg.print(cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor));
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

//parse json configs 
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

			parseFormIDs(json, p, jsonIndex, "refID", false);
			parseFormIDs(json, p, jsonIndex, "baseID", true);

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

				// only create one lightconfig object for these maps
				LightData::configIDToJsonCfg[cfg.configID] = cfg;
				LightData::defaultConfigs[cfg.configID] = cfg;

				// create one lightconfig object for each of these maps
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
	 const std::string& menuCategory,
	 const std::string& menuName,
	 RE::NiLight* niLight,
	 const std::string& refIDAndModName,
	 const std::string& matched,
	 const LightConfig& baseCfg,
	 bool refLight,
	 RE::FormID refFormID, bool preserveConfigID)
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

		 cfg.position = baseCfg.position;
		 cfg.configPath = configPath;
		 cfg.jsonIndex = jsonIndex;
		 cfg.configID = preserveConfigID ? baseCfg.configID : globals::nextID++;
		// since we apply from a base config mabye we dont even need to set menu catagory and mabye menu name as well
		 cfg.menuCategory = menuCategory;
		 cfg.menuName = menuName;
		 cfg.refFormIDsAndModNames.clear(); // clear copied baseCfg refs since were adding a new json object to json file
		 if (!refIDAndModName.empty()) {
			 cfg.refFormIDsAndModNames.push_back(refIDAndModName);
		 }

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

		 if (!cfg.refFormIDsAndModNames.empty()) {
			 cfg.flags |= static_cast<uint32_t>(LIGHT_FLAGS::kNoMerging);

			 if (cfg.refFormIDsAndModNames.size() == 1) {
				 newEntry["refID"] = cfg.refFormIDsAndModNames.front();
			 }
			 else {
				 newEntry["refID"] = cfg.refFormIDsAndModNames;
			 }
		 }

		 newEntry["menuCategory"] = cfg.menuCategory;
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
		 newEntry["flickerRandomness"] = truncateDecimals(cfg.flickerRandomness, 2);
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

		 newEntry["rotation"] = {
			 truncateDecimals(cfg.rotation[0], 2),
			 truncateDecimals(cfg.rotation[1], 2),
			 truncateDecimals(cfg.rotation[2], 2)
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
				 cfg.menuCategory,
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
				 cfg.menuCategory,
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