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

bool LightData::isISL = false; // we need a way to determine if isl, idk through config? 
                              // isl lights need different configuring then vanilla 

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

RE::NiPointLight* LightData::createNiPointLight() {
	auto* niPointLight = RE::NiPointLight::Create();
	if (!niPointLight) {
		logger::info("nipoint light creation failed");
	}
	return niPointLight;
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

void LightData::setISLFlag(RE::TESObjectLIGH* ligh)
{
	if (!ligh) return;

	auto rawPtr = reinterpret_cast<std::uint32_t*>(&ligh->data.flags);
	*rawPtr |= (1u << 14); // set 14th bit for ISL support
}


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
		isl->lighFormId = LoadScreenLightMain ? LoadScreenLightMain->GetFormID() : 0;
	}

	if (LoadScreenLightMain) {
		LightData::setISLFlag(LoadScreenLightMain);
	}
}

void LightData::setNiPointLightData(RE::NiPointLight* niPointLight, const LightConfig& cfg, RE::TESObjectLIGH* lighTemplate) {
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

void LightData::assignNiPointLightsToBank() {
	logger::info("Assigning niPointLight... total groups: {}", niPointLightNodeBank.size());

	try {
		for (auto& pair : niPointLightNodeBank) {
			const auto& cfg = pair.first;
			auto& bankedNodes = pair.second;

			// Create NiPointLight 
			auto niPointLight = createNiPointLight();

			if (!niPointLight) {
				logger::error("failed to create ni point light for '{} Json config'", cfg.nodeName);
				continue;
			}

			logger::debug("Created NiPointLight for node '{}'", cfg.nodeName);

			setNiPointLightData(niPointLight, cfg, LoadScreenLightMain);

			const size_t maxNodes = (cfg.nodeName == "candle") ? 60 : 20;

			for (size_t i = 0; i < maxNodes; ++i) {

				auto clonedNiPointLightAsNiObject = CloneNiPointLight(niPointLight);
				if (!clonedNiPointLightAsNiObject) {
					logger::error("Failed to clone NiPointLight for node '{}' (iteration {})", cfg.nodeName, i);
					continue;
				}

				logger::debug("Cloned NiPointLight for node '{}' (iteration {})", cfg.nodeName, i);

				clonedNiPointLightAsNiObject->name = cfg.nodeName + "_rl";

				// cast from niobject to nipoint light and extract from ni pointer 
				RE::NiPointLight* clonedNiPointLight = netimmerse_cast<RE::NiPointLight*>(clonedNiPointLightAsNiObject);
				if (!clonedNiPointLight) {
					logger::error("Cloned NiPointer is null for node '{}' (iteration {})", cfg.nodeName, i);
					continue;
				}

				//wrap in ni pointer again 
				RE::NiPointer<RE::NiPointLight> clonedNiPointLightPtr(clonedNiPointLight);

				// logger::info("adding to bank. ");
				bankedNodes.push_back(clonedNiPointLightPtr);
				logger::debug("Added cloned light for node '{}' (iteration {})", cfg.nodeName, i);
			}
		}


		logger::info("Finished assignClonedNodes");
	}
	catch (const std::exception& e) {
		logger::error("Exception in assignNiPointLightsToBank: {}", e.what());
		throw; // rethrow after logging
	}
}


RE::ShadowSceneNode::LIGHT_CREATE_PARAMS LightData::makeLightParams(const LightConfig& cfg)
{
	RE::ShadowSceneNode::LIGHT_CREATE_PARAMS p{};

	// Couldn't do it with a macro as not all config. variables can be used with LIGHT_CREATE_PARAMS.

	//Truman -  sounds good homie idk how to use that shit anyway xD

	p.dynamic = true;    // dynamic = game updates it every frame so yes
	p.shadowLight = cfg.shadowLight;   // obvious enough
	p.portalStrict = cfg.portalStrict; // idk 
	p.affectLand = cfg.affectLand; // obvious enough
	p.affectWater = cfg.affectWater; // obvious enough
	p.neverFades = cfg.neverFades; // obvious enough

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

