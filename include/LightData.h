#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include <unordered_map>
#include "logger.hpp"
#include "ClibUtil/EditorID.hpp"
#include "config.hpp"
#include "random.h"
#include "global.h"

struct PointLight
{
	RE::NiPointer<RE::NiPointLight> node;
	PointLight()
	{
		RE::NiPointer<RE::NiPointLight> tmp;
		tmp.reset(RE::NiPointLight::Create());

		auto* cloned = tmp->Clone();
		node.reset(netimmerse_cast<RE::NiPointLight*>(cloned));
	}

	static PointLight& getMasterPointLight() {
		static PointLight pl; 

		return pl;

	}
};


// extend ni point light runflickerTime data so Inverse squared lighting sees our lights otherwise darkness
//also handy data container extention
struct Overlay
{
	std::uint32_t flags;       // this is needeed for linear lighting, must apply the flag
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

struct LightData : RE::BSTEventSink<RE::BGSActorCellEvent> {

	static LightData* GetSingleton()
	{
		static LightData singleton;
		return &singleton;
	}

	static std::map<uint64_t, LightConfig> configIDToJsonCfg;

	static std::map<std::string, LightConfig> nodeNameToJsonCfg;

	static std::unordered_map<std::string, LightConfig> defaultConfigs;

	static void registerEventSink();
//	static void refillBankForSelectedTemplate(const std::string& lightName, const LightConfig& cfg);
	//static void assignNiPointLightsToBank(RE::NiPointer<RE::NiPointLight> niPointLight);
	static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref);
	static bool excludeLightEditorID(const RE::TESObjectLIGH* light);
	// template <class T> 
	// inline REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> ParseLightFlags(const T& obj);
	static void setNiPointLightAmbientAndDiffuse(RE::NiLight* niPointLight, const LightConfig& cfg);
	static void setNiPointLightDataFromCfg(RE::NiLight* niPointLight, const LightConfig& cfg,const std::string& lightName);
	static void setNiPointLightPos(RE::NiLight* light, const LightConfig& cfg);
	static RE::NiPoint3 getNiPointLightRadius(const LightConfig& cfg);
	static void setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg, const std::string& lightName);
	//static void setRelightFlag(RE::TESObjectLIGH* ligh); 
	static RE::ShadowSceneNode::LIGHT_CREATE_PARAMS makeLightParams(const LightConfig& cfg);
	static void attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg);
	static bool findConfigForLight(LightConfig& cfg, const std::string& lightName);
	static void updateConfigFromLight(LightConfig& cfg, RE::NiLight* niLight);
	static void attachLightUsingAttachPath(const LightConfig& cfg, RE::NiNode* root, RE::NiPointLight* light);
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

private:
	RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
	// void initialize();
};

 
