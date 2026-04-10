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
#include <xbyak/xbyak.h>

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

	enum Section
	{
		NONE,
		lightEdid,
		meshPathExact,
		meshPathPartial,
		priority,
		refid
	} section = NONE;

	while (std::getline(iniFile, line))
	{
		line = trim(line);
		if (line.empty())
			continue;

		if (line.starts_with(";"))
		{
		

			if (line.find("exclude by light editor id") != std::string::npos)
				section = lightEdid;
			else if (line.find("exclude specific mesh paths") != std::string::npos)
				section = meshPathExact;
			else if (line.find("exclude partial mesh paths") != std::string::npos)
				section = meshPathPartial;
			else if (line.find("priority") != std::string::npos)
				section = priority;
			else if (line.find("exclude by ref form id") != std::string::npos)
				section = refid;
			//else
			//	section = NONE;

			continue;
		}

		switch (section)
		{
		case lightEdid:
			toLower(line);
			globals::keywordLightGroups.push_back(line);
			logger::info("Added light editorID Exclusion: {}", line);
			continue;

		case meshPathExact:
			toLower(line);
			globals::meshPathExclusionList.push_back(line);
			logger::info("Added exact mesh path exclude: {}", line);
			continue;

		case meshPathPartial:
			toLower(line);
			globals::meshPathExclusionListPartialMatch.push_back(line);
			logger::info("Added partial mesh path exclude: {}", line);
			continue;


		case priority:
		
			if (!line.starts_with("0") && !line.starts_with("[")) {
				toLower(line);
				globals::priorityList.push_back(line);
				logger::info("Added priority node: {}", line);
				continue;
			}
		

		case refid:
		{
			auto tildePos = line.find('~');
			if (tildePos != std::string::npos) {
				std::string formIDStr = trim(line.substr(0, tildePos));
				std::string modName = trim(line.substr(tildePos + 1));

				try {
					if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
						formIDStr = formIDStr.substr(2);
					}

					RE::FormID parsedID = std::stoul(formIDStr, nullptr, 16);

					auto dataHandler = RE::TESDataHandler::GetSingleton();
					auto mod = dataHandler ? dataHandler->LookupModByName(modName) : nullptr;

					if (mod && mod->IsLight()) {
						auto ref = dataHandler->LookupForm<RE::TESObjectREFR>(parsedID, modName);

						if (!ref) {
							logger::warn(
								"Failed to resolve light plugin ref localID 0x{:X} from mod {}",
								static_cast<std::uint32_t>(parsedID),
								modName);
							continue;
						}

						globals::excludedRefFormIDs.insert(ref->GetFormID());

						logger::info(
							"Added excluded light plugin ref runtime formID: 0x{:08X} from {} (local: 0x{:X})",
							static_cast<std::uint32_t>(ref->GetFormID()),
							modName,
							static_cast<std::uint32_t>(parsedID));
					}
					else {
						globals::excludedRefFormIDs.insert(parsedID);

						logger::info(
							"Added excluded non-light runtime ref formID: 0x{:08X} from {}",
							static_cast<std::uint32_t>(parsedID),
							modName);
					}
				}
				catch (...) {
					logger::warn("Failed to parse excluded ref entry: {}", line);
				}

				continue;
			}

			try {
				std::string formIDStr = trim(line);

				//lines added by the in game menu apply this header that should be skipped to prevent error
				if (line.starts_with("[")) continue; 

				if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
					formIDStr = formIDStr.substr(2);
				}

				RE::FormID runtimeID = std::stoul(formIDStr, nullptr, 16);
				globals::excludedRefFormIDs.insert(runtimeID);

				logger::info(
					"Added excluded runtime ref formID: 0x{:08X}",
					static_cast<std::uint32_t>(runtimeID));
			}
			catch (...) {
				logger::warn("Failed to parse excluded runtime ref formID: {}", line);
			}

			continue;
		}
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

		if (key == "removefakegloworbs") {
			globals::removeFakeGlowOrbs = value == "true";
			continue;
		}

		if (key == "enablelightflickerprevention") {
			globals::enableLightFlickerPreventionMeasures = value == "true";
			continue; 
		}

		if (key == "enabledebugbulbs") {
			globals::enableDebugLightBulbs= value == "true";
			continue;
		}

		if (key == "whitelist") {
			splitString(value, ',', globals::whitelist);
			continue;
		}

		if (key == "logginglevel") {
			globals::loggingLevel = std::clamp(std::stoi(value), 0, 3);
			logger::info("Logging level set to {}", globals::loggingLevel);

			spdlog::level::level_enum lvl = spdlog::level::info;
			if (globals::loggingLevel == 0) lvl = spdlog::level::critical;
			else if (globals::loggingLevel == 1) lvl = spdlog::level::warn;
			else if (globals::loggingLevel == 3) lvl = spdlog::level::debug;

			spdlog::set_level(lvl);
			spdlog::flush_on(lvl);
			continue;
		}

		if (key == "light merge distance") {
			globals::lightMergeDistance = std::stof(value);
			continue;
		}

		if (key == "shadow light merge distance") {
			globals::shadowLightMergeDistance = std::stof(value);
			continue;
		}

		if (key == "light merge distance increased") {
			globals::lightMergeSeekingDistance = std::stof(value);
			continue;
		}

		if (key == "max z diff to merge") {
			globals::fMaxZDiffToMerge = std::stof(value);
			continue;
		}

		if (key == "max z diff to merge increased") {
			globals::fMaxZDiffToMergeIncreased = std::stof(value);
			continue;
		}

		if (key == "light fade per merge") {
			globals::lightFadePerMerge = std::stof(value);
			continue;
		}

		if (key == "light radius per merge") {
			globals::lightRadiusPerMerge = std::stof(value);
			continue;
		}

		if (key == "light fade max") {
			globals::lightFadeMax = std::stof(value);
			continue;
		}

		if (key == "light radius max") {
			globals::lightRadiusMax = std::stof(value);
			continue;
		}

		if (key == "light merge maxlights") {
			globals::lightMergeMaxLights = std::stoi(value);
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

//removes unsightly glow orbs from meshes
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

inline bool isLightPluginFormID(RE::FormID formID)
{
	return (formID & 0xFF000000) == 0xFE000000;
}

inline std::uint32_t getLightPluginLocalFormID(RE::FormID formID)
{
	return formID & 0x00000FFF;
}

inline std::uint32_t getLocalFormID(RE::FormID formID)
{
	if (isLightPluginFormID(formID)) {
		return formID & 0x00000FFF;  // ESL/light plugin
	}

	return formID & 0x00FFFFFF;      // normal plugin
}

inline bool isExcludedRef(const RE::TESObjectREFR* ref)
{
	if (!ref) {
		return false;
	}

	const RE::FormID runtimeFormID = ref->GetFormID();

	//logger::info("runtimeFormID = 0x{:08X}", static_cast<std::uint32_t>(runtimeFormID));

	// old behavior still works
	if (globals::excludedRefFormIDs.contains(runtimeFormID)) {
		logger::debug("excluded ref runtime formID 0x{:08X} skipping light attachment",
			static_cast<std::uint32_t>(runtimeFormID));
		return true;
	}

	return false;
}

/*inline bool isNodeExclude(const RE::BSFixedString& nodeName, RE::TESObjectREFR* ref)
{

	// Exact matches in exclusion list
	for (const auto& exclude : globals::nodeNameExclusionList) {
		if (nodeName == exclude) {
			logger::debug("isExclude: '{}' matched exact exclude '{}' skipping light attachment", nodeName.c_str(), exclude);
			return true;
		}
		
	}

	// Partial matches in exclusion list
	for (const auto& exclude : globals::nodeNameExclusionListPartialMatch) {
		if (nodeName.contains(exclude)) {
			logger::debug("isExclude: '{}' matched partial exclude '{}' skipping light attachment", nodeName.c_str(), exclude);
			return true;
		}
			
	}

	if (isExcludedRef(ref)) {
		return true;
	}
	

	return false;
}*/


inline bool isExclude(const std::string& meshPath, RE::TESObjectREFR* ref)
{
	// Exact matches in exclusion list
	for (const auto& exclude : globals::meshPathExclusionList) {
		if (meshPath == exclude) {
			logger::debug("isExclude: '{}' matched exact mesh exclude '{}' skipping light attachment", meshPath, exclude);
			return true;
		}
	}

	// Partial matches in exclusion list
	for (const auto& exclude : globals::meshPathExclusionListPartialMatch) {
		if (meshPath.contains(exclude)) {
			logger::debug("isExclude: '{}' matched partial mesh exclude '{}' skipping light attachment", meshPath, exclude);
			return true;
		}
	}

	if (isExcludedRef(ref)) {
		return true;
	}

	return false;
}

inline const std::string findPriorityMatch(const std::string& meshName)
{
	for (const auto& meshNameInPriorityList : globals::priorityList) {
		if (meshName.contains(meshNameInPriorityList)) {
			//logger::info("priority list item{} ", nodeNameInPriorityList);
			return meshNameInPriorityList;
		}
	}

	return ""; // safe, reference exists
}

//gets users skyrim pref setting. lights reinitialized are set to this exact distance
inline void getObjectFadeMult() {

	if (auto* setting = RE::GetINISetting("fLODFadeOutMultObjects:LOD")) {
		if (setting->GetType() == RE::Setting::Type::kFloat) {
			globals::fLODFadeOutMultObjects = setting->GetFloat() * 1000;
		}
	}
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


inline bool HasRelightLight(RE::TESObjectREFR* ref)
{
	if (!ref)
		return false;

	auto* root = ref->Get3D();
	if (!root)
		return false;

	auto* node = netimmerse_cast<RE::NiNode*>(root);
	if (!node)
		return false;

	for (auto& child : node->GetChildren()) {
		if (!child)
			continue;

		std::string_view name(child->name.c_str());
		if (name.size() < 2 || name[0] != 'R' || name[1] != 'L')
			continue;

		if (netimmerse_cast<RE::NiPointLight*>(child.get())) {
			return true;
		}
	}

	return false;
}

inline std::string extractMeshName(const std::string& path) {
	auto lastSlash = path.find_last_of("/\\");
	auto filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
	auto dotPos = filename.find_last_of('.');
	return (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;
}


inline bool TESRayHitStatic(RE::bhkWorld* world, RE::NiPoint3 start, RE::NiPoint3 end)
{
	RE::bhkPickData pickData{};
	const float scale = RE::bhkWorld::GetWorldScale();
	pickData.rayInput.from = start * scale;
	pickData.rayInput.to = end * scale;
	pickData.rayInput.enableShapeCollectionFilter = true;
	RE::CFilter filter{};
	filter.SetCollisionLayer(RE::COL_LAYER::kLOS);
	static const std::uint32_t sSystemGroup =
		RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup();
	filter.SetSystemGroup(sSystemGroup);
	pickData.rayInput.filterInfo = filter;
	world->PickObject(pickData);
	if (!pickData.rayOutput.HasHit())
		return false;
	auto* collidable = pickData.rayOutput.rootCollidable;
	if (!collidable)
		return false;
	auto layer = collidable->GetCollisionLayer();
	if (layer != RE::COL_LAYER::kStatic &&
		layer != RE::COL_LAYER::kTerrain &&
		layer != RE::COL_LAYER::kGround)
		return false;
	auto* niObj = RE::TES::GetSingleton()->Pick(pickData);
	if (!niObj || niObj->name.empty())
		return false;
	logger::debug("TES::Pick hit node: {}", niObj->name.c_str());
	return (niObj->name.contains("wall") || niObj->name.contains("floor") || niObj->name.contains("intcor") || niObj->name.contains("farmintinnend") ) &&
		!niObj->name.contains("shelf");
}

inline bool HasAnythingBetween(RE::NiPoint3 start, RE::NiPoint3 end)
{
	auto player = RE::PlayerCharacter::GetSingleton(); 

	if (!player) return false;

	auto* cell = player->GetParentCell();
	if (!cell) return false;
	auto* world = cell->GetbhkWorld();
	if (!world) return false;

	bool hitLow = TESRayHitStatic(world, start + RE::NiPoint3(0, 0, 35.0f), end + RE::NiPoint3(0, 0, 35.0f));
	bool hitMid = TESRayHitStatic(world, start + RE::NiPoint3(0, 0, 70.0f), end + RE::NiPoint3(0, 0, 70.0f));
	bool hitHigh = TESRayHitStatic(world, start + RE::NiPoint3(0, 0, 105.0f), end + RE::NiPoint3(0, 0, 105.0f));

	logger::debug("Ray results: low {} mid {} high {}", hitLow, hitMid, hitHigh);
	return hitLow && hitMid && hitHigh;
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

inline std::string BuildRefIDAndModName(RE::TESObjectREFR* ref)
{
	if (!ref) {
		return "";
	}

	const RE::TESFile* refOriginFile = ref->GetDescriptionOwnerFile();
	std::string modName = refOriginFile ? refOriginFile->fileName : "";

	if (modName.empty()) {
		logger::warn("BuildRefIDAndModName: ref {:08X} has no owning file", ref->GetFormID());
		return "";
	}

	const auto runtimeID = ref->GetFormID();

	if (refOriginFile->IsLight()) {
		// Best option if available in your CommonLib version
		const auto localID = ref->GetLocalFormID();

		logger::info(
			"Built light plugin ref string: 0x{:X}~{} (runtime: 0x{:08X})",
			static_cast<std::uint32_t>(localID),
			modName,
			static_cast<std::uint32_t>(runtimeID));

		return std::format("0x{:X}~{}", static_cast<std::uint32_t>(localID), modName);
	}

	logger::info(
		"Built non-light plugin ref string: 0x{:X}~{}",
		static_cast<std::uint32_t>(runtimeID),
		modName);

	return std::format("0x{:X}~{}", static_cast<std::uint32_t>(runtimeID), modName);
}


inline std::string BuildConfigPath(const std::string& refIDStr)
{
	if (refIDStr.empty()) {
		return "";
	}

	std::string safeName = refIDStr;

	// Replace characters that are annoying for file paths
	std::replace(safeName.begin(), safeName.end(), '~', '_');
	std::replace(safeName.begin(), safeName.end(), ':', '_');
	std::replace(safeName.begin(), safeName.end(), '\\', '_');
	std::replace(safeName.begin(), safeName.end(), '/', '_');

	return "Data/SKSE/Plugins/ReLight/Configs/" + safeName + ".json";
}

inline bool AppendMenuExcludedRefToINI(const std::string& iniPath, const std::string& refIDAndModName)
{
	if (iniPath.empty()) {
		logger::error("AppendMenuExcludedRefToINI: iniPath was empty");
		return false;
	}

	if (refIDAndModName.empty()) {
		logger::error("AppendMenuExcludedRefToINI: refIDAndModName was empty");
		return false;
	}

	try {
		std::ifstream inFile(iniPath);
		if (!inFile.is_open()) {
			logger::error("AppendMenuExcludedRefToINI: failed to open {}", iniPath);
			return false;
		}

		std::vector<std::string> lines;
		std::string line;
		while (std::getline(inFile, line)) {
			lines.push_back(line);
		}
		inFile.close();

		const std::string targetSection = "[Refs Excluded Using in Game Menu]";
		const std::string legacyComment = "; exclude refs from receiving relights";

		bool inTargetSection = false;
		bool sectionFound = false;
		bool alreadyExists = false;
		std::size_t insertPos = lines.size();
		std::size_t legacyInsertPos = lines.size();
		bool legacyBlockFound = false;

		for (std::size_t i = 0; i < lines.size(); ++i) {
			std::string trimmed = trim(lines[i]);

			if (trimmed == targetSection) {
				inTargetSection = true;
				sectionFound = true;
				insertPos = i + 1;
				continue;
			}

			if (!sectionFound && trimmed == legacyComment) {
				legacyBlockFound = true;
				legacyInsertPos = i + 1;
				continue;
			}

			if (inTargetSection) {
				if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
					insertPos = i;
					break;
				}

				if (trimmed.empty() || trimmed.starts_with(";")) {
					continue;
				}

				if (trimmed == refIDAndModName) {
					alreadyExists = true;
					break;
				}

				insertPos = i + 1;
			}
			else if (legacyBlockFound) {
				if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
					legacyInsertPos = i;
					legacyBlockFound = false;
					continue;
				}

				if (!trimmed.empty() && !trimmed.starts_with(";")) {
					legacyInsertPos = i + 1;
				}
			}
		}

		if (alreadyExists) {
			logger::info("AppendMenuExcludedRefToINI: entry already exists: {}", refIDAndModName);
			return true;
		}

		if (sectionFound) {
			lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(insertPos), refIDAndModName);
		}
		else {
			std::size_t sectionPos = lines.size();

			if (legacyInsertPos < lines.size()) {
				sectionPos = legacyInsertPos;
			}
			else if (!lines.empty() && !lines.back().empty()) {
				lines.push_back("");
			}

			if (sectionPos != lines.size()) {
				lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionPos), "");
				++sectionPos;
				lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionPos), targetSection);
				++sectionPos;
				lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionPos), refIDAndModName);
			}
			else {
				lines.push_back(targetSection);
				lines.push_back(refIDAndModName);
			}
		}

		std::ofstream outFile(iniPath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("AppendMenuExcludedRefToINI: failed to write {}", iniPath);
			return false;
		}

		for (const auto& outLine : lines) {
			outFile << outLine << '\n';
		}

		logger::info("AppendMenuExcludedRefToINI: added {}", refIDAndModName);
		return true;
	}
	catch (const std::exception& e) {
		logger::error("AppendMenuExcludedRefToINI failed: {}", e.what());
		return false;
	}
}


inline bool RemoveMenuExcludedRefFromINI(const std::string& iniPath, const std::string& refIDAndModName)
{
	if (iniPath.empty()) {
		logger::error("RemoveMenuExcludedRefFromINI: iniPath was empty");
		return false;
	}

	if (refIDAndModName.empty()) {
		logger::error("RemoveMenuExcludedRefFromINI: refIDAndModName was empty");
		return false;
	}

	try {
		std::ifstream inFile(iniPath);
		if (!inFile.is_open()) {
			logger::error("RemoveMenuExcludedRefFromINI: failed to open {}", iniPath);
			return false;
		}

		std::vector<std::string> lines;
		std::string line;
		while (std::getline(inFile, line)) {
			lines.push_back(line);
		}
		inFile.close();

		const std::string targetSection = "[Refs Excluded Using in Game Menu]";

		bool inTargetSection = false;
		bool sectionFound = false;
		bool removed = false;

		std::vector<std::string> output;
		output.reserve(lines.size());

		for (std::size_t i = 0; i < lines.size(); ++i) {
			std::string trimmed = trim(lines[i]);

			if (trimmed == targetSection) {
				inTargetSection = true;
				sectionFound = true;
				output.push_back(lines[i]);
				continue;
			}

			if (inTargetSection) {
				if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
					inTargetSection = false;
					output.push_back(lines[i]);
					continue;
				}

				if (trimmed == refIDAndModName) {
					removed = true;
					logger::info("RemoveMenuExcludedRefFromINI: removed {}", refIDAndModName);
					continue;
				}
			}

			output.push_back(lines[i]);
		}

		if (!sectionFound) {
			logger::info("RemoveMenuExcludedRefFromINI: section not found, nothing to remove");
			return true;
		}

		if (!removed) {
			logger::info("RemoveMenuExcludedRefFromINI: entry not present: {}", refIDAndModName);
			return true;
		}

		std::ofstream outFile(iniPath, std::ios::trunc);
		if (!outFile.is_open()) {
			logger::error("RemoveMenuExcludedRefFromINI: failed to write {}", iniPath);
			return false;
		}

		for (const auto& outLine : output) {
			outFile << outLine << '\n';
		}

		return true;
	}
	catch (const std::exception& e) {
		logger::error("RemoveMenuExcludedRefFromINI failed: {}", e.what());
		return false;
	}
}

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