#pragma once  

#include <string>  
#include <vector>  
#include <filesystem>
#include <fstream>
#include <array>
#include "logger.hpp"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

constexpr int COL_SIZE = 3;
constexpr int POS_SIZE = 3;

#define FOREACH_BOOL(B) \
B(shadowLight, false) \
B(portalStrict, true) \
B(affectLand, true) \
B(affectWater, true) \
B(neverFades, true) \

#define FOREACH_FLOAT(F) \
F(brightness, 3.0f) \
F(radius, 250.f) \
F(fov, 1.0f) \
F(falloff, 1.f) \
F(nearDistance, 5.f) \
F(depthBias, 1.0f) \
F(ambientRatio, 0.1f) \
F(flickerIntensity, 0.1f) \
F(flickersPerSecond, 0.1f) \
F(flickerTime, 0.0f) \
F(flickerAmplitude, 0.0f) \
F(startingFade, 0.f) \
F(size, 2.0f) \
F(cutoffOverride , 0.05f) \

#define BOOL2DEF(B, I) bool B{I};
#define FLOAT2DEF(B, I) float B{I};
#define BOOL2PRINT(C, I) logger::info(" {:30s} : {:s}", #C, C ? "true" : "false");
#define FLOAT2PRINT(C, I) logger::info(" {:30s} : {:.2f}", #C, C);

enum class LIGHT_FLAGS : uint32_t
{
    kCandle = 1 << 0,
    kChandelier = 1 << 1,
    kFire = 1 << 2,
    kGiantCampfire = 1 << 3,
    kOther = 1 << 4,
    kSpotLight = 1 << 5,
    kIncreasedMergeDistance = 1 << 6,
    kIncreasedMenuXYZScale = 1 << 7,
    kNoMerging = 1 << 8,
    kOutdoor = 1 << 9
};

inline const std::unordered_map<LIGHT_FLAGS, std::string> LightFlagNames{
    { LIGHT_FLAGS::kCandle, "Candle" },
    { LIGHT_FLAGS::kChandelier, "Chandelier" },
    { LIGHT_FLAGS::kFire, "Fire" },
    { LIGHT_FLAGS::kGiantCampfire, "GiantCampfire" },
    { LIGHT_FLAGS::kOther, "Other" },
    { LIGHT_FLAGS::kSpotLight, "SpotLight" },
    { LIGHT_FLAGS::kIncreasedMergeDistance, "IncreasedMergeDistance" },
    { LIGHT_FLAGS::kIncreasedMenuXYZScale, "IncreasedMenuXYZScale" },
    { LIGHT_FLAGS::kNoMerging, "NoMerging" },
    { LIGHT_FLAGS::kOutdoor, "Outdoor" }
};

struct LightConfig {
    FOREACH_BOOL(BOOL2DEF);
    FOREACH_FLOAT(FLOAT2DEF);
    std::string configPath{};                     // save the path from where this config is loaded
    std::vector<std::string> meshPaths;
    std::string menuCategory{};
    std::string menuName{};
    std::vector<std::string> refFormIDsAndModNames;
    std::vector<std::string> baseFormIDsAndModNames;
    std::array<int, COL_SIZE> diffuseColor{};     // NiPointLightRunflickerTime->data.color.red, blue green 
    std::array<float, POS_SIZE> position{0, 0, 30};       // RE::NiPointLight->local.translate.x, y z
    std::array<float, POS_SIZE> rotation{ 0, 0, 0 };       // RE::NiPointLight->local.rotation
    uint32_t flags{ 0 };
    std::vector<int> attachPath;
    uint32_t configID = 0;
    uint16_t jsonIndex = 0;

    inline void printFlags(uint32_t mask)
    {
        if (!mask) {
            logger::info("  <none>");
            return;
        }

        for (const auto& [flag, name] : LightFlagNames) {
            if (mask & static_cast<uint32_t>(flag)) {
                logger::info("  {}", name);
            }
        }
    }

    void print(bool exterior) {
        if (exterior) logger::info("loading exterior config:");
        logger::info("Path               : {}", configPath);
        for (const auto& mesh : meshPaths) {
            logger::info(" Mesh Path         : {}", mesh);
        }
        for (const auto& refID : refFormIDsAndModNames) {
            logger::info(" RefID and Mod Name : {}", refID);
        }
        logger::info(" Menu name         : {}", menuName);
        FOREACH_BOOL(BOOL2PRINT);
        FOREACH_FLOAT(FLOAT2PRINT);
        logger::info(" position          : [{}, {}, {}] ", position[0], position[1], position[2]);
        logger::info(" color             : [{}, {}, {}] ", diffuseColor[0], diffuseColor[1], diffuseColor[2]);
        logger::info(" config ID: {}", configID);
        logger::info(" json Index: {}", jsonIndex);
        logger::info(" flags    :");
        printFlags(flags);

        logger::info(" attachPath    :");
        for (const auto& i : attachPath) {
            logger::info("  {}", i);
        }
    }
    bool operator<(const LightConfig& other) const {
        return configID < other.configID;
    }
};


inline std::string ToUTF8(const fs::path& p) {
    auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

inline double truncateDecimals(const float& value, const int& decimals)
{
    double factor = std::pow(10.0f, decimals);
    double newValue = std::floor(value * factor) / factor;
    logger::debug("Truncating value {} to {} decimal places", value, newValue);
    return newValue;
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

inline bool updateConfigMap (std::vector<LightConfig>& vec, const LightConfig& updatedCfg)
{
    for (auto& existing : vec) {
        if (existing.configID == updatedCfg.configID) {
            existing = updatedCfg;
            return true;
        }
    }
    return false;
}

//used for in game menu
bool AddMeshPathToAllEntries(const std::string& filePath, const std::string& meshPath); 

bool AddRefIDToAllEntries(const std::string& configPath, const std::string& refID);

std::size_t CountJsonEntriesInFile(const std::string& configPath); 

uint32_t ParseFlags(const nlohmann::json& j);

nlohmann::json FlagsToJson(uint32_t mask);

bool loadConfiguration(LightConfig& config, const nlohmann::json& data);

bool saveConfiguration(const LightConfig& config);

bool saveNewConfiguration(LightConfig& config);

bool AppendNewConfigEntryFromLight(
    const std::string& configPath,
    std::uint16_t jsonIndex,
    const std::string& menuCategory,
    const std::string& menuName,
    RE::NiLight* niLight,
    const std::string& refIDsAndModNames,
    const std::string& matched, const LightConfig& baseCfg, bool refLight, RE::FormID refFormID, bool preserveConfigID = false);

void parseTemplates();

std::vector<LightConfig>& findConfigsForMeshPath(std::string& meshPath, bool interior);
