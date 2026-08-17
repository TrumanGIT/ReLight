
#include "LightData.h"
#include  "Utility.h"

// all relight lights are cloned from this light which itself is a clone of a fresh nipoint light
NiPointLight LightData::masterNiPointLight;

// used for light flicker prevention mode (should light affect surface hook)
std::deque<LightData::TriLightCache> LightData::triLightCache; 
std::mutex LightData::triLightCacheMutex;
std::atomic<uint16_t> LightData::triLightCacheGeneration = { 0 };

// (config to json id is for faster lookups then strings, used in flicker logic)
std::map<uint32_t, LightConfig> LightData::configIDToJsonCfg;

std::unordered_map<std::string, std::vector<LightConfig>> LightData::meshPathToJsonCfg;
std::unordered_map<std::string, std::vector<LightConfig>> LightData::meshPathToJsonCfgExteriors;

std::unordered_map<RE::FormID, std::vector<LightConfig>> LightData::refFormIDToJsonCfg;
std::unordered_map<RE::FormID, std::vector<LightConfig>> LightData::refFormIDToJsonCfgExteriors;

std::unordered_map<RE::FormID, std::vector<LightConfig>> LightData::baseFormIDToJsonCfg;
std::unordered_map<RE::FormID, std::vector<LightConfig>> LightData::baseFormIDToJsonCfgExteriors;

// at runtime save a copy of each tempaltes settings so we can restore to defaults later
std::unordered_map<uint32_t, LightConfig> LightData::defaultConfigs;

RE::TESForm* LightData::GetPluginLightEmittanceSource(RE::TESObjectREFR* ref) {

	if (!ref)
		return nullptr; 

	auto emittanceData = ref->extraList.GetByType<RE::ExtraEmittanceSource>();

	if (!emittanceData) return nullptr; 

	auto form = emittanceData->source; 

	if (!form) return nullptr; 

	return form; 
}

void LightData::SetPluginLightEmittanceSource(RE::TESObjectREFR* ref, RE::TESForm* form)
{
	if (!ref)
		return;

	if (auto* extra = ref->extraList.GetByType<RE::ExtraEmittanceSource>()) {
		extra->source = form;
	}
}

void LightData::ResetTriLightCache()
{
	// this is turned true 1 second later in the update hook giving some needed time to allow 
	// trichace to reflect the proper lighting scene.
	globals::secondAfterCellFullyLoaded.store(false);
	globals::cellFullyLoadedTimerStart = std::chrono::steady_clock::now();
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

	// flicker lights in vanillas position are set in their update hook using a offset. 
	bool isPluginLightWithFlicker = cfg.isPluginLight &&
		(cfg.flags & (static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kFlicker) |
			static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kFlickerSlow) |
			static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kPulse) |
			static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kPulseSlow)));

	if (!isPluginLightWithFlicker) {
		niPointLight->local.translate.x = cfg.position[0];
		niPointLight->local.translate.y = cfg.position[1];
		niPointLight->local.translate.z = cfg.position[2];
	}

	bool isSpotlight = LightData::HasRelightFlag(cfg.flags, RELIGHT_FLAGS::kSpotLight) ||
		(cfg.flags & static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kSpotlight)) ||
		(cfg.flags & static_cast<std::uint32_t>(RE::TES_LIGHT_FLAGS::kSpotShadow));



	if (isSpotlight) {
		RE::NiPoint3 angles;
		angles.x = RE::deg_to_rad(cfg.rotation[0]);
		angles.y = RE::deg_to_rad(cfg.rotation[1]);
		angles.z = RE::deg_to_rad(cfg.rotation[2]);
		niPointLight->local.rotate.SetEulerAnglesXYZ(angles);
	}

	// free floats used for merged lights position, needed for movement in flicker calcs
	if (!cfg.isPluginLight) {
	niPointLight->worldBound.center.x = cfg.position[0];
	niPointLight->worldBound.center.y = cfg.position[1];
	niPointLight->worldBound.center.z = cfg.position[2];
}
}

void LightData::setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::error("light nullptr for mesh {}", cfg.menuName);
		return;
	}
	if (auto* overlay = Overlay::Get(niPointLight)) {

		constexpr std::uint32_t kInverseSquare = 1u << 10;

		constexpr std::uint32_t kLinear = 1u << 11;
		
		//plugin lights get ISL support through their refs base object TESobjectLIGH flags
		if (!cfg.isPluginLight && (globals::allRelightsAsISL || LightData::HasRelightFlag(cfg.flags, RELIGHT_FLAGS::kInverseSquare))) {
			overlay->flags |= kInverseSquare;
		}

		//plugin lights get Linear lighting support through their refs base objects flags
		if (!cfg.isPluginLight && LightData::HasRelightFlag(cfg.flags, RELIGHT_FLAGS::kLinear)) {
			overlay->flags |= kLinear;
		}
			
		overlay->size = cfg.size; // isl
		overlay->cutoffOverride = cfg.cutoffOverride; // isl  
		logger::debug(" size set to: {} ", overlay->size);
		logger::debug("cutoffOverride  set to {}", overlay->cutoffOverride);
		logger::debug("Flags: 0x{:08X}",overlay->flags);
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

	//fireplace in whiterun dragons reach is 3.3x size so scaling light radius and brightness by scale is too much 
	// however we still scale mini objects like the 2 small fires on windhelm throne
	if (scale >= 1.0f) scale = 1.0f; 

	auto mult = cfg.isPluginLight ? globals::vanillaBrightnessModifier : globals::brightnessModifier; 

	data.fade = cfg.brightness * scale * mult; 

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

void LightData::SetTESObjectLightDataFromConfig(RE::TESObjectLIGH* light, const LightConfig& config) {

	light->data.fov = config.fov,
	light->data.fallofExponent = config.falloff; 
	light->data.nearDistance = config.nearDistance;

	light->data.flags.reset(); 
	ApplyTESLightFlags(light, config);
}

RE::TESObjectREFR* LightData::GetRefFromLight(RE::NiLight* light) {

	auto ref = light->GetUserData();

	return ref;
}

 float LightData::GetFOV(const LightConfig& cfg)
{
	if (!cfg.shadowLight) {
		return 90.0f;
	}

	if (LightData::HasRelightFlag(cfg.flags,RELIGHT_FLAGS::kSpotLight)) {
	 return RE::deg_to_rad(cfg.fov > 0.0f ? cfg.fov : 90.0f);
	}

	return RE::NI_TWO_PI;
}

RE::ShadowSceneNode::LIGHT_CREATE_PARAMS LightData::makeLightParams(const LightConfig& cfg)
{
	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS p{};

	p.dynamic = true;    // dynamic = game updates it every frame? 
	p.shadowLight = cfg.shadowLight;   
	p.portalStrict = cfg.portalStrict; // should always be on something to do with portals
	p.affectLand = cfg.affectLand; 
	p.affectWater = cfg.affectWater; 
	p.neverFades = cfg.neverFades; // no draw back apparently to set true (rob from skyblivion)

	//ISL reuses these size is controlled by the FOV  cutoff can be controlled by setting the Falloff Exponent parameter
	// not sure if isl uses the TESObjectLigh parameters or this one prolly TESOBjectLigh

	p.fov = GetFOV(cfg);          // for spotlights and hemi spheres maybe? 
	p.falloff = cfg.falloff;    // shadows (I think)
	
	p.nearDistance = cfg.nearDistance; // shadows (I think)
	p.depthBias = cfg.depthBias; // shadows 

	p.sceneGraphIndex = 0;      // always use 0 ?

	p.restrictedNode = nullptr; //idk
	p.lensFlareData = nullptr; //idk

	return p;
}

 RE::TESObjectLIGH* LightData::GetTESObjectLightFromNiLight(RE::NiLight* niLight)
{
	if (!niLight)
		return nullptr;

	auto ref = LightData::GetRefFromLight(niLight); 
	if (!ref)
		return nullptr;

	auto* baseObject = ref->GetBaseObject();
	if (!baseObject)
		return nullptr;

	return baseObject->As<RE::TESObjectLIGH>();
}

  bool LightData::HasRelightFlag(uint32_t flags, RELIGHT_FLAGS flag)
 {
	 return (flags & static_cast<uint32_t>(flag)) != 0;
 }

  bool LightData::ShouldMergeByFlags(uint32_t refAflags, uint32_t otherRefFlags)
 {
	 // same-type merges
	 if (HasRelightFlag(refAflags, RELIGHT_FLAGS::kCandle) &&
		 HasRelightFlag(otherRefFlags, RELIGHT_FLAGS::kCandle)) {
		 return true;
	 }

	 if (HasRelightFlag(refAflags, RELIGHT_FLAGS::kFire) &&
		 HasRelightFlag(otherRefFlags, RELIGHT_FLAGS::kFire)) {
		 return true;
	 }

	 // giant campfires merge with fires
	 if ((HasRelightFlag(refAflags, RELIGHT_FLAGS::kGiantCampfire) &&
		 HasRelightFlag(otherRefFlags, RELIGHT_FLAGS::kFire)) ||
		 (HasRelightFlag(refAflags, RELIGHT_FLAGS::kFire) &&
			 HasRelightFlag(otherRefFlags, RELIGHT_FLAGS::kGiantCampfire))) {
		 return true;
	 }

	 if (HasRelightFlag(refAflags, RELIGHT_FLAGS::kOther) &&
		 HasRelightFlag(otherRefFlags, RELIGHT_FLAGS::kOther)) {
		 return true;
	 }

	 return false;
 }


bool LightData::foundConfigForLightByConfigID(const RE::NiLight* light) {
	return LightData::configIDToJsonCfg.contains(light->unk138);
}

std::vector<LightConfig>* LightData::findConfigsByFormID(
	RE::FormID formID,
	bool isInterior,
	bool isBaseID)
{
	auto rawIndex = (formID & 0xFF000000) >> 24;

	bool isLight = rawIndex == 0xFE;
	if (!isLight) {
		formID &= 0x00FFFFFF;
	}

	auto& interiorMap = isBaseID
		? LightData::baseFormIDToJsonCfg
		: LightData::refFormIDToJsonCfg;

	auto& exteriorMap = isBaseID
		? LightData::baseFormIDToJsonCfgExteriors
		: LightData::refFormIDToJsonCfgExteriors;


	if (isInterior) {
		auto it = interiorMap.find(formID);
		if (it != interiorMap.end()) {
			logger::debug("Found interior {} config for 0x{:08X} ({} configs)",
				isBaseID ? "base" : "ref",
				static_cast<std::uint32_t>(formID),
				it->second.size());

			return &it->second;
		}
	}
	else {
		auto it = exteriorMap.find(formID);
		if (it != exteriorMap.end()) {
			logger::debug("Found exterior {} config for 0x{:08X} ({} configs)",
				isBaseID ? "base" : "ref",
				static_cast<std::uint32_t>(formID),
				it->second.size());

			return &it->second;
		}

		// Exterior fallback to interior
		auto it2 = interiorMap.find(formID);
		if (it2 != interiorMap.end()) {
			logger::debug("Fell back to interior {} config for exterior 0x{:08X} ({} configs)",
				isBaseID ? "base" : "ref",
				static_cast<std::uint32_t>(formID),
				it2->second.size());

			return &it2->second;
		}
	}

	return nullptr;
}

void LightData::updateConfigFromLight(LightConfig& cfg, const LightConfig& baseConfig, RE::NiLight* niLight) {

	auto& rt = niLight->GetLightRuntimeData();
	cfg = baseConfig; 

	cfg.radius = rt.radius.x;
	cfg.brightness = cfg.startingFade;

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

	// Update mesh path caches
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



		for (auto baseKey : updatedCfg.baseFormIDsAndModNames) {
			if (baseKey.empty()) {
				continue;
			}

			toLower(baseKey);

			auto tildePos = baseKey.find('~');
			if (tildePos == std::string::npos) {
				logger::warn("Invalid baseID format '{}' while updating runtime config cache", baseKey);
				continue;
			}

			std::string formIDStr = trim(baseKey.substr(0, tildePos));
			std::string modName = trim(baseKey.substr(tildePos + 1));

			try {
				if (formIDStr.starts_with("0x") || formIDStr.starts_with("0X")) {
					formIDStr = formIDStr.substr(2);
				}

				auto* dataHandler = RE::TESDataHandler::GetSingleton();
				if (!dataHandler) {
					logger::warn("TESDataHandler was null while updating base config cache");
					continue;
				}

				RE::FormID parsedID = std::stoul(formIDStr, nullptr, 16);

				const bool isLightPlugin = parsedID <= 0xFFF;

				if (!isLightPlugin) {
					parsedID &= 0x00FFFFFF;
				}

				auto mod = dataHandler->LookupModByName(modName);
				if (!mod) {
					logger::warn("Invalid mod name '{}' while updating base config cache", modName);
					continue;
				}

		

				RE::FormID runtimeID = 0;

				if (isLightPlugin) {
					// For light plugins, we need to resolve the actual FormID
					// Since this is a base ID, we look up the TESObjectLIGH by local ID
					auto* baseObject = dataHandler->LookupForm<RE::TESObjectLIGH>(parsedID, modName);
					if (!baseObject) {
						logger::warn(
							"Failed to resolve light plugin base localID 0x{:X} from mod '{}' while updating base config cache",
							static_cast<std::uint32_t>(parsedID),
							modName);
						continue;
					}
					runtimeID = baseObject->GetFormID();
				}
				else {
					runtimeID = parsedID;
				}

				if (LightData::HasRelightFlag(updatedCfg.flags, RELIGHT_FLAGS::kOutdoor)) {
					auto& vec = baseFormIDToJsonCfgExteriors[runtimeID];
					updated |= updateConfigMap(vec, updatedCfg);
				}
				else {
					auto& vec = baseFormIDToJsonCfg[runtimeID];
					updated |= updateConfigMap(vec, updatedCfg);
				}
			}
			catch (...) {
				logger::warn(
					"Failed to parse baseID '{}' while updating runtime config cache",
					baseKey);
			}
		}
	
		// Update ref ID caches (original behavior)
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

				const bool isLightPlugin = parsedID <= 0xFFF;

				if (!isLightPlugin) {
					parsedID &= 0x00FFFFFF;
				}

				RE::FormID runtimeID = 0;

				if (isLightPlugin) {
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

				if (LightData::HasRelightFlag(updatedCfg.flags, RELIGHT_FLAGS::kOutdoor)) {
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
	if (!ref || !globals::secondAfterCellFullyLoaded.load()) {
		return;
	}

	auto* baseObj = ref->GetBaseObject();
	if (!baseObj || baseObj->GetFormType() != RE::FormType::Activator) {
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
	RE::FormID formID)
{
	LightData::configIDToJsonCfg[cfg.configID] = cfg;
	LightData::defaultConfigs[cfg.configID] = cfg;

	if (isRefLight) {

		if (LightData::HasRelightFlag(cfg.flags,RELIGHT_FLAGS::kOutdoor)) {
			LightData::refFormIDToJsonCfgExteriors[formID].push_back(cfg);
		}
		else {
			LightData::refFormIDToJsonCfg[formID].push_back(cfg);
		}

		logger::info(
			"RegisterConfigInMaps: added ref config '{}' at {} index {}",
			cfg.menuName, cfg.configPath, cfg.jsonIndex);
	}
	else {

		if (LightData::HasRelightFlag(cfg.flags, RELIGHT_FLAGS::kOutdoor)) {
			LightData::baseFormIDToJsonCfgExteriors[formID].push_back(cfg);
		}
		else {
			LightData::baseFormIDToJsonCfg[formID].push_back(cfg);
		}

		logger::info(
			"RegisterConfigInMaps: added base config '{}' at {} index {}",
			cfg.menuName, cfg.configPath, cfg.jsonIndex);
	}
}