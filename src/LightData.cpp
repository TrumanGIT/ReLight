
#include <cstdint>
#include "global.h"
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "config.hpp"
#include "LightData.h"
#include <string>
#include <vector>
#include  "Utility.h"

// (config to json id is for faster lookups then strings, used in flicker logic)
std::map<uint64_t, LightConfig> LightData::configIDToJsonCfg;

// for search by mesh file path instead of node name
std::unordered_map<std::string, std::vector<LightConfig>> LightData::meshPathToJsonCfg;

//base lookup used when attaching lights to meshes
std::unordered_map<std::string, std::vector<LightConfig>> LightData:: nodeNameToJsonCfg;

// at runtime save a copy of each tempaltes settings so we can restore to defaults later
std::unordered_map<uint64_t, LightConfig> LightData::defaultConfigs;

// Try to exclude light by editorID.
bool LightData::excludeLightEditorID(const RE::TESObjectLIGH* light) {

	std::string edid = clib_util::editorID::get_editorID(light);

	if (!edid.empty()) {
		for (const auto& group : globals::keywordLightGroups) {
			if (containsAll(edid, group)) {
				logger::info("Excluding light by editorID: {}", edid);
				return true;
			}
		}
	}
	return false;
}

RE::NiPoint3 LightData::getNiPointLightRadius(const LightConfig& cfg)
{
	return RE::NiPoint3(cfg.radius, cfg.radius, cfg.radius);
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

	// idk about ambient after a quick google search it seems ambient is usually a fraction of diffuse
	// we could prolly research futher and get better results but for now good enough
	data.ambient.red = data.diffuse.red * cfg.ambientRatio;
	data.ambient.green = data.diffuse.green * cfg.ambientRatio;
	data.ambient.blue = data.diffuse.blue * cfg.ambientRatio;
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
		logger::error("light nullptr for node {}", cfg.nodeName);
		return;
	}


	if (auto* overlay = Overlay::Get(niPointLight)) {

		overlay->size = cfg.size; // isl
		overlay->cutoffOverride = cfg.cutoffOverride; // isl 
		overlay->fade = cfg.brightness;
		overlay->radius = cfg.radius;
		overlay->lighFormId = 0;
		overlay->unk138 = static_cast<std::uint32_t>(cfg.configID); 
	}
}

void LightData::setNiPointLightDataFromCfg(RE::FormID& formID,  RE::NiLight* niPointLight, const LightConfig& cfg) {
	if (!niPointLight) {
		logger::error("light nullptr for node {}", cfg.nodeName);
		return;
	}
	auto& data = niPointLight->GetLightRuntimeData();

	logger::debug(" Setting Light Data for {} from Configs", cfg.nodeName);

	data.fade = cfg.brightness;
	data.radius = getNiPointLightRadius(cfg);

	data.unk138 = cfg.configID;

	logger::debug(" radius set to: {} ", cfg.radius);
	logger::debug(" brightness set to: {} ", cfg.brightness);
	logger::debug("config ID set to {}", cfg.configID); 

	setNiPointLightPos(niPointLight, cfg);

	logger::debug(" position set to: x:{} y:{} z:{} ", cfg.position[0], cfg.position[1], cfg.position[2]);

	setNiPointLightAmbientAndDiffuse(niPointLight, cfg);

	logger::debug(" diffuse color set to: r:{} g:{} b:{} ", cfg.diffuseColor[0], cfg.diffuseColor[1], cfg.diffuseColor[2]);

	if (globals::islInstalled) setOverlayData(niPointLight, cfg);

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

bool LightData::foundConfigForLight(const RE::NiLight* light) {
	
	for  (auto& [name, vectorOfConfigs] : LightData::nodeNameToJsonCfg) {
		for (auto& cfg : vectorOfConfigs) {
			if (light->unk138 == cfg.configID) {
				return true;
			}
		}
	}

	for (auto& [meshFilePaths, vectorOfConfigs] : LightData::meshPathToJsonCfg) {
		for (auto& cfg : vectorOfConfigs) {
			if (light->unk138 == cfg.configID) {
				return true;
			}
		}
	}

	return false;
}

void LightData::updateConfigFromLight(LightConfig& cfg, RE::NiLight* niLight) {
	auto& rt = niLight->GetLightRuntimeData();
	auto& dataExt = LightData::configIDToJsonCfg[rt.unk138];

	cfg = dataExt; 

	cfg.radius = rt.radius.x;
	cfg.brightness = rt.fade;

	cfg.position[0] = niLight->local.translate.x;
	cfg.position[1] = niLight->local.translate.y;
	cfg.position[2] = niLight->local.translate.z;

	cfg.diffuseColor[0] = int(rt.diffuse.red * 255.0f);
	cfg.diffuseColor[1] = int(rt.diffuse.green * 255.0f);
	cfg.diffuseColor[2] = int(rt.diffuse.blue * 255.0f);

	cfg.flickerIntensity = dataExt.flickerIntensity;
	cfg.flickersPerSecond = dataExt.flickersPerSecond;

	if (globals::islInstalled) {

		if (auto* overlay = Overlay::Get(niLight)) {
			cfg.size = overlay->size;
			cfg.cutoffOverride = overlay->cutoffOverride;
		}
	}
	cfg.print();
}
