
#include "config.hpp"
#include "LightData.h"
#include "global.h"
#include "LightManager.h"
#include "utility.h"

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
			if (data["flags"].is_string()) {
				std::string flags = data["flags"].get<std::string>();
				size_t start = 0, end = flags.find(',');
				while (end != std::string::npos) {
					config.flags.push_back(flags.substr(start, end - start));
					start = end + 1;
					end = flags.find(',', start);
				}
				config.flags.push_back(flags.substr(start));
			}
			else if (data["flags"].is_array()) {
				for (auto& f : data["flags"]) {
					config.flags.push_back(f.get<std::string>());
				}
			}
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

		json& originalEntry = data[0];

		//nodename can be a array but config only hold 1 node name
		if (originalEntry.contains("nodeName")) {
			newEntry["nodeName"] = originalEntry["nodeName"];
		}
		else {
			newEntry["nodeName"] = config.nodeName; // fallback
		}

		if (originalEntry.contains("meshPath")) {
			newEntry["meshPath"] = originalEntry["meshPath"];
		}
		else {
			newEntry["meshPath"] = config.meshPath; // fallback
		}

		newEntry["menuName"] = config.menuName;

#define JSON_WRITE(C, I) newEntry[#C] = config.C;
#define JSON_WRITE_FLOAT(C, I) newEntry[#C] = truncateDecimals(config.C, 2);

		FOREACH_BOOL(JSON_WRITE)
		FOREACH_FLOAT(JSON_WRITE_FLOAT)

		newEntry["color"] = { config.diffuseColor[0], config.diffuseColor[1], config.diffuseColor[2] };
		newEntry["position"] = { 
			truncateDecimals(config.position[0], 2),
			truncateDecimals(config.position[1], 2),
			truncateDecimals(config.position[2], 2)
		};
		newEntry["flags"] = config.flags;
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


// ini parser already filled the users desired, priority nodes, so these ones have no priority
// doesnet actually sort file path or node name atm.
void sortInPriorityList(const LightConfig& cfg) {

	RE::BSFixedString nodeNameBS(cfg.nodeName.c_str());

	if (std::find(globals::priorityList.begin(),
		globals::priorityList.end(),
		nodeNameBS) == globals::priorityList.end())
	{
		globals::priorityList.push_back(nodeNameBS);
	}
}

void parseTemplates() {
	logger::info("Parsing light templates..");
	std::vector<std::string> paths = GetConfigPaths();
	static uint64_t nextID = 1;

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

		int jsonIndex = 0;

		for (json json : entries) {

			std::vector<std::string> nodeNames;

			if (json.contains("nodeName")) {
				if (json["nodeName"].is_string()) {
					nodeNames.push_back(json["nodeName"].get<std::string>());
				}
				else if (json["nodeName"].is_array()) {
					nodeNames = json["nodeName"].get<std::vector<std::string>>();
				}
			}

			// create a config for each node name listed so 1 json config can work for multuple nodes.
			for (const auto& nodeName : nodeNames) {
				LightConfig cfg;
				loadConfiguration(cfg, json);
				cfg.configPath = p;
				cfg.configID = nextID++;

				cfg.jsonIndex = jsonIndex;

				//set each cfg name according to the current iteration of all node names read (for multiple node names for 1 config support)
				cfg.nodeName = nodeName;

				toLower(cfg.nodeName);

				sortInPriorityList(cfg);

				cfg.print();

				LightData::configIDToJsonCfg[cfg.configID] = cfg;
				LightData::defaultConfigs[cfg.configID] = cfg;
				LightData::nodeNameToJsonCfg[cfg.nodeName].push_back(cfg);
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

			for (const auto& meshPath : meshFilePaths) {
				LightConfig cfg;
				loadConfiguration(cfg, json);
				cfg.configPath = p;
				cfg.configID = nextID++;

				cfg.jsonIndex = jsonIndex;

				//set each cfg name according to the current iteration of all meshpaths read (for multiple mesh paths for 1 config support)
				cfg.meshPath = meshPath;

				toLower(cfg.meshPath);

				cfg.print();

				LightData::configIDToJsonCfg[cfg.configID] = cfg;
				LightData::defaultConfigs[cfg.configID] = cfg;
				LightData::meshPathToJsonCfg[cfg.meshPath].push_back(cfg);
			}

			//used to get the right index to save back too
			jsonIndex++;
		}
	}
}

std::vector<LightConfig> findConfigsForNode(std::string& nodeName)
{
	std::vector<LightConfig> result;

	//careful mutablitiy here idk if matters that much just marking it. 
	toLower(nodeName); 

	if (nodeName.empty())
		return result;

	auto it = LightData::nodeNameToJsonCfg.find(nodeName);
	if (it != LightData::nodeNameToJsonCfg.end()) {
		result = it->second;
		return result;
	}

	logger::warn(
		" found node'{}' but no config exists",
		nodeName
	);

	return result;
}


