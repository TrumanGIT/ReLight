#pragma once
#include <spdlog/sinks/basic_file_sink.h>
#include "ClibUtil/EditorID.hpp"
#include "LightData.h"
#include "Hooks.h"
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

// TODO:: I may delete this or repurpose this. 
inline RE::NiPointer<RE::NiAVObject> CloneNiPointLight(RE::NiPointLight* NiPointLight) {

    RE::NiCloningProcess cloningProcess;
    auto NiPointLightClone = NiPointLight->CreateClone(cloningProcess);
    if (!NiPointLightClone) {
        logger::error("Failed to clone NiNode");
        return nullptr;
    }

    auto NiPointLightCloneAsAv = static_cast<RE::NiAVObject*>(NiPointLightClone);

    // Successfully cloned node
    return RE::NiPointer<RE::NiAVObject>(NiPointLightCloneAsAv);
}

inline void glowOrbRemover(RE::NiNode* node)
{
    if (!node)
        return;

    // Copy raw pointers to avoid iterator invalidation
    std::vector<RE::NiAVObject*> childrenCopy;
    childrenCopy.reserve(node->GetChildren().size());

    for (auto& c : node->GetChildren()) {
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

// TODO:: Rework well probobly move this to ini file since were done with masterlist
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
                        logger::info("Culled CandleGlow01 MPS emitter ");
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

//TODO:: reimplement for new ni pointLightBank ban, we need to find a way to have priority for nodes coming in so
// so chandeliers overtake candles ect (some chandeliers have candle in node name) 

std::string findPriorityMatch(const std::string& nodeName)
{
    //  Check priority list created from ini file first
    for (auto& nodeNameInPriorityList : priorityList) {
        if (nodeName.find(nodeNameInPriorityList) != std::string::npos)
            return nodeName;
    }

    // No priority specified
    for (auto& pair : niPointLightNodeBank) {
        const auto& jsonCfg = pair.first;

        if (nodeName.find(jsonCfg.nodeName) != std::string::npos)
            return jsonCfg.nodeName;
    }

    return ""; // no match
}

//we clone and store NIpointLight nodes in bank 
inline RE::NiPointer<RE::NiAVObject> getNextNodeFromBank(const std::string& nodeName)
{
    for (auto& [cfg, bank] : niPointLightNodeBank) {

        // Check if this config applies to the requested nodeName
        if (nodeName.find(cfg.nodeName) == std::string::npos)
            continue;

        if (bank.empty()) {
            logger::warn("getNextNodeFromBank: '{}' has no nodes available", nodeName);
            return nullptr;
        }

        static std::unordered_map<std::string, std::size_t> counters;
        auto& count = counters[nodeName];

        if (count >= bank.size())
            count = 0;

        RE::NiPointer<RE::NiAVObject> obj = bank[count];
        if (!obj) {
            logger::warn("getNextNodeFromBank: '{}' index {} is null", nodeName, count);
            return nullptr;
        }

        count++;
        return obj;
    }

    logger::warn("getNextNodeFromBank: '{}' no matching LightConfig found", nodeName);
    return nullptr;
}



//TO DO:: change to use ni point lights
// torches need special placement of light so they dont light up when not equipped. 
inline bool TorchHandler(const std::string& nodeName, RE::NiPointer<RE::NiNode>& a_root)
{
    if (nodeName == "torch") {
        RE::NiNode* attachLight = nullptr;
        RE::NiNode* torchFire = nullptr;

        // must null check everything or crash city. 

        for (auto& child : a_root->GetChildren()) {
            if (!child) continue; // 
            auto childNode = child->AsNode();
            if (childNode && childNode->name == "TorchFire") {
                torchFire = childNode;
                break;
            }
        }

        if (torchFire) {
            for (auto& child : torchFire->GetChildren()) {
                if (!child) continue;
                auto childNode = child->AsNode();
                if (childNode && childNode->name == "AttachLight") {
                    attachLight = childNode;
                    break;
                }
            }
        }

        if (attachLight) {
            RE::NiPointer<RE::NiAVObject> nodePtr = getNextNodeFromBank("torch");
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

bool IsNordicHallMesh(const std::string& nodeName)
{
    return nordicHallMeshes.contains(nodeName);
}

inline bool applyCorrectNordicHallTemplate(std::string nodeName, RE::NiPointer<RE::NiNode>& a_root)
{
    if (!IsNordicHallMesh(nodeName))
        return false;

    //TODO:: we need a way to apply multiple lights to nordic hall meshes
//before we iterated through each node in a template wired gave us, now we only have single json objects, we will have to think of something

    return true;
}

// some nodes are called scene root this is to take care of them. 
inline bool handleSceneRoot(const char* nifPath, RE::NiPointer<RE::NiNode>& a_root, const std::string& nodeName)
{
    // Skip if nodeName does not contain "scene"
    if (nodeName.find("scene") == std::string::npos)
        return false;

    if (!nifPath)
        return true;

    std::string path = nifPath;
    toLower(path);

    logger::info("scene root node detected, checking path: {}", path);

    // Determine bankType based on path
    std::string bankType;
    if (path.find("candlehornfloor") != std::string::npos || path.find("mwcandle01") != std::string::npos) {
        bankType = "candlehornfloor01";
    }
    else if (path.find("candle") != std::string::npos) {
        logger::info("handleSceneRootByPath: matched candlehorntable/wall or mwcandle01 in path");
        bankType = "candle";
    }
    else {
        return true; // not a relevant mesh
    }

    // Get the next node from the bank
    if (auto nodePtr = getNextNodeFromBank(bankType); nodePtr) {
        if (removeFakeGlowOrbs)
            glowOrbRemover(a_root.get());

        a_root->AttachChild(nodePtr.get());
        logger::info("Attached '{}' node to '{}'", bankType, a_root->name.c_str());
    }
    else {
        logger::warn("handleSceneRootByPath: Attach target or nodePtr was null for '{}'", bankType);
    }

    return true;
}

// some nodes are called dummy this is to take care of them.
inline void dummyHandler(RE::NiNode* root, const std::string& nodeName)
{
    if (nodeName.find("dummy") == std::string::npos)
        return;

    if (removeFakeGlowOrbs)
        glowOrbRemover(root);

    for (auto& child : root->GetChildren()) {
        if (!child) continue;

        auto childAsNode = child->AsNode();
        if (!childAsNode) {
            logger::info("dummy handler: child of dummy node could not be cast AsNode()");
            continue;
        }

        std::string childName = childAsNode->name.c_str();
        toLower(childName);

        for (auto& [substr, bankType] : childBankMap) {
            if (childName.find(substr) != std::string::npos) {
                if (auto nodePtr = getNextNodeFromBank(bankType); nodePtr) {
                    root->AttachChild(nodePtr.get());
                }
                else {
                    logger::info("DummyHandler: '{}' node from bank was null", bankType);
                }
                return; // stop after first match
            }
        }
    }
}

// this was made for debugging 
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
        for (auto& child : node->GetChildren()) {
            DumpFullTree(child.get(), depth + 1);
        }
    }
}
