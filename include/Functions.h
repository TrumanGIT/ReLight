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

/*inline void initialize() {
     logger::info("loading forms");
   auto dataHandler = RE::TESDataHandler::GetSingleton();
   //LoadScreenLightMain (seemingly unsued, does not come through the  light gen hook so useful as a dymmy) 
   dummyLightObject = dataHandler->LookupForm<RE::TESObjectLIGH>(0x00105300, "Skyrim.esm");
   if (!dummyLightObject) {
        logger::info("TESObjectLIGH dummyLightObject (0x00105300) not found");
    }
}*/

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
	for (auto& g : group) {
		if (ID.find(g) == std::string::npos)
			return false;
	}
	return true;
}

//TODO:: Log set values for debugging (saved me alot of flickerTime with users) 
inline void iniParser()
{
	std::string path = "Data\\SKSE\\Plugins\\ReLight.ini";

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
	enum Section { NONE, exact, partial, priority } section = NONE;

	while (std::getline(iniFile, line))
	{
		line = trim(line);
		if (line.empty()) continue;

		
		if (line.starts_with(";"))
		{
			toLower(line);

			if (line.find("exclude specific nodes") != std::string::npos)
				section = exact;
			else if (line.find("exclude partial nodes") != std::string::npos)
				section = partial;
			else if (line.find("priority") != std::string::npos)
				section = priority;
			else
				section = NONE;

			continue;
		}

		switch (section)
		{
		case exact:
			toLower(line);
			globals::exclusionList.push_back(line);
			logger::info("Added exact exclude: {}", line);
			continue;

		case partial:
			toLower(line);
			globals::exclusionListPartialMatch.push_back(line);
			logger::info("Added partial exclude: {}", line);
			continue;

		case priority:
			 toLower(line);
			 globals::priorityList.push_back(line);
			logger::info("Added priority node: {}", line);
			continue;

		default:
			break;
		}

		auto eq = line.find('=');
		if (eq == std::string::npos)
			continue;

		std::string key = trim(line.substr(0, eq));
		toLower(key); 

		std::string value = trim(line.substr(eq + 1));
		toLower(value);

		std::string vLow = value;
		toLower(vLow);

		auto parseBool = [&](const std::string& v) {
			return (v == "true" || v == "1" || v == "yes");
			};

		if (key == "disabletorchlights") {
			globals::disableTorchLights = parseBool(vLow);
			continue;
		}

		if (key == "removefakegloworbs") {
			globals::removeFakeGlowOrbs = parseBool(vLow);
			continue;
		}

		if (key == "whitelist") {
			splitString(value, ',', globals::whitelist);
			continue;
		}

		if (key == "logginglevel") {
			globals::loggingLevel = std::stoi(value);
			globals::loggingLevel = std::clamp(globals::loggingLevel, 0, 3);
			logger::info("Logging level set to {}", globals::loggingLevel);
			spdlog::level::level_enum user_level = spdlog::level::info;
			switch (globals::loggingLevel) {
				case 0:
				{
					user_level = spdlog::level::critical;
					break;
				}
				case 1:
				{
					user_level = spdlog::level::warn;
					break;
				}
				case 2:
				{
					user_level = spdlog::level::info;
					break;
				}
				case 3:
				{
					user_level = spdlog::level::debug;
					break;
				}
			}
			spdlog::set_level(user_level);
			spdlog::flush_on(user_level);
			continue;
		}
	}

	iniFile.close();

	logger::info("ReLight.ini parsed successfully!");
}

inline bool IsInSoulCairnOrApocrypha(RE::PlayerCharacter* player) {
	if (!player) {
		return false;
	}

	static RE::FormID soulCairnFormID = 0x2001408;
	static RE::FormID apocryphaFormID = 0x0401C0B2;

	auto worldspace = player->GetWorldspace();
	if (!worldspace) {
		// logger::info("worldSpace not valid cant get location");
		return false;  // Not in a worldspace (probably in an interior cell)
	}

	// logger::debug("current worldspace = {}", worldspace->GetFormID());

	if (worldspace->GetFormID() == apocryphaFormID || worldspace->GetFormID() == soulCairnFormID) {
		//  logger::info("is in soul cairn or apocrypha");
		return true;
	}

	return false;
}

inline RE::NiPointLight* cloneNiPointLight(RE::NiPointLight* niPointLight) {

	if (!niPointLight) {
		logger::warn("no ni point light to clone!");
			return nullptr;
	}

	auto cloneAsNiAv = niPointLight->Clone();
	if (!cloneAsNiAv) {
		logger::error("Failed to clone NiNode");
		return nullptr;
	}

	auto niPointLightClone = netimmerse_cast<RE::NiPointLight*>(cloneAsNiAv);

	return niPointLightClone;
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
			childAsNode->SetAppCulled(true);
			continue;
		}

		// Recursive call to handle nested nodes
		glowOrbRemover(childAsNode);
	}
}

inline bool isExclude(const std::string& nodeName, /*const char* nifPath,*/ RE::NiNode* root)
{
	if (nodeName == "mpscandleflame01.nif" && globals::removeFakeGlowOrbs) {
		if (!root)
			return true;

		// TODO:: mps glow remover doesent seem to be working. I suspect 
		// that the issue is MPS is not loaded through load3d hook like it is PostCreate() but idk
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
	for (const auto& exclude : globals::exclusionList) {
		if (nodeName == exclude)
			return true;
	}

	// Partial matches in exclusion list
	for (const auto& exclude : globals::exclusionListPartialMatch) {
		if (nodeName.find(exclude) != std::string::npos)
			return true;
	}

	/*if (!nifPath)
		return false;

	std::string path = nifPath;
	toLower(path);

	// Some modded torches name "off" variants incorrectly
	if (path.find("off") != std::string::npos)
		return true;*/

	return false;
}

inline const RE::BSFixedString& findPriorityMatch(const RE::BSFixedString& nodeName)
{
	for (const auto& nodeNameInPriorityList : globals::priorityList) {
		if (nodeName.contains(nodeNameInPriorityList))
			return nodeNameInPriorityList;
	}

	return ""; // safe, reference exists
}


//TODO:: Reimplement

inline bool applyCorrectNordicHallTemplate(std::string nodeName)
{
	static const std::unordered_set<std::string> nordicHallMeshes = {
	"norcathallsm1way01",
	"norcathallsm1way02",
	"norcathallsm1way03",
	"norcathallsm2way01",
	"norcathallsm3way01",
	"norcathallsm3way02",
	"norcathallsm4way01",
	"norcathallsm4way02",
	"nortmphallbgcolumnsm01",
	"nortmphallbgcolumnsm02",
	"nortmphallbgcolumn01",
	"nortmphallbgcolumn03"
	};

	if (!nordicHallMeshes.contains(nodeName))
		return false;
	//TODO:: we need a way to apply multiple lights to nordic hall meshes
//before we iterated through each node in a template wired gave us, now we only have single json objects, we will have to think of something

	return true;
}
// some nodes are called scene root this is to take care of them. 
/*inline bool handleSceneRoot(const char* nifPath, RE::NiPointer<RE::NiNode>& a_root, const std::string& nodeName)
{
	// Skip if nodeName does not contain "scene"
	if (nodeName.find("scene") == std::string::npos)
		return false;

	if (!nifPath)
		return true;

	std::string path = nifPath;
	toLower(path);

	logger::debug("scene root node detected, checking path: {}", path);

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
}*/

// some nodes are called dummy this is to take care of them.
/*inline void dummyHandler(RE::NiNode* root, const std::string& nodeName)
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
}*/

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

inline void hasInverseSquareLighting()
{
	const auto path =
		std::filesystem::path("Data/Shaders/Features/InverseSquareLighting.ini");

	globals::islInstalled = std::filesystem::exists(path);

	logger::info("info isl found?: {}", globals::islInstalled);
}

inline float getRandomFloat(const float& min, const float& max, uint32_t rngState)
{
	return min + (max - min) * Random::rand(rngState);
}