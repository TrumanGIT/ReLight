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

		auto parseBool = [&](const std::string& v) {
			return (v == "true" || v == "1" || v == "yes");
			};

		if (key == "removefakegloworbs") {
			globals::removeFakeGlowOrbs = parseBool(value);
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
inline void getObjectFadeMult(float& fLODFadeOutMultObjects) {

	if (auto* setting = RE::GetINISetting("fLODFadeOutMultObjects:LOD")) {
		if (setting->GetType() == RE::Setting::Type::kFloat) {
			fLODFadeOutMultObjects = setting->GetFloat() * 1000;
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

inline bool RayHitStatic(RE::bhkWorld* world, RE::NiPoint3 start, RE::NiPoint3 end)
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

	return (layer == RE::COL_LAYER::kStatic ||
		layer == RE::COL_LAYER::kTerrain ||
		layer == RE::COL_LAYER::kGround);
}

inline bool HasAnythingBetween(RE::TESObjectREFR* refA, RE::TESObjectREFR* refB)
{
	if (!refA || !refB) return false;

	auto* cell = refA->GetParentCell();
	if (!cell) return false;

	auto* world = cell->GetbhkWorld();
	if (!world) return false;

	RE::NiPoint3 start = refA->GetPosition();
	RE::NiPoint3 end = refB->GetPosition();

	start.z += 8.0f;
	end.z += 30.0f;

	// direction
	RE::NiPoint3 dir = end - start;
	dir.Unitize();

	// perpendicular offset
	RE::NiPoint3 perp(-dir.y, dir.x, 0.0f);
	perp.Unitize();

	float offset = 20.0f;

	RE::NiPoint3 startLeft = start - perp * offset;
	RE::NiPoint3 endLeft = end - perp * offset;

	RE::NiPoint3 startRight = start + perp * offset;
	RE::NiPoint3 endRight = end + perp * offset;

	bool hitCenter = RayHitStatic(world, start, end);
	bool hitLeft = RayHitStatic(world, startLeft, endLeft);
	bool hitRight = RayHitStatic(world, startRight, endRight);

	logger::debug("Ray results: center {} left {} right {}",
		hitCenter, hitLeft, hitRight);


	return hitCenter && hitLeft && hitRight;
}

