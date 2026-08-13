#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <fstream>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <sstream>
#include <iostream>
#include <xbyak/xbyak.h>
#include <ClibUtil/EditorID.hpp>

#include "global.h"
#include "LightData.h"


namespace logger = SKSE::log;

inline void isPlayerInInteriorCell(){

	auto* player = RE::PlayerCharacter::GetSingleton();
if (!player) {
	logger::warn("PlayerCharacter singleton is null, cannot initialize lastCellWasInterior");
	return; // or set default: lastCellWasInterior = false;
}

auto* playerCell = player->GetParentCell();
if (!playerCell) {
	logger::warn("Player's parent cell is null, cannot initialize lastCellWasInterior");
	return; // or set default: lastCellWasInterior = false;
}

globals::lastCellWasInterior = playerCell->IsInteriorCell();
logger::info("Initialized lastCellWasInterior to {}", globals::lastCellWasInterior);
}

inline std::string removePrefix(const std::string& str, const std::string& prefix)
{
	if (str.size() >= prefix.size() &&
		str.compare(0, prefix.size(), prefix) == 0) {
		return str.substr(prefix.size());
	}
	return str;
}

//mutable
inline void toLower(std::string& str) {
	for (auto& c : str) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
}

//immumutable
inline std::string toLowerImmut(std::string str) {
	for (auto& c : str) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return str;
}

inline std::string trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t");
	size_t end = s.find_last_not_of(" \t");
	return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// used for multi light menu names in attach light section of menu
inline std::string StripTrailingIdentifier(std::string name)
{
	// strip " [number]"
	auto openBracket = name.rfind(" [");

	if (openBracket != std::string::npos &&
		name.back() == ']')
	{
		bool valid = true;

		for (size_t i = openBracket + 2; i < name.size() - 1; i++) {
			if (!std::isdigit(static_cast<unsigned char>(name[i]))) {
				valid = false;
				break;
			}
		}

		if (valid) {
			name.erase(openBracket);
		}
	}

	return name;
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

inline void glowOrbRemoverImpl(RE::NiNode* node)
{
	if (!node)
		return;

	std::vector<RE::NiAVObject*> childrenCopy;
	childrenCopy.reserve(node->GetChildren().size());
	for (auto& c : node->GetChildren())
		childrenCopy.push_back(c.get());

	for (auto& child : childrenCopy) {
		if (!child)
			continue;

		auto childAsNode = child->AsNode();
		if (!childAsNode)
			continue;

		std::string name = childAsNode->name.c_str();
		toLower(name);

		if (name.find("glow") != std::string::npos) {
			childAsNode->SetAppCulled(true);
			continue;
		}

		glowOrbRemoverImpl(childAsNode);
	}
}

inline void glowOrbRemover(RE::NiNode* node)
{
	if (!node)
		return;

	std::string name = node->name.c_str();
	toLower(name);

	if (name.find("tel") != std::string::npos)
		return;

	auto user = node->GetUserData();
	if (!user)
		return;

	auto* baseObj = user->GetBaseObject();
	if (skyrim_cast<RE::TESFlora*>(baseObj)) {
		logger::info("Flora detected, not removing glow");
		return;
	}

	glowOrbRemoverImpl(node);
}

inline const std::string findPriorityMatch(const std::string& meshName)
{
	for (const auto& meshNameInPriorityList : globals::priorityList) {
		if (meshNameInPriorityList.empty()) {
			continue;
		}

		if (meshName.contains(meshNameInPriorityList)) {
			return meshNameInPriorityList;
		}
	}

	return "";
}

//gets users skyrim pref setting. lights reinitialized are set to this exact distance
inline void getObjectFadeMult() {

	if (auto* setting = RE::GetINISetting("fLODFadeOutMultObjects:LOD")) {
		if (setting->GetType() == RE::Setting::Type::kFloat) {
			globals::fLODFadeOutMultObjects = setting->GetFloat() * 1000;
		}
	}
}

inline RE::NiAVObject* FindObjectByNameRecursive(RE::NiAVObject* root, std::string_view targetName)
{
	if (!root) {
		return nullptr;
	}

	auto name = std::string_view(root->name.c_str());
	if (name == targetName) {
		return root;
	}

	auto* asNode = root->AsNode();
	if (!asNode) {
		return nullptr;
	}

	for (const auto& child : asNode->GetChildren()) {
		if (!child) {
			continue;
		}

		if (auto* found = FindObjectByNameRecursive(child.get(), targetName)) {
			return found;
		}
	}

	return nullptr;
}

// this was made for debugging 
/*inline void DumpFullTree(RE::NiAVObject* obj, int depth = 0)
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
}*/


inline void hasInverseSquareLighting()
{
	const auto path =
		std::filesystem::path("Data/Shaders/InverseSquareLighting/InverseSquareLighting.hlsli");


	globals::islInstalled = std::filesystem::exists(path);

	logger::info("info isl found?: {}", globals::islInstalled);
}

inline void hasNativeMeshLightFlickerFix()
{
	const auto path =
		std::filesystem::path("Data/SKSE/Plugins/NativeMeshLightFlickerFix.ini");


	globals::isNativeLightFlickerFixInstalled = std::filesystem::exists(path);

	logger::info("Native Light FLicker Mesh Fix Installed?: {}", globals::isNativeLightFlickerFixInstalled);
}


inline std::string extractMeshName(const std::string& path) {
	auto lastSlash = path.find_last_of("/\\");
	auto filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
	auto dotPos = filename.find_last_of('.');
	return (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;
}


inline auto PackFloat = [](float z, float coverage) -> float {
	uint32_t zBits = (uint32_t)(int16_t)(int)z & 0xFFFF;
	uint32_t covBits = (uint32_t)(int16_t)(int)coverage & 0xFFFF;
	uint32_t packed = (zBits << 16) | covBits;
	float result;
	memcpy(&result, &packed, sizeof(float));
	return result;
	};

inline auto UnpackFloat = [](float packed, float& z, float& coverage) {
	uint32_t bits;
	memcpy(&bits, &packed, sizeof(float));
	z = (float)(int16_t)((bits >> 16) & 0xFFFF);
	coverage = (float)(int16_t)(bits & 0xFFFF);
	};


inline float PackFD(uint16_t gen, uint16_t idx)
{
	uint32_t packed = ((uint32_t)gen << 16) | idx;
	float f;
	memcpy(&f, &packed, sizeof(float));
	return f;
}

inline void UnpackFD(float fd, uint16_t& gen, uint16_t& idx)
{
	uint32_t packed;
	memcpy(&packed, &fd, sizeof(float));
	gen = (uint16_t)(packed >> 16);
	idx = (uint16_t)(packed & 0xFFFF);
}

inline std::string BuildConfigPath(const std::string& fileName)
{
	if (fileName.empty()) {
		return "";
	}

	std::string safeName = fileName;

	std::replace(safeName.begin(), safeName.end(), '~', '_');
	std::replace(safeName.begin(), safeName.end(), ':', '_');
	std::replace(safeName.begin(), safeName.end(), '\\', '_');
	std::replace(safeName.begin(), safeName.end(), '/', '_');

	return "Data/SKSE/Plugins/ReLight/Configs/" + safeName + ".json";
}

// used in attach light menu when selecting "attach another light" option
inline LightConfig FindRefIDConfigForAttachAnother(RE::TESObjectREFR* selected)
{

	LightConfig cfg = {};

	if (!selected) {
		return cfg;
	}

	RE::FormID formID = selected->GetFormID();

	if (auto it = LightData::refFormIDToJsonCfg.find(formID);
		it != LightData::refFormIDToJsonCfg.end() && !it->second.empty()) {
		logger::info("FindSourceConfigForAttachAnother: found ref config for {:08X}", formID);
		return it->second[0];
	}

	if (auto it = LightData::refFormIDToJsonCfgExteriors.find(formID);
		it != LightData::refFormIDToJsonCfgExteriors.end() && !it->second.empty()) {
		logger::info("FindSourceConfigForAttachAnother: found exterior ref config for {:08X}", formID);
		return it->second[0];
	}

	logger::warn("FindSourceConfigForAttachAnother: no config found for {:08X}", formID);
	return cfg;
}

// must update ref root transforms after changing position of a ni node
inline void UpdateRefRootTransforms(RE::TESObjectREFR* selected)
{
	if (!selected) {
		return;
	}

	RE::NiUpdateData updateData{};
	updateData.time = 0.0f;
	updateData.flags = RE::NiUpdateData::Flag::kDirty;

	auto a_root = selected->Get3D();
	if (!a_root) {
		return;
	}

	a_root->UpdateTransformAndBounds(updateData);
}

