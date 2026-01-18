#include "nlohmann/json.hpp"
#include "config.hpp"
#include "LightData.h"
#include "global.h"

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

bool loadConfiguration(LightConfig& config, const std::string& configPath) {
    try {
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            logger::error("Failed to open config file: {}", ToUTF8(configPath));
            return false;
        }

        std::string raw((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());

        json data = json::parse(raw, nullptr, true, true);

        if (data.contains("nodeName")) {
            config.nodeName = data["nodeName"].get<std::string>();
        }

        FOREACH_BOOL(BOOL2JSON_READ)

        FOREACH_FLOAT(FLOAT2JSON_READ)

        // clamp radius.
		config.radius = std::clamp(config.radius, 0.0f, 256.f);

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
                auto val = arr[i].get<int>();
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

        if(data.contains("attachPath") && data["attachPath"].is_array()) {
            config.attachPath.clear();

            for (const auto& v : data["attachPath"]) {
                if (!v.is_number_integer()) {
                    logger::warn("Non-integer value in attachPath in {}", configPath);
                    continue;
                }

                int idx = v.get<int>();
                if (idx < 0) {
                    logger::warn("Negative index {} in attachPath in {}", idx, configPath);
                    continue;
                }

                config.attachPath.push_back(idx);
            }
        }

        return true;
    }
    catch (const json::exception& e) {
        logger::error("cannot read JSON file due to {}", std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        logger::error("cannot read JSON file due to {}", std::string(e.what()));
        return false;
    }
}

bool saveConfiguration(const LightConfig& config, const std::string& configPath) {
	try {
		json data;

		data["nodeName"] = config.nodeName;

#define JSON_WRITE(C, I) data[#C] = config.C;

        FOREACH_BOOL(JSON_WRITE);
        FOREACH_FLOAT(JSON_WRITE);

		data["color"] = {
            config.diffuseColor[0],
            config.diffuseColor[1],
            config.diffuseColor[2]
		};

		data["position"] = {
			config.position[0],
			config.position[1],
			config.position[2]
		};

		data["flags"] = config.flags;

        data["attachPath"] = config.attachPath;

		std::ofstream out(configPath, std::ios::trunc);
		if (!out.is_open()) {
			logger::error("Failed to open config file for writing: {}", configPath);
			return false;
		}

		out << data.dump(4);

        logger::info("Successfully saved light data to template at {}", configPath);

		return true;
	}
	catch (const std::exception& e) {
		logger::error("Failed to write config {}: {}", configPath, e.what());
		return false;
	}
}

void sortFilePathOrNodeName(const LightConfig& cfg) {
    // add to priority list if a node name

        // add to meshfile path if contains a "/"
 //   if (cfg.nodeName.contains("/")) {

   //     meshFilePaths.push_back(cfg.nodeName);
    //}

    if (std::find(priorityList.begin(),
        priorityList.end(),
        cfg.nodeName) == priorityList.end())
    {
        priorityList.push_back(cfg.nodeName);
    }
}

void parseTemplates() {
    logger::info("Parsing light templates..");
    std::vector<std::string> paths = GetConfigPaths();
    static uint64_t nextID = 1;

    for (const auto& p : paths) {
        logger::info(" reading.. {}", p);
        LightConfig cfg;
        loadConfiguration(cfg, p);
        cfg.configPath = p;
        cfg.configID = nextID++;
        cfg.seed = cfg.seed = static_cast<uint32_t>(std::hash<std::string>{}(cfg.nodeName)); cfg.print();
        cfg.rngState = cfg.seed ? cfg.seed : 1;
        cfg.startingFade = cfg.fade;
       // Template temp;
      //  temp.config = std::move(cfg);
        sortFilePathOrNodeName();
        LightData::configIDToJsonCfg[cfg.configID] = cfg;
        LightData::defaultConfigs[cfg.nodeName] = cfg;
        LightData::nodeNameToJsonCfg[cfg.nodeName] = std::move(cfg);
    }
}

LightConfig findConfigForNode(const std::string& nodeName)
{
    if (nodeName.empty()) return LightConfig();
    for (auto& pair : LightData::nodeNameToJsonCfg) {
        const auto& name = pair.first;
        if (nodeName.find(name) != std::string::npos)
            return pair.second;
    }
    logger::warn("No template found by findConfigForNode for node {}", nodeName);
    return LightConfig(); // no match
}