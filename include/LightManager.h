#pragma once

#include "logger.hpp"
#include "LightData.h"


struct LightManager : RE::BSTEventSink<RE::BGSActorCellEvent> {

    	static LightManager* GetSingleton()
    {
        static LightManager singleton;
        return &singleton;
    }

    static void attachNiPointLightToShadowSceneNode(RE::NiLight* niPointLight, const LightConfig& cfg, RE::TESObjectREFR* a_this);

    static void registerEventSink();

    static void attachLightUsingAttachPath(const LightConfig& cfg, RE::NiNode* root, RE::NiPointLight* light, RE::FormID refFormID);

   static bool processByFilePath(RE::TESObjectREFR* a_this, RE::NiNode* a_root);

   static void processByNodeName(RE::NiNode* a_root, const RE::BSFixedString& match, RE::TESObjectREFR* a_this);

   static bool dummyHandler(RE::TESObjectREFR* a_this, const RE::BSFixedString& nodeName, RE::NiNode* a_root);

  static void reinitializeLightsWithinRange(RE::PlayerCharacter* player); 

  static void attachOrMergeLight(RE::TESObjectREFR* a_this,
      RE::NiPointLight* childLight, const LightConfig& cfg, RE::NiNode* a_root, const float radius);

  static void AttachDebugMarker(RE::NiNode* a_node, RE::NiLight* light);


private:
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;

};

