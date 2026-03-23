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
	enum Section { NONE, exact, partial, priority, refid } section = NONE;

	while (std::getline(iniFile, line))
	{
		line = trim(line);
		if (line.empty())
			continue;

		if (line.starts_with(";"))
		{
			toLower(line);

			if (line.find("exclude specific nodes") != std::string::npos)
				section = exact;
			else if (line.find("exclude partial nodes") != std::string::npos)
				section = partial;
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
			if (!line.starts_with("0")) {
				globals::priorityList.push_back(line);
				logger::info("Added priority node: {}", line);
				continue;
			}
		

		case refid:
		{
			try {
				RE::FormID id =
					static_cast<RE::FormID>(std::stoul(line, nullptr, 0));

				globals::excludedRefFormIDs.insert(id);
				logger::info("Added excluded ref FormID: {:08X}", id);
			}
			catch (...) {
				logger::warn("Invalid FormID in exclude-by-ref section: {}", line);
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

inline bool isExclude(const RE::BSFixedString& nodeName, RE::FormID refFormID)
{

	// Exact matches in exclusion list
	for (const auto& exclude : globals::exclusionList) {
		if (nodeName == exclude) {
			logger::debug("isExclude: '{}' matched exact exclude '{}' skipping light attachment", nodeName.c_str(), exclude);
			return true;
		}
		
	}

	// Partial matches in exclusion list
	for (const auto& exclude : globals::exclusionListPartialMatch) {
		if (nodeName.contains(exclude)) {
			logger::debug("isExclude: '{}' matched partial exclude '{}' skipping light attachment", nodeName.c_str(), exclude);
			return true;
		}
			
	}

	if (globals::excludedRefFormIDs.contains(refFormID)) {
		logger::debug("excluded ref '{}' skipping light attachment", refFormID);
		return true;
	}
	

	return false;
}

inline const RE::BSFixedString findPriorityMatch(const RE::BSFixedString& nodeName)
{
	for (const auto& nodeNameInPriorityList : globals::priorityList) {
		if (nodeName.contains(nodeNameInPriorityList)) {
			//logger::info("priority list item{} ", nodeNameInPriorityList);
			return nodeNameInPriorityList;
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
		std::filesystem::path("Data/Shaders/InverseSquareLighting/InverseSquareLighting.hlsli");

	globals::islInstalled = std::filesystem::exists(path);

	logger::info("info isl found?: {}", globals::islInstalled);
}


template <class T, std::size_t BYTES>
inline void hook_function_prologue(std::uintptr_t a_src)
{
	struct Patch : Xbyak::CodeGenerator
	{
		Patch(std::uintptr_t a_originalFuncAddr, std::size_t a_originalByteLength)
		{
			// Hook returns here. Execute the restored bytes and jump back to the original function.
			for (size_t i = 0; i < a_originalByteLength; ++i) {
				db(*reinterpret_cast<std::uint8_t*>(a_originalFuncAddr + i));
			}

			jmp(ptr[rip]);
			dq(a_originalFuncAddr + a_originalByteLength);
		}
	};

	Patch p(a_src, BYTES);
	p.ready();

	auto& trampoline = SKSE::GetTrampoline();
	trampoline.write_branch<5>(a_src, T::thunk);

	auto alloc = trampoline.allocate(p.getSize());
	std::memcpy(alloc, p.getCode(), p.getSize());

	T::func = reinterpret_cast<std::uintptr_t>(alloc);
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
	return (niObj->name.contains("wall") || niObj->name.contains("floor") || niObj->name.contains("intcor") || niObj->name.contains("farmintend") ) &&
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

inline void ComputeClosestLights(RE::BSLight* outLights[7], RE::BSLightingShaderProperty* p)
{
	auto* pass = p->renderPassList.head;
	if (!pass || !pass->geometry)
		return;
	auto& center = pass->geometry->worldBound.center;

	const float triRadius = pass->geometry->worldBound.radius;

	auto* ssNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
	if (!ssNode)
		return;
	auto& rt = ssNode->GetRuntimeData();
	struct Candidate
	{
		RE::BSLight* light;
		float dist;
	};
	std::vector<Candidate> candidates;
	auto gatherLights = [&](auto& container)
		{
			for (auto& l : container)
			{
				if (!l) continue;
				auto* light = l.get();
				if (!light || !light->light) continue;

				auto& pos = light->light->world.translate;
				float dx = pos.x - center.x;
				float dy = pos.y - center.y;
				float dz = pos.z - center.z;
				float distXY2 = dx * dx + dy * dy + dz * dz;

				switch (light->unk060)
				{
				case 1:
				{
					float threshold = globals::minCandleCoverage;
					if (triRadius < 325) threshold = globals::minCandleCoverageSM;
					else if (triRadius > 850) threshold = globals::minCandleCoverageXL;
					if (distXY2 > threshold * threshold) continue;
					break;
				}
				case 2:
					if (distXY2 > globals::minChandelierCoverage * globals::minChandelierCoverage) continue;
					break;
				case 3:
				{
					float threshold = globals::minFireCoverage;
					if (triRadius > 850) threshold = globals::minFireCoverageXL;
					if (distXY2 > threshold * threshold) continue;
					break;
				}
				default:
					if (light->light->radius.x < 1000) {
						float threshold = triRadius > 1000 ? globals::globalCoverageXL : globals::globalCoverage;
						if (distXY2 > threshold * threshold) continue;
					}
					break;
				}

				candidates.push_back({ light, distXY2 });
			}
		};
	gatherLights(rt.activeLights);
	gatherLights(rt.activeShadowLights);
	std::sort(candidates.begin(), candidates.end(),
		[](auto& a, auto& b)
		{
			return a.dist < b.dist;
		});
	int count = std::min(7, (int)candidates.size());
	for (int i = 0; i < count; i++)
		outLights[i] = candidates[i].light;
}

inline void ResetTriLightCache()
{
	std::lock_guard lock(LightData::triLightCacheMutex);
	LightData::triLightCache.clear();
	LightData::triLightCacheGeneration.fetch_add(1);
}


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