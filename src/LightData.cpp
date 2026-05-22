
#include <cstdint>
#include "global.h"
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "config.hpp"
#include "LightData.h"
#include <string>
#include <vector>
#include  "Utility.h"

NiPointLight LightData::masterNiPointLight;

std::deque<LightData::TriLightCache> LightData::triLightCache; 
std::mutex LightData::triLightCacheMutex;
std::atomic<uint16_t> LightData::triLightCacheGeneration = { 0 };

// (config to json id is for faster lookups then strings, used in flicker logic)
std::map<uint32_t, LightConfig> LightData::configIDToJsonCfg;

std::unordered_map<std::string, std::vector<LightConfig>> LightData::meshPathToJsonCfg;
std::unordered_map<std::string, std::vector<LightConfig>> LightData::meshPathToJsonCfgExteriors;

std::unordered_map<RE::FormID, std::vector<LightConfig>> LightData::refFormIDToJsonCfg;
std::unordered_map<RE::FormID, std::vector<LightConfig>> LightData::refFormIDToJsonCfgExteriors;

// at runtime save a copy of each tempaltes settings so we can restore to defaults later
std::unordered_map<uint32_t, LightConfig> LightData::defaultConfigs;

void LightData::ResetTriLightCache()
{
	LightData::triLightCacheGeneration.fetch_add(1);
}

RE::NiPoint3 LightData::getNiPointLightRadius(const LightConfig& cfg, const float scale)
{
	float z = cfg.fov;
	z = z >= 50.0f ? 1.414f : z;
	z = std::clamp(z, 0.01f, 50.0f);

	return RE::NiPoint3(cfg.radius * scale, cfg.radius * scale, z * scale);
}

void LightData::setNiPointLightAmbientAndDiffuse(RE::NiLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::warn("nullptr passed to set ni point light ambient and diffuse");
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	// supposedly main color of light 
	data.diffuse.red = cfg.diffuseColor[0] / 255.0f;
	data.diffuse.green = cfg.diffuseColor[1] / 255.0f;
	data.diffuse.blue = cfg.diffuseColor[2] / 255.0f;

	// ambient is removed with community shaders and will break functinality
	//if its set because you will be writing into the wrong fields
	if (!globals::islInstalled) {
		data.ambient.red = data.diffuse.red * cfg.ambientRatio;
		data.ambient.green = data.diffuse.green * cfg.ambientRatio;
		data.ambient.blue = data.diffuse.blue * cfg.ambientRatio;
	}
}

void LightData::setNiPointLightPos(RE::NiLight* niPointLight, const LightConfig& cfg)
{
	if (!niPointLight) {
		logger::warn("nullptr passed to set ni point light ambient and diffuse");
		return;
	}
	niPointLight->local.translate.x = cfg.position[0];
	niPointLight->local.translate.y = cfg.position[1];
	niPointLight->local.translate.z = cfg.position[2];
}

void LightData::setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::error("light nullptr for mesh {}", cfg.menuName);
		return;
	}
	if (auto* overlay = Overlay::Get(niPointLight)) {

		constexpr std::uint32_t kInverseSquare = 1u << 10;

		overlay->flags |= kInverseSquare;
		overlay->size = cfg.size; // isl
		overlay->cutoffOverride = cfg.cutoffOverride; // isl 
		overlay->lighFormId = 0; 
		logger::debug(" size set to: {} ", overlay->size);
		logger::debug("cutoffOverride  set to {}", overlay->cutoffOverride);
	}
}

void LightData::setNiPointLightDataFromCfg(RE::NiLight* niPointLight, const LightConfig& cfg, float scale) {
	if (!niPointLight) {
		logger::error("light nullptr for node {}", cfg.menuName);
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	auto& lightName = cfg.menuName;

	logger::debug(" Setting Light Data for {} from Configs", lightName);

	if (scale >= 1.0f) scale = 1.0f; 

	data.fade = cfg.brightness * scale * globals::brightnessModifier;
	data.radius = getNiPointLightRadius(cfg, scale);

	// used for quick runtime acess in flicker calcs and the in game menu (quicker then string lookup)
	data.unk138 = cfg.configID;

	logger::debug(" radius set to: {} ", cfg.radius);
	logger::debug("objects scale = {}", scale); 
	logger::debug(" brightness set to: {} ", cfg.brightness);
	logger::debug("config ID set to {}", cfg.configID); 

	setNiPointLightPos(niPointLight, cfg);

	logger::debug(" position set to: x:{} y:{} z:{} ", cfg.position[0], cfg.position[1], cfg.position[2]);

	setNiPointLightAmbientAndDiffuse(niPointLight, cfg);

	logger::debug(" diffuse color set to: r:{} g:{} b:{} ", cfg.diffuseColor[0], cfg.diffuseColor[1], cfg.diffuseColor[2]);

	if (globals::islInstalled) setOverlayData(niPointLight, cfg);

}

 float LightData::GetFOV(LightConfig cfg)
{
	if (!cfg.shadowLight) {
		return 1.0;
	}

	if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kSpotLight)) {
	 return RE::deg_to_rad(cfg.fov > 0.0f ? cfg.fov : 90.0f);
	}

	return RE::NI_TWO_PI;
}

RE::ShadowSceneNode::LIGHT_CREATE_PARAMS LightData::makeLightParams(const LightConfig& cfg)
{
	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS p{};

	// Couldn't do it with a macro as not all config. variables can be used with LIGHT_CREATE_PARAMS.

	//Truman -  sounds good homie idk how to use that shit anyway xD

	p.dynamic = true;    // dynamic = game updates it every frame? 
	p.shadowLight = cfg.shadowLight;   
	p.portalStrict = cfg.portalStrict; // should always be on something to do with culling / optimizaitons
	p.affectLand = cfg.affectLand; 
	p.affectWater = cfg.affectWater; 
	p.neverFades = cfg.neverFades; // no draw back apparently to set true (rob from skyblivion)

	p.fov = GetFOV(cfg);          // for spotlights and hemi spheres maybe? 
	p.falloff = cfg.falloff;    // shadows (I think)
	p.nearDistance = cfg.nearDistance; // shadows (I think)
	p.depthBias = cfg.depthBias; // shadows (I think)

	p.sceneGraphIndex = 0;      // always use 0 ?

	p.restrictedNode = nullptr; //idk
	p.lensFlareData = nullptr; //idk

	return p;
}

bool LightData::foundConfigForLight(const RE::NiLight* light) {
	return LightData::configIDToJsonCfg.contains(light->unk138);
}

void LightData::updateConfigFromLight(LightConfig& cfg, const LightConfig& baseConfig, RE::NiLight* niLight) {

	auto& rt = niLight->GetLightRuntimeData();
	cfg = baseConfig; 

	cfg.radius = rt.radius.x;
	cfg.brightness = cfg.startingFade;

	//cfg.position[0] = niLight->local.translate.x;
	//cfg.position[1] = niLight->local.translate.y;
	//cfg.position[2] = niLight->local.translate.z;

	cfg.diffuseColor[0] = int(rt.diffuse.red * 255.0f);
	cfg.diffuseColor[1] = int(rt.diffuse.green * 255.0f);
	cfg.diffuseColor[2] = int(rt.diffuse.blue * 255.0f);

	cfg.flickerIntensity = baseConfig.flickerIntensity;
	cfg.flickersPerSecond = baseConfig.flickersPerSecond;

	if (globals::islInstalled) {

		if (auto* overlay = Overlay::Get(niLight)) {
			cfg.size = overlay->size;
			cfg.cutoffOverride = overlay->cutoffOverride;
		}
	}
	cfg.print(false);
}

bool LightData::updateRuntimeConfigCaches(const LightConfig& updatedCfg)
{
	bool updated = false;

	// keep direct configID lookup in sync
	configIDToJsonCfg[updatedCfg.configID] = updatedCfg;

	for (auto meshKey : updatedCfg.meshPaths) {
		if (meshKey.empty()) {
			continue;
		}

		toLower(meshKey);

		if (auto it = meshPathToJsonCfg.find(meshKey); it != meshPathToJsonCfg.end()) {
			updated |= updateConfigMap(it->second, updatedCfg);
		}

		if (auto it = meshPathToJsonCfgExteriors.find(meshKey); it != meshPathToJsonCfgExteriors.end()) {
			updated |= updateConfigMap(it->second, updatedCfg);
		}
	}

	for (auto refKey : updatedCfg.refFormIDsAndModNames) {
		if (refKey.empty()) {
			continue;
		}

		toLower(refKey);

		auto tildePos = refKey.find('~');
		if (tildePos == std::string::npos) {
			logger::warn("Invalid refID format '{}' while updating runtime config cache", refKey);
			continue;
		}

		std::string formIDStr = trim(refKey.substr(0, tildePos));
		std::string modName = trim(refKey.substr(tildePos + 1));

		try {
			if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
				formIDStr = formIDStr.substr(2);
			}

			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				logger::warn("TESDataHandler was null while updating ref config cache");
				continue;
			}

			RE::FormID parsedID = std::stoul(formIDStr, nullptr, 16);

			auto mod = dataHandler->LookupModByName(modName);
			if (!mod) {
				logger::warn("Invalid mod name '{}' while updating ref config cache", modName);
				continue;
			}

			RE::FormID runtimeID = 0;

			if (mod->IsLight()) {
				auto* ref = dataHandler->LookupForm<RE::TESObjectREFR>(parsedID, modName);
				if (!ref) {
					logger::warn(
						"Failed to resolve light plugin ref localID 0x{:X} from mod '{}' while updating ref config cache",
						static_cast<std::uint32_t>(parsedID),
						modName);
					continue;
				}

				runtimeID = ref->GetFormID();
			}
			else {
				runtimeID = parsedID;
			}

			if (updatedCfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
				auto& vec = refFormIDToJsonCfgExteriors[runtimeID];
				updated |= updateConfigMap(vec, updatedCfg);
			}
			else {
				auto& vec = refFormIDToJsonCfg[runtimeID];
				updated |= updateConfigMap(vec, updatedCfg);
			}
		}
		catch (...) {
			logger::warn(
				"Failed to parse refID '{}' while updating runtime config cache",
				refKey);
		}
	}

	return updated;
}

//used to stop light flicker prevention mode from blocking dyncamically spawned lights from 
// torch or candle activators. 
void LightData::InvalidateTriLightCacheForActivator(RE::TESObjectREFR* ref)
{
	if (!ref) {
		return;
	}

	auto* baseObj = ref->GetBaseObject();
	if (!baseObj || baseObj->GetFormType() != RE::FormType::Activator) {
		return;
	}

	if (!globals::secondAfterCellFullyLoaded.load()) {
		return;
	}

	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) {
		return;
	}

	const auto refPos = ref->GetPosition();
	const auto playerPos = player->GetPosition();

	const float dx = refPos.x - playerPos.x;
	const float dy = refPos.y - playerPos.y;
	const float dz = refPos.z - playerPos.z;

	const float dist2 = dx * dx + dy * dy + dz * dz;

	constexpr float maxInvalidateDistance = 300.0f;

	if (dist2 > maxInvalidateDistance * maxInvalidateDistance) {
		return;
	}

	SKSE::GetTaskInterface()->AddTask([]() {
		logger::debug("nearby activator spawned, resetting light cache");
		LightData::triLightCacheGeneration.fetch_add(1);
		});
}

void LightData::AddConfigToMaps(
	const LightConfig& cfg,
	bool isRefLight,
	RE::FormID refFormID)
{
	LightData::configIDToJsonCfg[cfg.configID] = cfg;
	LightData::defaultConfigs[cfg.configID] = cfg;

	if (isRefLight) {

		if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
			LightData::refFormIDToJsonCfgExteriors[refFormID].push_back(cfg);
		}
		else {
			LightData::refFormIDToJsonCfg[refFormID].push_back(cfg);
		}

		logger::info(
			"RegisterConfigInMaps: added ref config '{}' at {} index {}",
			cfg.menuName,
			cfg.configPath,
			cfg.jsonIndex);
	}
	else {

		for (auto meshPath : cfg.meshPaths) {

			if (meshPath.empty()) {
				continue;
			}

			toLower(meshPath);

			if (cfg.flags & static_cast<uint32_t>(LIGHT_FLAGS::kOutdoor)) {
				LightData::meshPathToJsonCfgExteriors[meshPath].push_back(cfg);
			}
			else {
				LightData::meshPathToJsonCfg[meshPath].push_back(cfg);
			}

			globals::priorityList.push_back(meshPath);

			logger::info(
				"RegisterConfigInMaps: added mesh config '{}' at {} index {}",
				meshPath,
				cfg.configPath,
				cfg.jsonIndex);
		}
	}
}