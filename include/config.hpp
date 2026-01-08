#pragma once  

#include <string>  
#include <vector>  
#include <filesystem>
#include <fstream>
#include <array>
//#include "global.h"
#include "logger.hpp"

namespace fs = std::filesystem;

constexpr int COL_SIZE = 3;
constexpr int POS_SIZE = 3;

#define FOREACH_BOOL(B) \
B(shadowLight, false) \
B(portalStrict, false) \
B(affectLand, true) \
B(affectWater, true) \
B(neverFades, true) \

#define FOREACH_FLOAT(F) \
F(fade, 0.f) \
F(radius, 0.f) \
F(size, 1.f) \
F(cutoffOverride , 0.5f) \
F(ambientRatio, 0.1f) \
F(fov, 90.f) \
F(falloff, 1.f) \
F(nearDistance, 5.f) \
F(depthBias, 0.0005f) \
F(constAttenuation, 0.f) \
F(linearAttenuation, 0.f) \
F(quadraticAttenuation, 0.f) \
F(flickerIntensity, 0.2f) \
F(flickersPerSecond, 3.f) \

#define BOOL2DEF(B, I) bool B{I};
#define FLOAT2DEF(B, I) float B{I};
#define BOOL2PRINT(C, I) logger::info(" {:30s} : {:s}", #C, C ? "true" : "false");
#define FLOAT2PRINT(C, I) logger::info(" {:30s} : {:.2f}", #C, C);

struct LightConfig {
    FOREACH_BOOL(BOOL2DEF);
    FOREACH_FLOAT(FLOAT2DEF);
    std::string configPath{};                     // save the path from where this config is loaded
    std::string nodeName{};                       //NiPointLightRunTime->data.radius
    std::array<int, COL_SIZE> diffuseColor{};     // NiPointLightRunTime->data.color.red, blue green 
    std::array<float, POS_SIZE> position{};       // RE::NiPointLight->local.translate.x, y z
    std::vector<std::string> flags{};             // NiPointLightRunTime->data.flags

    void print() {
        logger::info("Path               : {}", configPath);
        logger::info(" Node name         : {}", nodeName);
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

//struct Template {
  //  LightConfig config;
 //   std::vector<RE::NiPointer<RE::NiPointLight>> bank;
//};

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

    std::vector<std::string> paths;

    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
		logger::critical("Config directory {} does not exist.", ToUTF8(dir));
        return paths;
    }

    auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    if (ec) {
        logger::critical("Cannot iterate over {}: {}", ToUTF8(dir), ec.message());
    }

    while (it != end) {
        const auto& p = it->path();

        if (fs::is_regular_file(p, ec) && p.extension() == ".json") {
			logger::info("Found config file: {}", ToUTF8(p));
            paths.push_back(ToUTF8(p));
        }

        ec.clear();

        it.increment(ec);

        if (ec) {
            logger::critical("Skipping path under {}: {}", ToUTF8(dir), ec.message());
            ec.clear();
        }
    }

    return paths;
}


bool loadConfiguration(LightConfig& config, const std::string& configPath);

bool saveConfiguration(const LightConfig& config, const std::string& configPath);

void parseTemplates();

