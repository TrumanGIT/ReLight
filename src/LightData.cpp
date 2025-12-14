#pragma once
#include <cstdint>
#include "global.h"
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "Functions.h"
#include "config.hpp"
#include "LightData.h"
#include <string>
#include <vector>

bool LightData::isISL = true; // we need a way to determine if isl, idk through config? 
                              // isl lights need different configuring then vanilla 

// at runtime save a copy of each tempaltes settings so we can restore to defaults later
std::unordered_map<std::string, LightConfig> LightData::defaultConfigs = {}; 

bool LightData::shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref, const std::string& modName)
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

// Try to exclude light by editorID.
bool LightData::excludeLightEditorID(const RE::TESObjectLIGH* light) {

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

/* // were not using tesobject ligh flags anymore need to reimplement this.
template <class T>
REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t>
parseLightFlags(const T& obj)
{
	REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> flags;

	for (const auto& flagStr : obj.flags) {
		auto it = kLightFlagMap.find(flagStr);
		if (it != kLightFlagMap.end()) {
			flags.set(it->second);
		}
	}
	return flags;
}*/

// create ni point light and wrap in ni pointer for safe keeping
RE::NiPointer<RE::NiPointLight> LightData::createNiPointLight() {
	auto* niPointLight = RE::NiPointLight::Create();
	if (!niPointLight) {
		logger::info("nipoint light creation failed");
		return nullptr; 
	}

	return RE::NiPointer<RE::NiPointLight>(niPointLight);
}

RE::NiPoint3 LightData::getNiPointLightRadius(const LightConfig& cfg)
{
	return RE::NiPoint3(cfg.radius, cfg.radius, cfg.radius);
}

void LightData::setNiPointLightAmbientAndDiffuse(RE::NiPointLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::warn("nullptr passed to set ni point light ambient and diffuse");
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	// supposedly main color of light 
	data.diffuse.red = cfg.diffuseColor[0] / 255.0f;
	data.diffuse.green = cfg.diffuseColor[1] / 255.0f;
	data.diffuse.blue = cfg.diffuseColor[2] / 255.0f;

	// idk about ambient after a quick google search it seems ambient is usually a fraction of diffuse
	// we could prolly research futher and get better results but for now good enough
	data.ambient.red = data.diffuse.red * cfg.ambientRatio;
	data.ambient.green = data.diffuse.green * cfg.ambientRatio;
	data.ambient.blue = data.diffuse.blue * cfg.ambientRatio;
}

void LightData::setNiPointLightPos(RE::NiPointLight* niPointLight, const LightConfig& cfg)
{
	if (!niPointLight) return;
	niPointLight->local.translate.x = cfg.position[0];
	niPointLight->local.translate.y = cfg.position[1];
	niPointLight->local.translate.z = cfg.position[2];
}

//void LightData::setISLFlag(RE::TESObjectLIGH* ligh)
//{
	//if (!ligh) return;

	//auto rawPtr = reinterpret_cast<std::uint32_t*>(&ligh->data.flags);
	//*rawPtr |= (1u << 14); // set 14th bit for ISL support
//}


void LightData::setISLData(RE::NiPointLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::error("light nullptr for node {}", cfg.nodeName);
		return;
	}
	if (auto* isl = ISL_Overlay::Get(niPointLight)) {

		isl->size = cfg.size; // isl
		isl->cutoffOverride = cfg.cutoffOverride; // isl 
		isl->fade = cfg.fade;
		isl->radius = cfg.radius;
		// trick isl into thinking the ref has a base object of type: TESObjectLIGH object
		isl->lighFormId = 0;
	}
}

void LightData::setNiPointLightDataFromCfg(RE::NiPointLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::error("light nullptr for node {}", cfg.nodeName);
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	logger::info(" Setting Light Data for {} from Configs", cfg.nodeName);

	data.fade = cfg.fade;
	data.radius = getNiPointLightRadius(cfg);

	
	logger::info(" radius set to: {} ", cfg.radius);
	logger::info(" fade set to: {} ", cfg.fade);

	setNiPointLightPos(niPointLight, cfg);

	logger::info(" position set to: x:{} y:{} z:{} ", cfg.position[0], cfg.position[1], cfg.position[2]);

	setNiPointLightAmbientAndDiffuse(niPointLight, cfg);

	logger::info(" diffuse color set to: r:{} g:{} b:{} ", cfg.diffuseColor[0], cfg.diffuseColor[1], cfg.diffuseColor[2]);

	if (isISL) {
		setISLData(niPointLight, cfg); 
	}
}

void LightData::assignNiPointLightsToBank(RE::NiPointer<RE::NiPointLight> niPointLight) {
	logger::info("Assigning niPointLight... total groups: {}", niPointLightNodeBank.size());
	
	if (!niPointLight) {
		logger::error("Failed to create ni point light");
		return;
	}

	try {
		for (auto& pair : niPointLightNodeBank) {
			const std::string& nodeName = pair.first;
			Template& temp = pair.second;
			const LightConfig& cfg = temp.config;
			auto& bankedNodes = temp.bank;

			if (nodeName != temp.config.nodeName) {
				logger::error("Template node name {} do not match map key {}", nodeName, temp.config.nodeName);
				continue;
			}

			setNiPointLightDataFromCfg(niPointLight.get(), cfg);

			//I noticed over 60 candles used from bank in bannered mare, I wonder if this is true. or if we are pulling
			// more lights then needed. should count all candles in a cell, see if matches bank count, if not then investigate
			const size_t maxNodes = (cfg.nodeName == "candle") ? 78 : 25;

			for (size_t i = 0; i < maxNodes; ++i) {

				auto clonedNiPointLight = cloneNiPointLight(niPointLight.get());

				if (!clonedNiPointLight) {
					logger::error("Failed to clone NiPointLight for node '{}' (iteration {})", nodeName, i);
					continue;
				}

				clonedNiPointLight->name = nodeName;

				RE::NiPointer<RE::NiPointLight> clonedNiPointLightPtr(clonedNiPointLight);

				logger::debug("Cloned NiPointLight for node '{}' (iteration {})", nodeName, i);
			
				// logger::info("adding to bank. ");
				bankedNodes.push_back(clonedNiPointLightPtr);
				logger::debug("Added cloned light for node '{}' (iteration {})", nodeName, i);
			}
		}

		logger::info("Finished assignClonedNodes");
	}
	catch (const std::exception& e) {
		logger::error("Exception in assignNiPointLightsToBank: {}", e.what());
		throw; // rethrow after logging
	}
}

void LightData::refillBankForSelectedTemplate(const std::string& lightName, const LightConfig& cfg) {

	auto& selectedTemplateNodeBank = niPointLightNodeBank[lightName].bank;

	auto size = selectedTemplateNodeBank.size();

	selectedTemplateNodeBank.clear();

	LightData::setNiPointLightDataFromCfg(masterNiPointLight.get(), cfg);

	for (std::size_t i = 0; i < size; i++) {

		auto clonedNiPointLight = cloneNiPointLight(masterNiPointLight.get());

		if (!clonedNiPointLight) {
			logger::error("Failed to clone NiPointLight for node '{}' (iteration {})", lightName, i);
			continue;
		}

		clonedNiPointLight->name = lightName;

		RE::NiPointer<RE::NiPointLight> clonedNiPointLightPtr(clonedNiPointLight);

		logger::debug("Cloned NiPointLight for node '{}' (iteration {})", lightName, i);

		// logger::info("adding to bank. ");
		selectedTemplateNodeBank.push_back(clonedNiPointLightPtr);
		logger::debug("Added cloned light for node '{}' (iteration {})", lightName, i);
	}
}

RE::ShadowSceneNode::LIGHT_CREATE_PARAMS LightData::makeLightParams(const LightConfig& cfg)
{
	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS p{};

	// Couldn't do it with a macro as not all config. variables can be used with LIGHT_CREATE_PARAMS.

	//Truman -  sounds good homie idk how to use that shit anyway xD

	p.dynamic = true;    // dynamic = game updates it every frame so yes
	p.shadowLight = cfg.shadowLight;   
	p.portalStrict = cfg.portalStrict; // idk 
	p.affectLand = cfg.affectLand; 
	p.affectWater = cfg.affectWater; 
	p.neverFades = cfg.neverFades; 

	p.fov = cfg.fov;   // idk
	p.falloff = cfg.falloff;    // idk 
	p.nearDistance = cfg.nearDistance; // idk
	p.depthBias = cfg.depthBias; // idk 

	p.sceneGraphIndex = 0;      // always use 0 

	p.restrictedNode = nullptr; //idk
	p.lensFlareData = nullptr; //idk 

	return p;
}

void LightData::attachNiPointLightToShadowSceneNode(RE::NiPointLight* niPointLight, const LightConfig& cfg) {

	//logger::info("attempting to create NiPointLight BSlight and attach to ShadowSceneNode");

	if (!niPointLight) {
		logger::error("createShadowSceneNode: niPointLight is null");
		return;
	}

	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params = makeLightParams(cfg);

	logger::debug("Light paramaters for {}", niPointLight->name);

	printLightParams(params);

	auto* shadowSceneNode = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];

	if (!shadowSceneNode) {
		logger::warn("no shadow scene node to grab in (createShadowSceneNode()");
		return;
	}
	RE::BSLight* BsLight = shadowSceneNode->AddLight(niPointLight, params);

	if (!BsLight) {
		logger::info("no BSLight created in (createShadowSceneNode() for {}", niPointLight->name);
		return;
	}
}

/*std::string LightData::getBaseNodeName(const std::string& lightName) {
	const std::string suffix = "_rl";
	if (lightName.size() >= suffix.size() && lightName.compare(lightName.size() - suffix.size(), suffix.size(), suffix) == 0) {
		return lightName.substr(0, lightName.size() - suffix.size());
	}
	return lightName;
} */ 

bool LightData::findConfigForLight(LightConfig& cfg, const std::string& lightName) {
	//const std::string baseName = getBaseNodeName(lightName);
	for  (auto& [name, temp] : niPointLightNodeBank) {
		if (name == lightName) {
			cfg = temp.config;
			return true;
		}
	}
	return false;
}

void LightData::updateConfigFromLight(LightConfig& cfg, RE::NiLight* niLight) {
	auto& rt = niLight->GetLightRuntimeData();

	cfg.radius = rt.radius.x;

	cfg.fade = rt.fade;

	cfg.position[0] = niLight->local.translate.x;
	cfg.position[1] = niLight->local.translate.y;
	cfg.position[2] = niLight->local.translate.z;

	cfg.diffuseColor[0] = int(rt.diffuse.red * 255.0f);
	cfg.diffuseColor[1] = int(rt.diffuse.green * 255.0f);
	cfg.diffuseColor[2] = int(rt.diffuse.blue * 255.0f);

	if (auto* isl = ISL_Overlay::Get(niLight)) {
		cfg.size = isl->size;
		cfg.cutoffOverride = isl->cutoffOverride;
	}
}

