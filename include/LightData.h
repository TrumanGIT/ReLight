#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include <unordered_map>
#include <ClibUtil/EditorID.hpp>

#include "logger.hpp"
#include "global.h"
#include "config.hpp"


class NiPointLight
{
public:
	RE::NiPointer<RE::NiPointLight> light;

	NiPointLight()
	{
		RE::NiPointLight* tmp = RE::NiPointLight::Create();

		if (!tmp) {
			return;
		}

		auto clone = netimmerse_cast<RE::NiPointLight*>(tmp->Clone());
		if (!clone) {
			return;
		}

		light.reset(clone);
	}
};

// extend ni point light runflickerTime data so Inverse squared lighting sees our lights otherwise darkness
//also handy data container extention
struct Overlay
{
	std::uint32_t flags;       // this is needeed for linear lighting (LL), must apply the flag probly a bit mask. (LL support has yet to be added) 
	float         cutoffOverride;//ISL need for isl from config
	RE::FormID    lighFormId; // ISL dont need
	RE::NiColor   diffuse;
	float         radius;
	float         pad1C;
	float         size; //ISL need for isl from config
	float         fade;
	std::uint32_t unk138;

	static Overlay* Get(RE::NiLight* niLight)
	{
		return reinterpret_cast<Overlay*>(&niLight->GetLightRuntimeData());
	}
};

struct LightData {

	struct TriLightCache
	{
		RE::BSLightingShaderProperty* lightShaderProp;
		RE::BSLight* lights[7];
	};

	static std::mutex triLightCacheMutex;
	static std::deque<TriLightCache> triLightCache;
	static std::atomic<uint16_t> triLightCacheGeneration;

	static NiPointLight masterNiPointLight;

	static std::map<uint32_t, LightConfig> configIDToJsonCfg;

	static std::unordered_map<std::string, std::vector<LightConfig>> meshPathToJsonCfg;

	static std::unordered_map<std::string, std::vector<LightConfig>> meshPathToJsonCfgExteriors;

	static std::unordered_map<RE::FormID, std::vector<LightConfig>> refFormIDToJsonCfg;
	static std::unordered_map<RE::FormID, std::vector<LightConfig>> refFormIDToJsonCfgExteriors;

	static std::unordered_map<RE::FormID, std::vector<LightConfig>> baseFormIDToJsonCfg;
	static std::unordered_map<RE::FormID, std::vector<LightConfig>> baseFormIDToJsonCfgExteriors;

	static std::unordered_map<uint32_t, LightConfig> defaultConfigs;

	static RE::TESForm* GetPluginLightEmittanceSource(RE::TESObjectREFR* ref);

	static void SetPluginLightEmittanceSource(RE::TESObjectREFR* ref, RE::TESForm* form);

	static void ResetTriLightCache();

	static void setNiPointLightAmbientAndDiffuse(RE::NiLight* niPointLight, const LightConfig& cfg);

	static void setNiPointLightDataFromCfg(RE::NiLight* niPointLight, const LightConfig& cfg, float scale);

	static void setNiPointLightPos(RE::NiLight* light, const LightConfig& cfg);

	static RE::NiPoint3 getNiPointLightRadius(const LightConfig& cfg, const float scale);

	static RE::TESObjectREFR* GetRefFromLight(RE::NiLight* light);

	static void setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg);

	static float GetFOV(const LightConfig& cfg);

	static RE::ShadowSceneNode::LIGHT_CREATE_PARAMS makeLightParams(const LightConfig& cfg);

	static bool foundConfigForLightByConfigID(const RE::NiLight* light);

	static std::vector<LightConfig>* findConfigsByFormID(
		RE::FormID formID,
		bool isInterior,
		bool isBaseID);

	static void updateConfigFromLight(LightConfig& cfg, const LightConfig& baseConfig, RE::NiLight* niLight);

	static bool updateRuntimeConfigCaches(const LightConfig& updatedCfg);

	static void InvalidateTriLightCacheForActivator(RE::TESObjectREFR* ref);

	static void AddConfigToMaps(
		const LightConfig& cfg,
		bool isRefLight,
		RE::FormID refFormID);

	static void printLightParams(const RE::ShadowSceneNode::LIGHT_CREATE_PARAMS& params) {
		logger::debug(" shadowLight	 {}", params.shadowLight);
		logger::debug(" portalStrict  {}", params.portalStrict);
		logger::debug(" affectLand	 {}", params.affectLand);
		logger::debug(" affectWater	 {}", params.affectWater);
		logger::debug(" neverFades	 {}", params.neverFades);
		logger::debug(" fov			 {}", params.fov);
		logger::debug(" falloff		 {}", params.falloff);
		logger::debug(" nearDistance  {}", params.nearDistance);
		logger::debug(" depthBias	 {}", params.depthBias);
	}

	static void SetTESObjectLightDataFromConfig(RE::TESObjectLIGH* light, const LightConfig& config);

	static RE::TESObjectLIGH* GetTESObjectLightFromNiLight(RE::NiLight* niLight);

	static bool HasRelightFlag(uint32_t flags, RELIGHT_FLAGS flag);

	static bool ShouldMergeByFlags(uint32_t refAflags, uint32_t otherRefFlags);

}; 