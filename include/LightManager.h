#pragma once

#include "logger.hpp"
#include "LightData.h"



// correct timing to attach lights because world position data is loaded, earlier = lights show up at cell origin 0,0,0
struct Load3D {

    static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading);

    static inline REL::Relocation<decltype(thunk)> func;

    static constexpr std::size_t idx{ 0x6A };

    static void Install();
};


struct LightManager : RE::BSTEventSink<RE::BGSActorCellEvent> {

    	static LightManager* GetSingleton()
    {
        static LightManager singleton;
        return &singleton;
    }

    static void attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg);

    static void registerEventSink();

    static void attachLightUsingAttachPath(const LightConfig& cfg, RE::NiNode* root, RE::NiPointLight* light);

   static bool processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root);

   static void processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this);

   static bool dummyHandler(RE::TESObjectREFR* a_this, const RE::BSFixedString& nodeName, RE::NiNode* a_root);

  static void reinitializeLightsWithinRange(RE::PlayerCharacter* player); 

  static void attachOrMergeLight(RE::TESObjectREFR* a_this,
      RE::NiPointLight* childLight, const LightConfig& cfg, RE::NiNode* a_root);

private:
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

};

