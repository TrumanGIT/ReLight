#pragma once  

#include <string>  
#include <vector>  
#include <filesystem>
#include <fstream>
#include <array>

namespace fs = std::filesystem;

constexpr int COL_SIZE = 3;
constexpr int POS_SIZE = 3;

#define FOREACH_TEMPLATE(T) \
T(candle) \
T(chandelier) \
T(fires) \

struct LightConfig {
    std::string nodeName{};
    float fade{0.0};                              // TESObjectLIGH->fade
    std::uint32_t radius{0};                      // TESObjectLIGH->data.radius
    std::array<int, COL_SIZE> RGBValues{};        // TESObjectLIGH->data.color.red, blue green 
    std::array<float, POS_SIZE> position{};       // RE::NiPointLight->local.translate.x, y z
    std::vector<std::string> flags{};             // TES::ObjectLigh->data.flags

    void print() {
        logger::info("Node name : {}", nodeName);
        logger::info(" fade     : {}", fade);
        logger::info(" radius   : {}", radius);
        logger::info(" position : [{}, {}, {}] ", position[0], position[1], position[2]);
        logger::info(" color    : [{}, {}, {}] ", RGBValues[0], RGBValues[1], RGBValues[2]);
        logger::info(" flags    :");
        for (const auto& f: flags) {
            logger::info("  {}", f);
        }
    }
};

extern std::vector<LightConfig> lightConfigs;

inline std::string ToUTF8(const fs::path& p) {
    auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

inline std::string GetConfigDir() {
    const auto root = std::filesystem::path(REL::Module::get().filename()).parent_path();
    return (root / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / "Configs" / "").string();
}

inline std::vector<std::string> GetConfigPaths() {
    const fs::path dir = GetConfigDir();
#define DIR2PATH(T) (dir / #T / #T "cfg.json").string(),

    static std::vector<std::string> paths = {
        FOREACH_TEMPLATE(DIR2PATH)
    };

    return paths;
}

bool loadConfiguration(LightConfig& config, const std::string& configPath);

void parseTemplates();