#include "nlohmann/json.hpp"
#include "config.hpp"
#include "LightData.h"
#include "global.h"

using json = nlohmann::json;

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

        if (data.contains("fade")) {
            config.fade = data["fade"].get<float>();
        }

        if (data.contains("radius")) {
            config.radius = data["radius"].get<uint32_t>();
        }

        if (data.contains("emittanceColor") && data["emittanceColor"].is_array()) {
            auto& arr = data["emittanceColor"];
            for (size_t i = 0; i < std::min(arr.size(), size_t(COL_SIZE)); ++i) {
                auto val = arr[i].get<int>();
                config.RGBValues[i] = val > 255 ? val - 255 : val;
            }
        }

        if (data.contains("position") && data["position"].is_array()) {
            auto& arr = data["position"];
            for (size_t i = 0; i < std::min(arr.size(), size_t(POS_SIZE)); ++i) {
                auto val = arr[i].get<int>();
                config.RGBValues[i] = val > 255 ? val - 255 : val;
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

void parseTemplates() {
    logger::info("Parsing light templates..");
    std::vector<std::string> paths = GetConfigPaths();

    for (const auto& p : paths) {
        logger::info(" reading.. {}", p);
        LightConfig cfg;
        loadConfiguration(cfg, p);
        cfg.print();
        std::vector<RE::NiPointer<RE::NiPointLight>> objs{};
        niPointLightNodeBank[std::move(cfg)] = objs;
    }
}