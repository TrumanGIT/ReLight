#pragma once
#include <spdlog/sinks/basic_file_sink.h>
#include "ClibUtil/EditorID.hpp"
#include "LightData.h"
#include "global.h"
#include <fstream>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <sstream>
#include <iostream>

namespace logger = SKSE::log;

inline void toLower(std::string& str) {
    for (auto& c : str) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
}

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}


inline void splitString(const std::string& input, char delimiter, std::vector<std::string>& listToSplit)
{
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        // Trim leading whitespace
        while (!item.empty() && std::isspace(static_cast<unsigned char>(item.front()))) {
            item.erase(item.begin());
        }

        // Trim trailing whitespace
        while (!item.empty() && std::isspace(static_cast<unsigned char>(item.back()))) {
            item.pop_back();
        }

        listToSplit.push_back(item);
        spdlog::info("Added '{}' to whitelist", item);
    }
}

inline bool containsAll(std::string ID,
    const std::vector<std::string_view>& group)
{
    toLower(ID);
    for (auto g : group) {
        if (ID.find(g) == std::string::npos)
            return false;
    }
    return true;
}

// Try to exclude light by editorID.
inline bool excludeLightEditorID(const RE::TESObjectLIGH* light) {

    std::string edid = clib_util::editorID::get_editorID(light);

    if (!edid.empty()) {
        for (const auto& group : keywordLightGroups) {
            if (containsAll(edid, group)) {
                logger::info("Excluding light by editorID: {}", edid);
                return true;
            }
        }
    }

    return false;
}

inline void IniParser() {
    std::ifstream iniFile("Data\\SKSE\\Plugins\\MLO.ini");
    std::string line;

    while (std::getline(iniFile, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == ';') continue;

        if (line.starts_with("disableShadowCasters=")) {
            std::string value = line.substr(std::string("disableShadowCasters=").length());
            toLower(value);

            if (value == "true" || value == "1")
                disableShadowCasters = 1;
            else if (value == "false" || value == "0")
                disableShadowCasters = 0;
            else
                spdlog::warn("Invalid value for disableShadowCasters: {}", value);

            spdlog::info("INI override: disableShadowCasters = {}", disableShadowCasters);

        }
        else if (line.starts_with("disableTorchLights=")) {
            std::string value = line.substr(std::string("disableTorchLights=").length());
            toLower(value);

            if (value == "true" || value == "1")
                disableTorchLights = true;
            else if (value == "false" || value == "0")
                disableTorchLights = false;
            else
                spdlog::warn("Invalid value for disableTorchLights: {}", value);

            spdlog::info("INI override: disableTorchLights = {}", disableTorchLights);
        }
        else if (line.starts_with("removeFakeGlowOrbs=")) {
            std::string value = line.substr(std::string("removeFakeGlowOrbs=").length());
            toLower(value);

            if (value == "true" || value == "1")
                removeFakeGlowOrbs = true;
            else if (value == "false" || value == "0")
                removeFakeGlowOrbs = false;
            else
                spdlog::warn("Invalid value for removeFakeGlowOrbs: {}", value);

            spdlog::info("INI override: removeFakeGlowOrbs = {}", removeFakeGlowOrbs);
        }

        else if (line.starts_with("RGB Values=")) {
            auto values = line.substr(std::string("RGB Values=").length());

            // Find each color
            auto rPos = values.find("Red:");
            auto gPos = values.find("Green:");
            auto bPos = values.find("Blue:");

            if (rPos != std::string::npos) {
                red = static_cast<std::uint8_t>(std::stoi(values.substr(rPos + 4)));
            }
            if (gPos != std::string::npos) {
                green = static_cast<std::uint8_t>(std::stoi(values.substr(gPos + 6)));
            }
            if (bPos != std::string::npos) {
                blue = static_cast<std::uint8_t>(std::stoi(values.substr(bPos + 5)));
            }

            spdlog::info("INI override: Bulb RGB values set to R:{} G:{} B:{}", red, green, blue);
        }

        else if (line.starts_with("whitelist=")) {
            std::string prefix = "whitelist=";

            line.erase(0, prefix.length());

            splitString(line, ',', whitelist);
        }

    }

}

//TODO:: move this to ini file? 
inline void ReadMasterListAndFillExcludes() {
    std::string path = "Data\\SKSE\\Plugins\\Masterlist.ini";

    if (!std::filesystem::exists(path)) {
        logger::warn("INI file not found: {}", path);
        return;
    }

    std::ifstream iniFile(path);
    if (!iniFile.is_open()) {
        logger::warn("Failed to open INI file: {}", path);
        return;
    }

    std::string line;
    int section = 0; // 0=normal, 1=exact excludes, 2=partial excludes

    while (std::getline(iniFile, line)) {
        line = trim(line);
        if (line.empty())
            continue;

        if (line.starts_with(";")) {
            // detect section headers
            if (line.find("EXCLUDE SPECIFIC NODES BY NAME") != std::string::npos) {
                section = 1;
            }
            else if (line.find("EXCLUDE PARTIAL NODES BY NAME") != std::string::npos) {
                section = 2;
            }
            continue;
        }

        if (section == 1) {
            line = trim(line);  // already trimming, good
            toLower(line);      // lowercase for consistency
            exclusionList.push_back(line);
            logger::info("Added exact exclude: '{}'", line);  // wrap in quotes to see trailing whitespace
        }
        else if (section == 2) {
            line = trim(line);
            toLower(line);
            exclusionListPartialMatch.push_back(line);
            logger::info("Added partial exclude: '{}'", line);
        }
    }

    iniFile.close();
}

inline void Initialize() {
     logger::info("loading forms");
    auto dataHandler = RE::TESDataHandler::GetSingleton(); // single instance

    // below is a unused light base object . I need it to pass as arguments for ni point light generator function
 // the ni point light will inherit the data from this object, We will make a copy of this object so we dont not modify the original and update the data of the copy
 // depending on the ni point light we are creating. (see assignNiPointLightsToBank())

    LoadScreenLightMain = dataHandler->LookupForm<RE::TESObjectLIGH>(0x00105300, "Skyrim.esm");
    if (!LoadScreenLightMain) {
        logger::info("TESObjectLIGH LoadScreenLightMain (0x00105300) not found");
    }
} 

inline bool IsInSoulCairnOrApocrypha(RE::PlayerCharacter* player) {
    if (!player) {
        return false;
    }
    auto worldspace = player->GetWorldspace();
    if (!worldspace) {
        // logger::info("worldSpace not valid cant get location");
        return false;  // Not in a worldspace (probably in an interior cell)
    }

    // logger::info("current worldspace = {}", worldspace->GetFormID());

    if (worldspace->GetFormID() == apocryphaFormID || worldspace->GetFormID() == soulCairnFormID) {
        //  logger::info("is in soul cairn or apocrypha");
        return true;
    }

    return false;
}

// TO DO:: I may delete this or repurpose this. 
inline RE::NiPointer<RE::NiPointLight> cloneNPointLight(RE::NiPointLight NiPointLight) {

    RE::NiCloningProcess cloningProcess;
    auto NiPointLightClone = NiPointLight->CreateClone(cloningProcess);
    if (!NiPointLightClone) {
        logger::error("Failed to clone NiNode");
        return nullptr;
    }

    // Successfully cloned node
    return RE::NiPointer<RE::NiObject>(yourGlowNodePrototype);
}

inline void glowOrbRemover(RE::NiNode* node)
{
    if (!node)
        return;

    // Copy raw pointers to avoid iterator invalidation
    std::vector<RE::NiAVObject*> childrenCopy;
    childrenCopy.reserve(node->children.size());

    for (auto& c : node->children) {
        childrenCopy.push_back(c.get());
    }

    for (auto& child : childrenCopy) {
        if (!child)
            continue;

        auto childAsNode = child->AsNode();
        if (!childAsNode)
            continue;

        std::string name = childAsNode->name.c_str();
        toLower(name);

        if (name.find("glow") != std::string::npos) {
            node->DetachChild(childAsNode);
            continue;
        }

        // Recursive call to handle nested nodes
        glowOrbRemover(childAsNode);
    }
}

// TO DO:: Rework well probobly move this to ini file since were done with masterlist
inline bool isExclude(const std::string& nodeName, const char* nifPath, RE::NiNode* root)
{
    if (nodeName == "mpscandleflame01.nif" && removeFakeGlowOrbs) {
        if (!root)
            return true;

        // this is to remove glow orbs from Master particle system candles
        if (auto* flameNode = root->GetObjectByName("mpscandleflame01")) {
            if (auto* flameNiNode = flameNode->AsNode()) {

               
                if (auto* glowEmitter = flameNiNode->GetObjectByName("CandleGlow01-Emitter")) {
                    if (auto* emitterNode = glowEmitter->AsNode()) {
                        emitterNode->SetAppCulled(true);
                        logger::info("Culled CandleGlow01 emitter safely (no iteration)");
                        return true;
                    }
                }
            }
        }
    }

    // Exact matches in exclusion list
    for (const auto& exclude : exclusionList) {
        if (nodeName == exclude)
            return true;
    }

    // Partial matches in exclusion list
    for (const auto& exclude : exclusionListPartialMatch) {
        if (nodeName.find(exclude) != std::string::npos)
            return true;
    }


    if (!nifPath)
        return false;


    std::string path = nifPath;
    toLower(path);

    // Some modded torches name "off" variants incorrectly
    if (path.find("off") != std::string::npos)
        return true;

    return false;
}

//TODO:: delete or reimplement for new ni node bank 
// finds if a incoming node name matches any of our partial search keywords
inline std::string matchedKeyword(const std::string& nodeName)
{

    for (const auto& keyword : priorityList) {
        if (nodeName.find(keyword) != std::string::npos) {
            return keyword;
        }
    }

    return {};
}

//TODO:: rework this for the ni light pointer node bank
//we clone and store ni nodes in a bank on startup to help with performance 
inline RE::NiPointer<RE::NiNode> getNextNodeFromBank(const std::string& keyword)
{
    auto it = keywordNodeBank.find(keyword);

    if (it == keywordNodeBank.end() || it->second.empty()) {
        logger::warn("getNextNodeFromBank: '{}' has no nodes available", keyword);
        return nullptr;
    }

    auto& bank = it->second; // keywords nodebank array

    // static here means initialised once and map contents survive each call (personal note)
    static std::unordered_map<std::string, std::size_t> counters;
    auto& count = counters[keyword];                                 // index for the next node to use in bank

    if (count >= bank.size())
        count = 0; // resets bank to 0 if we passed the limit (likely fine to recycle the nodes)

    RE::NiPointer<RE::NiNode> node = bank[count];

    if (!node) {
        logger::warn("getNextNodeFromBank: '{}' node index {} is null", keyword, count);
        return nullptr;
    }

    count++;

    return node;
}
// TODO:: implement a functiuon that can read light flags and turn into vector of strings. 
uint32_t ParseLightFlagsFromString(const LightConfig& j)
{
    // Start with all bits OFF
    uint32_t flags = 0;

    // If no "flags" key or not an array -> nothing to do
    if (!j.contains("flags") || !j["flags"].is_array())
        return flags;

    // Loop every flag string ("kDynamic", "kFlicker", etc)
    for (auto& f : j["flags"]) {

        // Convert JSON value to std::string
        std::string name = f.get<std::string>();

        // Find the enum in the lookup table
        if (auto it = kLightFlagMap.find(name); it != kLightFlagMap.end()) {

            // Turn ON the corresponding bits using bitwise OR
            flags |= static_cast<uint32_t>(it->second);

        } else {
            // Flag string not recognized
            logger::warn("Unknown TES_LIGHT_FLAGS '{}'", name);
        }
    }

    // Return the combined bitmask
    return flags;
}

//TO DO:: change to use ni point lights

// on startup store a bunch of cloned nodes so we dont have to clone from disk during gameplay
inline void CreateNiPointLightsFromJSONAndFillBank() {
    logger::info("Assigning niPointLight... total groups: {}", keywordNodeBank.size());

    BackupLightData(); 

    for (auto& [jsonConfig, bankedNodes] : NiPointLightNodeBank) {

        for (auto& cfg : jsonConfigs) {
            // Apply current config data to the template light
            SetTESObjectLIGHData(cfg); 

            // Create NiPointLight 
            RE::NiPointLight* niPointLight = Hooks::TESObjectLIGH_GenDynamic::func(
                LoadScreenLightMain,   // Template TESObjectLIGH
                nullptr,               // Reference to attach to (none for now)
                nullptr,               // Node to attach to (none for now)
                false,                 // forceDynamic
                true,                  // useLightRadius
                false                  // affectRequesterOnly
            );

            // Apply position from the JSON config
            ApplyLightPosition(niPointLight, cfg);

            // Determine max nodes based on nodeName
            const size_t maxNodes = (cfg.nodeName == "candle") ? 65 : 25;

            for (size_t i = 0; i < maxNodes; ++i) {
                // Clone the NiPointLight as a NiObject
                auto clonedNiPointLightAsNiObject = cloneNiPointLight(niPointLight);
                
                // Add the cloned light to the bank
                bankedNodes.push_back(clonedNiPointLightAsNiObject);
            }
        }
    }

    RestoreLightData();
    logger::info("Finished assignClonedNodes");
}

// stole this from somewhere Po3 or thiago99,

template <class T, std::size_t size = 5>
inline void write_thunk_call(std::uintptr_t a_src) {
    auto& trampoline = SKSE::GetTrampoline();
    if constexpr (size == 6) {
        T::func = *(uintptr_t*)trampoline.write_call<6>(a_src, T::thunk);
    }
    else {
        T::func = trampoline.write_call<size>(a_src, T::thunk);
    }
}

// checs if fake lights should be disabled by checking some user settings. and excluding dynamicform lights
// or whitelisted lights by checking the plugin name or carryable or shadowcasters lol

inline bool should_disable_light(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, std::string modName)
{
    if (!ref || !light || ref->IsDynamicForm()) {
        return false;
    }

    auto player = RE::PlayerCharacter::GetSingleton();

    if (IsInSoulCairnOrApocrypha(player)) {
        logger::info("player is in apocrypha or soul cairn so we should not disable light");
        return false;
    }
    if (disableShadowCasters == false &&
        light->data.flags.any(RE::TES_LIGHT_FLAGS::kOmniShadow,
            RE::TES_LIGHT_FLAGS::kHemiShadow, RE::TES_LIGHT_FLAGS::kSpotShadow))
    {
        return false;
    }

    if (disableTorchLights == false &&
        light->data.flags.any(RE::TES_LIGHT_FLAGS::kCanCarry))
    {
        return false;
    }

    for (const auto& whitelistedMod : whitelist) {
        if (modName.find(whitelistedMod) != std::string::npos) {
            return false;
        }
    }

    return true;
}
// method to swap fire color models not used anymore see below

/*inline void ApplyColorSwitch(RE::TESModel* bm, const std::string& newPath) {
    if (!bm) return;
    auto currentModel = bm->GetModel();
    if (currentModel != newPath) {
        if (ModelsAndOriginalFilePaths.find(bm) == ModelsAndOriginalFilePaths.end()) {
            ModelsAndOriginalFilePaths[bm] = currentModel;
        }
        bm->SetModel(newPath.c_str());
    }
}*/


//TO DO:: change to use ni point lights
// torches need special placement of light so they dont light up when not equipped. 
inline bool TorchHandler(const std::string& nodeName, RE::NiPointer<RE::NiNode>& a_root)

{
    if (nodeName == "torch") {
        RE::NiNode* attachLight = nullptr;
        RE::NiNode* torchFire = nullptr;

        // must null check everything or crash city. 

        for (auto& child : a_root->children) {
            if (!child) continue; // 
            auto childNode = child->AsNode();
            if (childNode && childNode->name == "TorchFire") {
                torchFire = childNode;
                break;
            }
        }

        if (torchFire) {
            for (auto& child : torchFire->children) {
                if (!child) continue;
                auto childNode = child->AsNode();
                if (childNode && childNode->name == "AttachLight") {
                    attachLight = childNode;
                    break;
                }
            }
        }

        if (attachLight) {
            RE::NiPointer<RE::NiNode> nodePtr = getNextNodeFromBank("torch");
            if (nodePtr) {
                attachLight->AttachChild(nodePtr.get());
                // logger::info("attached light to torch at specific spot {}", nodeName);
                return true;
            }
        }
        else {
            logger::warn("hand held torch light placement failed for {}", nodeName);
        }
    }
    return false;
}


//TO DO:: change to use ni point lights
inline bool applyCorrectNordicHallTemplate(std::string nodeName, RE::NiPointer<RE::NiNode>& a_root)
{
    auto it = nordicHallMeshesAndTemplates.find(nodeName);
    if (it == nordicHallMeshesAndTemplates.end() || it->second.empty()) {
        return false;
    }

    //std::string templatePath = "Meshes\\MLO\\Templates\\" + it->second;

    RE::NiPointer<RE::NiNode> loaded;
    auto args = RE::BSModelDB::DBTraits::ArgsType();

    auto result = RE::BSModelDB::Demand(templatePath.c_str(), loaded, args);
    if (result != RE::BSResource::ErrorCode::kNone || !loaded) {
        logger::warn("Failed to load NIF file {}", templatePath);
        return false;
    }

    auto fadeNode = loaded->AsNode();
    if (!fadeNode) {
        logger::warn("Loaded NIF has no root node: {}", templatePath);
        return false;
    }

    RE::NiCloningProcess cloneProc;

    for (const auto& child : fadeNode->children) {
        if (!child) {
            return true;
        }

        auto childAsNode = child->AsNode();
        if (childAsNode) {
            auto clone = childAsNode->CreateClone(cloneProc);
            if (clone) {
                a_root->AttachChild(clone->AsNode());
            }
        }
    }

    return true;
}

// some nodes are called scene root this is to take care of them. 
inline bool handleSceneRoot(const char* nifPath, RE::NiPointer<RE::NiNode>& a_root, std::string nodeName)
{


    if (nodeName.find("scene") == std::string::npos)
        return false;

    if (!nifPath) {
        return true;
    }

    std::string path = nifPath;

    toLower(path);

    logger::info("scene root node detected, checking path: {}", path);

    std::string bankType;

    if (path.find("candlehornfloor") != std::string::npos || path.find("mwcandle01") != std::string::npos)
    {
        bankType = "candlehornfloor01";
    }


    else if (path.find("candle") != std::string::npos)
    {
        logger::info("handleSceneRootByPath: matched candlehorntable/wall or mwcandle01 in path");
        bankType = "candle";
    }

    else
    {
        return true; // not a relevant mesh
    }

    RE::NiPointer<RE::NiNode> nodePtr = getNextNodeFromBank(bankType);

    if (nodePtr) {
        if (removeFakeGlowOrbs)
            glowOrbRemover(a_root.get());
        a_root->AttachChild(nodePtr.get());
        logger::info("Attached '{}' node to '{}'", bankType, a_root->name.c_str());
        return true;
    }
    else
    {
        logger::warn("handleSceneRootByPath: Attach target or nodePtr was null for '{}'", bankType);
        return true;
    }
}
// some nodes are called dummy this is to take care of them.
inline void dummyHandler(RE::NiNode* root, std::string nodeName)
{
    // Only operate on nodes whose own name contains "dummy"

    if (nodeName.find("dummy") == std::string::npos)
        return;

    if (removeFakeGlowOrbs)
        glowOrbRemover(root);

    // Search children for a NiNode whose name contains "candle"
    for (auto& child : root->children) {
        if (!child) continue;

        auto childAsNode = child->AsNode();
        if (!childAsNode) {
            logger::info("dummy handler: child of dummy node could not be cast AsNode()");
            continue;
        }

        std::string childName = childAsNode->name.c_str();
        toLower(childName);

        if (childName.find("chandel") != std::string::npos) { // skyrim spells chandelier wrong sometimes so "chandel" (for example sometimes chandelier has 2 'L's in its name, thanks bethesda)
            RE::NiPointer<RE::NiNode> nodePtr = getNextNodeFromBank("chandel");
            if (!nodePtr) {
                logger::info("DummyHandler: chandelier node from bank was null");
                return;
            }
            root->AttachChild(nodePtr.get());
            return;
        }

        if (childName.find("ruins_floorcandlelampmid") != std::string::npos) {
            RE::NiPointer<RE::NiNode> nodePtr = getNextNodeFromBank("ruinsfloorcandlelampmidon");
            if (!nodePtr) {
                logger::info("DummyHandler: ruinsfloorcandlelampmidon node from bank was null");
                return;
            }
            root->AttachChild(nodePtr.get());
            return;
        }

        if (childName.find("candle") != std::string::npos) {
            RE::NiPointer<RE::NiNode> nodePtr = getNextNodeFromBank("candle");
            if (!nodePtr) {
                logger::info("DummyHandler: candle node from bank was null");
                return;
            }
            root->AttachChild(nodePtr.get());
            return;
        }

    }
}

inline void DumpFullTree(RE::NiAVObject* obj, int depth = 0)
{
    if (!obj) return;

    std::string indent(depth * 2, ' ');

    logger::info("{}- {} [{}]", indent, obj->name.c_str(), obj->GetRTTI() ? obj->GetRTTI()->name : "unknown");

    // if geometry, dump alpha + shader via GEOMETRY_RUNTIME_DATA
    if (auto geom = obj->AsGeometry()) {
        auto& runtime = geom->GetGeometryRuntimeData();
        if (runtime.properties[RE::BSGeometry::States::kProperty]) {
            logger::info("{}  * alphaProperty present", indent);
        }
        if (runtime.properties[RE::BSGeometry::States::kEffect]) {
            logger::info("{}  * shaderProperty present", indent);
        }
    }

    // recurse if node
    if (auto node = obj->AsNode()) {
        for (auto& child : node->children) {
            DumpFullTree(child.get(), depth + 1);
        }
    }
}
