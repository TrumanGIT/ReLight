#pragma once  

#include <string>  
#include <vector>  
#include <filesystem>
#include <fstream>
#include <array>

#include "logger.hpp"

namespace fs = std::filesystem;

constexpr int COL_SIZE = 3;
constexpr int POS_SIZE = 3;

#define FOREACH_TEMPLATE(T) \
T(candle) \
T(chandelier) \
T(fires) \

#define FOREACH_BOOL(B) \
B(shadowLight, false) \
B(portalStrict, false) \
B(affectLand, true) \
B(affectWater, true) \
B(neverFades, true) \

#define FOREACH_FLOAT(F) \
F(fade, 0) \
F(radius, 0) \
F(ambientRatio, 0.1) \
F(fov, 90.f) \
F(falloff, 1.f) \
F(nearDistance, 5.f) \
F(depthBias, 0.0005f) \

#define BOOL2DEF(B, I) bool B{I};
#define FLOAT2DEF(B, I) float B{I};
#define BOOL2PRINT(C, I) logger::info(" {:30s} : {:s}", #C, C ? "true" : "false");
#define FLOAT2PRINT(C, I) logger::info(" {:30s} : {:.2f}", #C, C);

struct LightConfig {
    FOREACH_BOOL(BOOL2DEF);
    FOREACH_FLOAT(FLOAT2DEF);
    std::string nodeName{};                  // TESObjectLIGH->data.radius
    std::array<int, COL_SIZE> diffuseColor{};     // TESObjectLIGH->data.color.red, blue green 
    std::array<float, POS_SIZE> position{};       // RE::NiPointLight->local.translate.x, y z
    std::vector<std::string> flags{};             // TES::ObjectLigh->data.flags

    void print() {
        logger::info("Node name          : {}", nodeName);
        FOREACH_BOOL(BOOL2PRINT);
        FOREACH_FLOAT(FLOAT2PRINT);
        logger::info(" position          : [{}, {}, {}] ", position[0], position[1], position[2]);
        logger::info(" color             : [{}, {}, {}] ", diffuseColor[0], diffuseColor[1], diffuseColor[2]);
        logger::info(" flags    :");
        for (const auto& f: flags) {
            logger::info("  {}", f);
        }
    }
    bool operator<(const LightConfig& other) const {
        return nodeName < other.nodeName;
    }
};

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