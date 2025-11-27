#pragma once  

#include <string>  
#include <vector>  
#include <filesystem>

namespace fs = std::filesystem;

constexpr int COL_SIZE = 3;
constexpr int POS_SIZE = 3;

struct LightConfig {
    std::string nodeName{};
    float fade{0.0};                              // TESObjectLIGH->fade
    std::uint32_t radius{0};                      // TESObjectLIGH->data.radius
    std::array<int, COL_SIZE> RGBValues{};        // TESObjectLIGH->data.color.red, blue green 
    std::array<float, POS_SIZE> position{};       // RE::NiPointLight->local.translate.x, y z
    std::vector<std::string> flags{};             // TES::ObjectLigh->data.flags
};

inline std::string ToUTF8(const fs::path& p) {
    auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

inline std::string GetConfigPath() {
    const auto root = std::filesystem::path(REL::Module::get().filename()).parent_path();
    return (root / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / PRODUCT_NAME ".json").string();
}

bool LoadConfiguration(LightConfig& config);