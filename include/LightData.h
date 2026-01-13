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

// extend ni point light runtime data so Inverse squared lighting sees our lights otherwise darkness
//also handy data container extention
struct Overlay
{
	std::uint32_t flags;       // I dont think we need this but idk can ignore for now
	float         cutoffOverride;//ISL need for isl from config
	RE::FormID    lighFormId; // ISL dont need
	RE::NiColor   diffuse;
	float         radius;
	float         pad1C;
	float         size; //ISL need for isl from config
	float         fade;
	std::uint32_t unk138;

	float         flickerIntensity = 0.2f;   // Relight specific values
	float         flickersPerSecond = 3.0f;  // Relight specific values

	float speedRandomness = 1.0; // this is if we want random flickers or not

	float seed; 

	float time; // used in flicker calcs

	float startingFade; // used in flicker calcs 

	bool initialized = false;

	static Overlay* Get(RE::NiLight* niLight)
	{
		return reinterpret_cast<Overlay*>(&niLight->GetLightRuntimeData());
	}

	uint32_t rngState = 1;

	float getRandomFloat(const float& min, const float& max)
	{
		return min + (max - min) * Random::rand(rngState);
	}
};

//the engine func to attach lights to obejects
inline RE::NiLight* CallGenDynamic(
	RE::TESObjectLIGH* light,
	RE::TESObjectREFR* ref,
	RE::NiNode* node,
	bool forceDynamic,
	bool useLightRadius,
	bool affectRefOnly)
{
	using func_t = RE::NiLight* (
		RE::TESObjectLIGH*,
		RE::TESObjectREFR*,
		RE::NiNode*,
		bool,
		bool,
		bool);

	static REL::Relocation<func_t> func{ RELOCATION_ID(17208, 17610) };
	return func(light, ref, node, forceDynamic, useLightRadius, affectRefOnly);
}

struct LightData : public RE::BSTEventSink<RE::BGSActorCellEvent> {

	static LightData* GetSingleton()
	{
		static LightData singleton;
		return &singleton;
	}

	static std::map<std::string, LightConfig> nodeNameToJsonCfg;

	static std::unordered_map<std::string, LightConfig> defaultConfigs;

	static void onKDataLoaded();
//	static void refillBankForSelectedTemplate(const std::string& lightName, const LightConfig& cfg);
	//static void assignNiPointLightsToBank(RE::NiPointer<RE::NiPointLight> niPointLight);
	static bool shouldDisableLight(RE::TESObjectLIGH* light, RE::TESObjectREFR* ref);
	static bool excludeLightEditorID(const RE::TESObjectLIGH* light);
	// template <class T>
	// inline REX::EnumSet<RE::TES_LIGHT_FLAGS, std::uint32_t> ParseLightFlags(const T& obj);
	static void setNiPointLightAmbientAndDiffuse(RE::NiLight* niPointLight, const LightConfig& cfg);
	static void setNiPointLightDataFromCfg(RE::NiLight* niPointLight, const LightConfig& cfg, std::string lightName);
	static void setNiPointLightPos(RE::NiLight* light, const LightConfig& cfg);
	static RE::NiPoint3 getNiPointLightRadius(const LightConfig& cfg);
	static  RE::NiPointer<RE::NiPointLight> createNiPointLight();
	static void setOverlayData(RE::NiLight* niPointLight, const LightConfig& cfg, std::string lightName);
	static void setRelightFlag(RE::TESObjectLIGH* ligh); 
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

 