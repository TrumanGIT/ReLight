#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include <unordered_map>
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
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

	static std::unordered_map<uint32_t, LightConfig> defaultConfigs;

	static void ResetTriLightCache();

	static bool ContainsEditorID(
		const std::string& edid,
		const std::vector<std::string>& keywords);

	static void setNiPointLightAmbientAndDiffuse(RE::NiLight* niPointLight, const LightConfig& cfg);

	static void setNiPointLightDataFromCfg(RE::NiLight* niPointLight, const LightConfig& cfg, const float scale);

	static void setNiPointLightPos(RE::NiLight* light, const LightConfig& cfg);

	static RE::NiPoint3 getNiPointLightRadius(const LightConfig& cfg, const float scale);

	static void setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg);

	static float GetFOV(LightConfig cfg);

	static RE::ShadowSceneNode::LIGHT_CREATE_PARAMS makeLightParams(const LightConfig& cfg);

	static bool foundConfigForLight(const RE::NiLight* light);

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


static	bool HasLightFlag(uint32_t flags, LIGHT_FLAGS flag)
	{
		return (flags & static_cast<uint32_t>(flag)) != 0;
	}

static bool ShouldMergeByFlags(uint32_t refAflags, uint32_t otherRefFlags)
	{
		// same-type merges
		if (HasLightFlag(refAflags, LIGHT_FLAGS::kCandle) &&
			HasLightFlag(otherRefFlags, LIGHT_FLAGS::kCandle)) {
			return true;
		}

		if (HasLightFlag(refAflags, LIGHT_FLAGS::kFire) &&
			HasLightFlag(otherRefFlags, LIGHT_FLAGS::kFire)) {
			return true;
		}

		// giant campfires should merge with fires
		if ((refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kGiantCampfire) &&
			otherRefFlags & static_cast<uint32_t>(LIGHT_FLAGS::kFire)) ||

			(refAflags & static_cast<uint32_t>(LIGHT_FLAGS::kFire) &&
				otherRefFlags & static_cast<uint32_t>(LIGHT_FLAGS::kGiantCampfire)))
		{
			return true;
		}

		if (HasLightFlag(refAflags, LIGHT_FLAGS::kOther) &&
			HasLightFlag(otherRefFlags, LIGHT_FLAGS::kOther)) {
			return true;
		}

		return false;
	}
	// void initialize();
};





